#include "Server.hpp"
#include "ServerConfig.hpp"
#include "protocol/ProtocolEncoder.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

namespace rtype::server {

Server::Server(uint16_t tcp_port, uint16_t udp_port)
    : network_plugin_(nullptr)
    , tcp_port_(tcp_port)
    , udp_port_(udp_port)
    , running_(false)
    , next_player_id_(1)
    , next_session_id_(1) {
}

Server::~Server() {
    stop();
}

bool Server::start() {
    std::cout << "[Server] Starting hybrid server - TCP:" << tcp_port_
              << " UDP:" << udp_port_ << "...\n";

    try {
        network_plugin_ = plugin_manager_.load_plugin<engine::INetworkPlugin>(
            "plugins/asio_network.so", "create_network_plugin");
        if (!network_plugin_) {
            std::cerr << "[Server] Failed to load network plugin\n";
            return false;
        }
        std::cout << "[Server] Network plugin loaded successfully\n";
    } catch (const std::exception& e) {
        std::cerr << "[Server] Exception loading plugin: " << e.what() << "\n";
        return false;
    }

    // Start hybrid server (TCP + UDP)
    if (!network_plugin_->start_server(tcp_port_, udp_port_)) {
        std::cerr << "[Server] Failed to start hybrid server\n";
        return false;
    }

    // Set callback for TCP client disconnection
    network_plugin_->set_on_client_disconnected([this](uint32_t client_id) {
        on_client_disconnected(client_id);
    });

    // Set lobby callbacks
    lobby_manager_.set_lobby_state_callback([this](uint32_t lobby_id, const std::vector<uint8_t>& payload) {
        on_lobby_state_changed(lobby_id, payload);
    });
    lobby_manager_.set_countdown_callback([this](uint32_t lobby_id, uint8_t seconds_remaining) {
        on_countdown_tick(lobby_id, seconds_remaining);
    });
    lobby_manager_.set_game_start_callback([this](uint32_t lobby_id, const std::vector<uint32_t>& player_ids) {
        on_game_start(lobby_id, player_ids);
    });

    running_ = true;
    std::cout << "[Server] Server started successfully\n";
    std::cout << "[Server] TCP port " << tcp_port_ << " - Connections, Lobby\n";
    std::cout << "[Server] UDP port " << udp_port_ << " - Gameplay\n";
    std::cout << "[Server] Waiting for connections...\n";
    return true;
}

void Server::stop() {
    if (!running_)
        return;
    std::cout << "[Server] Stopping server...\n";
    running_ = false;

    if (network_plugin_)
        network_plugin_->stop_server();
    std::cout << "[Server] Server stopped\n";
}

void Server::run() {
    if (!running_) {
        std::cerr << "[Server] Cannot run - server not started\n";
        return;
    }
    const auto tick_duration = std::chrono::milliseconds(config::TICK_INTERVAL_MS);
    auto last_tick_time = std::chrono::steady_clock::now();

    std::cout << "[Server] Running at " << config::SERVER_TICK_RATE << " TPS (tick interval: "
              << config::TICK_INTERVAL_MS << "ms)\n";

    while (running_) {
        auto tick_start = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(tick_start - last_tick_time);
        float delta_time = delta.count() / 1000000.0f;
        last_tick_time = tick_start;

        // Process network packets (TCP + UDP)
        handle_packets();

        // Update lobby system
        lobby_manager_.update();

        // Update all active game sessions
        update_game_sessions(delta_time);

        // Sleep to maintain tick rate
        auto tick_end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tick_end - tick_start);
        if (elapsed < tick_duration)
            std::this_thread::sleep_for(tick_duration - elapsed);
    }
}

void Server::handle_packets() {
    auto packets = network_plugin_->receive();

    for (const auto& packet : packets) {
        if (packet.data.size() < protocol::HEADER_SIZE) {
            std::cerr << "[Server] Packet too small from client " << packet.sender_id << "\n";
            continue;
        }
        protocol::PacketHeader header = protocol::ProtocolEncoder::decode_header(packet.data.data(), packet.data.size());
        if (header.version != protocol::PROTOCOL_VERSION) {
            std::cerr << "[Server] Invalid protocol version from client " << packet.sender_id << ": "
                      << static_cast<int>(header.version) << "\n";
            continue;
        }
        std::vector<uint8_t> payload(
            packet.data.begin() + protocol::HEADER_SIZE,
            packet.data.begin() + protocol::HEADER_SIZE + header.payload_length
        );
        route_packet(packet.sender_id, header, payload, packet.protocol);
    }
}

void Server::route_packet(uint32_t client_id, const protocol::PacketHeader& header,
                          const std::vector<uint8_t>& payload, engine::NetworkProtocol protocol) {
    auto packet_type = static_cast<protocol::PacketType>(header.type);

    // Route based on protocol
    if (protocol == engine::NetworkProtocol::TCP) {
        // TCP packets: connection, lobby, authentication
        switch (packet_type) {
            case protocol::PacketType::CLIENT_CONNECT: {
                if (payload.size() != sizeof(protocol::ClientConnectPayload)) {
                    std::cerr << "[Server] Invalid CLIENT_CONNECT payload size\n";
                    return;
                }
                protocol::ClientConnectPayload connect_payload;
                std::memcpy(&connect_payload, payload.data(), sizeof(connect_payload));
                handle_client_connect(client_id, connect_payload);
                break;
            }
            case protocol::PacketType::CLIENT_DISCONNECT: {
                if (payload.size() != sizeof(protocol::ClientDisconnectPayload)) {
                    std::cerr << "[Server] Invalid CLIENT_DISCONNECT payload size\n";
                    return;
                }
                protocol::ClientDisconnectPayload disconnect_payload;
                std::memcpy(&disconnect_payload, payload.data(), sizeof(disconnect_payload));
                handle_client_disconnect(client_id, disconnect_payload);
                break;
            }
            case protocol::PacketType::CLIENT_PING: {
                if (payload.size() != sizeof(protocol::ClientPingPayload)) {
                    std::cerr << "[Server] Invalid CLIENT_PING payload size\n";
                    return;
                }
                protocol::ClientPingPayload ping_payload;
                std::memcpy(&ping_payload, payload.data(), sizeof(ping_payload));
                handle_client_ping(client_id, ping_payload);
                break;
            }
            case protocol::PacketType::CLIENT_JOIN_LOBBY: {
                if (payload.size() != sizeof(protocol::ClientJoinLobbyPayload)) {
                    std::cerr << "[Server] Invalid CLIENT_JOIN_LOBBY payload size\n";
                    return;
                }
                protocol::ClientJoinLobbyPayload join_payload;
                std::memcpy(&join_payload, payload.data(), sizeof(join_payload));
                handle_client_join_lobby(client_id, join_payload);
                break;
            }
            case protocol::PacketType::CLIENT_LEAVE_LOBBY: {
                if (payload.size() != sizeof(protocol::ClientLeaveLobbyPayload)) {
                    std::cerr << "[Server] Invalid CLIENT_LEAVE_LOBBY payload size\n";
                    return;
                }
                protocol::ClientLeaveLobbyPayload leave_payload;
                std::memcpy(&leave_payload, payload.data(), sizeof(leave_payload));
                handle_client_leave_lobby(client_id, leave_payload);
                break;
            }
            case protocol::PacketType::CLIENT_INPUT: {
                // Also handle inputs via TCP as fallback when UDP isn't connected
                if (payload.size() != sizeof(protocol::ClientInputPayload)) {
                    std::cerr << "[Server] Invalid CLIENT_INPUT (TCP) payload size\n";
                    return;
                }
                protocol::ClientInputPayload input_payload;
                std::memcpy(&input_payload, payload.data(), sizeof(input_payload));
                handle_client_input(client_id, input_payload);
                break;
            }
            default:
                std::cerr << "[Server] Unexpected TCP packet type: 0x" << std::hex
                          << static_cast<int>(header.type) << std::dec << "\n";
                break;
        }
    } else if (protocol == engine::NetworkProtocol::UDP) {
        // UDP packets: gameplay, inputs
        switch (packet_type) {
            case protocol::PacketType::CLIENT_UDP_HANDSHAKE: {
                if (payload.size() != sizeof(protocol::ClientUdpHandshakePayload)) {
                    std::cerr << "[Server] Invalid CLIENT_UDP_HANDSHAKE payload size\n";
                    return;
                }
                protocol::ClientUdpHandshakePayload handshake_payload;
                std::memcpy(&handshake_payload, payload.data(), sizeof(handshake_payload));
                handle_udp_handshake(client_id, handshake_payload);
                break;
            }
            case protocol::PacketType::CLIENT_INPUT: {
                if (payload.size() != sizeof(protocol::ClientInputPayload)) {
                    std::cerr << "[Server] Invalid CLIENT_INPUT payload size\n";
                    return;
                }
                protocol::ClientInputPayload input_payload;
                std::memcpy(&input_payload, payload.data(), sizeof(input_payload));
                handle_client_input(client_id, input_payload);
                break;
            }
            case protocol::PacketType::CLIENT_PING: {
                // Ping can also come via UDP
                if (payload.size() != sizeof(protocol::ClientPingPayload)) {
                    return;
                }
                protocol::ClientPingPayload ping_payload;
                std::memcpy(&ping_payload, payload.data(), sizeof(ping_payload));
                // For UDP ping, we need to find the TCP client and respond via UDP
                uint32_t tcp_client_id = network_plugin_->get_tcp_client_from_udp(client_id);
                if (tcp_client_id != 0) {
                    handle_client_ping(tcp_client_id, ping_payload);
                }
                break;
            }
            default:
                std::cerr << "[Server] Unexpected UDP packet type: 0x" << std::hex
                          << static_cast<int>(header.type) << std::dec << "\n";
                break;
        }
    }
}

void Server::handle_client_connect(uint32_t client_id, const protocol::ClientConnectPayload& payload) {
    std::string player_name(payload.player_name);
    player_name = player_name.substr(0, player_name.find('\0'));

    std::cout << "[Server] TCP client " << client_id << " connecting as '" << player_name << "'\n";
    if (connected_clients_.find(client_id) != connected_clients_.end()) {
        std::cerr << "[Server] Client " << client_id << " already connected\n";
        return;
    }
    uint32_t player_id = generate_player_id();
    PlayerInfo info(client_id, player_id, player_name);
    connected_clients_[client_id] = info;
    player_to_client_[player_id] = client_id;
    protocol::ServerAcceptPayload accept;
    accept.assigned_player_id = htonl(player_id);
    accept.server_tick_rate = 60;
    accept.max_players = 4;
    accept.map_id = htons(0);

    send_tcp_packet(client_id, protocol::PacketType::SERVER_ACCEPT,
                    serialize(accept));

    std::cout << "[Server] Client " << client_id << " accepted with player ID " << player_id << "\n";
    std::cout << "[Server] Total connected clients: " << connected_clients_.size() << "\n";
}

void Server::handle_client_disconnect(uint32_t client_id, const protocol::ClientDisconnectPayload& payload) {
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end())
        return;
    std::cout << "[Server] Client " << client_id << " (" << it->second.player_name << ") disconnecting\n";
    // TODO Phase 2: Remove from lobby if in one
    // TODO Phase 3: Remove from game session if in one
    player_to_client_.erase(it->second.player_id);
    connected_clients_.erase(it);
    std::cout << "[Server] Total connected clients: " << connected_clients_.size() << "\n";
}

void Server::handle_client_ping(uint32_t client_id, const protocol::ClientPingPayload& payload) {
    if (connected_clients_.find(client_id) == connected_clients_.end())
        return;
    protocol::ServerPongPayload pong;
    pong.client_timestamp = payload.client_timestamp;
    pong.server_timestamp = htonl(static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    ));
    send_tcp_packet(client_id, protocol::PacketType::SERVER_PONG,
                    serialize(pong));
}

void Server::handle_udp_handshake(uint32_t udp_client_id,
                                  const protocol::ClientUdpHandshakePayload& payload) {
    uint32_t player_id = ntohl(payload.player_id);
    uint32_t session_id = ntohl(payload.session_id);

    std::cout << "[Server] UDP handshake from player " << player_id
              << " for session " << session_id << "\n";

    // Find the TCP client for this player
    for (auto& [tcp_client_id, player_info] : connected_clients_) {
        if (player_info.player_id == player_id) {
            // Associate UDP with TCP client
            network_plugin_->associate_udp_client(tcp_client_id, udp_client_id);
            player_info.udp_client_id = udp_client_id;

            std::cout << "[Server] UDP associated: TCP client " << tcp_client_id
                      << " <-> UDP client " << udp_client_id << "\n";
            return;
        }
    }

    std::cerr << "[Server] UDP handshake failed: player " << player_id << " not found\n";
}

void Server::on_client_disconnected(uint32_t client_id) {
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end())
        return;
    std::cout << "[Server] Client " << client_id << " (" << it->second.player_name
              << ") disconnected (timeout or network error)\n";

    uint32_t player_id = it->second.player_id;
    lobby_manager_.leave_lobby(player_id);

    // TODO Phase 3: Handle in-game disconnection
    player_to_client_.erase(player_id);
    connected_clients_.erase(it);
    std::cout << "[Server] Total connected clients: " << connected_clients_.size() << "\n";
}

// ============== TCP Sending ==============

void Server::send_tcp_packet(uint32_t client_id, protocol::PacketType type,
                             const std::vector<uint8_t>& payload) {
    protocol::PacketHeader header;
    header.version = protocol::PROTOCOL_VERSION;
    header.type = static_cast<uint8_t>(type);
    header.payload_length = static_cast<uint16_t>(payload.size());
    header.sequence_number = 0;

    std::vector<uint8_t> packet_data(protocol::HEADER_SIZE);
    protocol::ProtocolEncoder::encode_header(header, packet_data.data());
    packet_data.insert(packet_data.end(), payload.begin(), payload.end());

    engine::NetworkPacket packet;
    packet.data = packet_data;
    network_plugin_->send_tcp_to(packet, client_id);
}

void Server::broadcast_tcp_packet(protocol::PacketType type, const std::vector<uint8_t>& payload) {
    protocol::PacketHeader header;
    header.version = protocol::PROTOCOL_VERSION;
    header.type = static_cast<uint8_t>(type);
    header.payload_length = static_cast<uint16_t>(payload.size());
    header.sequence_number = 0;

    std::vector<uint8_t> packet_data(protocol::HEADER_SIZE);
    protocol::ProtocolEncoder::encode_header(header, packet_data.data());
    packet_data.insert(packet_data.end(), payload.begin(), payload.end());

    engine::NetworkPacket packet;
    packet.data = packet_data;
    network_plugin_->broadcast_tcp(packet);
}

void Server::broadcast_tcp_to_lobby(uint32_t lobby_id, protocol::PacketType type,
                                    const std::vector<uint8_t>& payload) {
    auto player_ids = lobby_manager_.get_lobby_players(lobby_id);

    for (uint32_t player_id : player_ids) {
        for (const auto& [client_id, player_info] : connected_clients_) {
            if (player_info.player_id == player_id) {
                send_tcp_packet(client_id, type, payload);
                break;
            }
        }
    }
}

// ============== UDP Sending ==============

void Server::send_udp_packet(uint32_t client_id, protocol::PacketType type,
                             const std::vector<uint8_t>& payload) {
    protocol::PacketHeader header;
    header.version = 0x01;
    header.type = static_cast<uint8_t>(type);
    header.payload_length = static_cast<uint16_t>(payload.size());
    header.sequence_number = 0;

    std::vector<uint8_t> packet_data(protocol::HEADER_SIZE);
    protocol::ProtocolEncoder::encode_header(header, packet_data.data());
    packet_data.insert(packet_data.end(), payload.begin(), payload.end());

    engine::NetworkPacket packet;
    packet.data = packet_data;
    network_plugin_->send_udp_to(packet, client_id);
}

void Server::broadcast_udp_to_session(uint32_t session_id, protocol::PacketType type,
                                      const std::vector<uint8_t>& payload) {
    auto session_it = game_sessions_.find(session_id);

    if (session_it == game_sessions_.end())
        return;

    protocol::PacketHeader header;
    header.version = 0x01;
    header.type = static_cast<uint8_t>(type);
    header.payload_length = static_cast<uint16_t>(payload.size());
    header.sequence_number = 0;

    std::vector<uint8_t> packet_data(protocol::HEADER_SIZE);
    protocol::ProtocolEncoder::encode_header(header, packet_data.data());
    packet_data.insert(packet_data.end(), payload.begin(), payload.end());

    engine::NetworkPacket packet;
    packet.data = packet_data;

    auto player_ids = session_it->second->get_player_ids();
    for (uint32_t player_id : player_ids) {
        for (const auto& [client_id, player_info] : connected_clients_) {
            if (player_info.player_id == player_id && player_info.has_udp_connection()) {
                network_plugin_->send_udp_to(packet, client_id);
                break;
            }
        }
    }
}

void Server::broadcast_tcp_to_session(uint32_t session_id, protocol::PacketType type,
                                      const std::vector<uint8_t>& payload) {
    auto session_it = game_sessions_.find(session_id);

    if (session_it == game_sessions_.end())
        return;

    auto player_ids = session_it->second->get_player_ids();
    for (uint32_t player_id : player_ids) {
        auto client_it = player_to_client_.find(player_id);
        if (client_it != player_to_client_.end()) {
            send_tcp_packet(client_it->second, type, payload);
        }
    }
}

// ============== Lobby Callbacks ==============

void Server::handle_client_join_lobby(uint32_t client_id,
                                      const protocol::ClientJoinLobbyPayload& payload) {
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end()) {
        std::cerr << "[Server] JOIN_LOBBY from unknown client " << client_id << "\n";
        return;
    }
    uint32_t player_id = ntohl(payload.player_id);
    if (player_id != it->second.player_id) {
        std::cerr << "[Server] Player ID mismatch in JOIN_LOBBY\n";
        return;
    }
    protocol::GameMode game_mode = static_cast<protocol::GameMode>(payload.game_mode);
    protocol::Difficulty difficulty = static_cast<protocol::Difficulty>(payload.difficulty);
    std::cout << "[Server] Player " << player_id << " (" << it->second.player_name
              << ") requesting lobby (mode: " << static_cast<int>(game_mode)
              << ", difficulty: " << static_cast<int>(difficulty) << ")\n";
    uint32_t lobby_id = lobby_manager_.join_lobby(player_id, game_mode, difficulty);
    if (lobby_id == 0) {
        std::cerr << "[Server] Failed to join/create lobby for player " << player_id << "\n";
        return;
    }
    it->second.in_lobby = true;
    it->second.lobby_id = lobby_id;
}

void Server::handle_client_leave_lobby(uint32_t client_id,
                                       const protocol::ClientLeaveLobbyPayload& payload) {
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end()) {
        std::cerr << "[Server] LEAVE_LOBBY from unknown client " << client_id << "\n";
        return;
    }
    uint32_t player_id = ntohl(payload.player_id);
    if (player_id != it->second.player_id) {
        std::cerr << "[Server] Player ID mismatch in LEAVE_LOBBY\n";
        return;
    }
    std::cout << "[Server] Player " << player_id << " (" << it->second.player_name
              << ") leaving lobby\n";
    bool left = lobby_manager_.leave_lobby(player_id);
    if (left) {
        it->second.in_lobby = false;
        it->second.lobby_id = 0;
    }
}

void Server::on_lobby_state_changed(uint32_t lobby_id, const std::vector<uint8_t>& payload) {
    std::cout << "[Server] Broadcasting lobby state for lobby " << lobby_id << "\n";
    broadcast_tcp_to_lobby(lobby_id, protocol::PacketType::SERVER_LOBBY_STATE, payload);
}

void Server::on_countdown_tick(uint32_t lobby_id, uint8_t seconds_remaining) {
    protocol::ServerGameStartCountdownPayload countdown;
    countdown.lobby_id = htonl(lobby_id);
    countdown.countdown_value = seconds_remaining;

    std::cout << "[Server] Countdown tick for lobby " << lobby_id << ": "
              << static_cast<int>(seconds_remaining) << "s\n";
    broadcast_tcp_to_lobby(lobby_id, protocol::PacketType::SERVER_GAME_START_COUNTDOWN,
                           serialize(countdown));
}

void Server::on_game_start(uint32_t lobby_id, const std::vector<uint32_t>& player_ids) {
    std::cout << "[Server] Game starting for lobby " << lobby_id
              << " with " << player_ids.size() << " players\n";
    const auto* lobby = lobby_manager_.get_lobby(lobby_id);

    if (!lobby) {
        std::cerr << "[Server] Cannot start game - lobby " << lobby_id << " not found\n";
        return;
    }
    protocol::GameMode game_mode = lobby->game_mode;
    protocol::Difficulty difficulty = lobby->difficulty;
    uint32_t session_id = generate_session_id();

    auto session = std::make_unique<GameSession>(session_id, game_mode, difficulty, 0);
    session->set_state_snapshot_callback([this](uint32_t sid, const std::vector<uint8_t>& snapshot) {
        on_state_snapshot(sid, snapshot);
    });
    session->set_entity_spawn_callback([this](uint32_t sid, const std::vector<uint8_t>& spawn_data) {
        on_entity_spawn(sid, spawn_data);
    });
    session->set_entity_destroy_callback([this](uint32_t sid, uint32_t entity_id) {
        on_entity_destroy(sid, entity_id);
    });
    session->set_game_over_callback([this](uint32_t sid, const std::vector<uint32_t>& pids) {
        on_game_over(sid, pids);
    });

    for (uint32_t player_id : player_ids) {
        auto client_it = player_to_client_.find(player_id);
        if (client_it == player_to_client_.end())
            continue;
        uint32_t client_id = client_it->second;
        auto& player_info = connected_clients_[client_id];
        player_info.in_lobby = false;
        player_info.lobby_id = 0;
        player_info.in_game = true;
        player_info.session_id = session_id;
        session->add_player(player_id, player_info.player_name);
        protocol::ServerGameStartPayload game_start;
        game_start.game_session_id = htonl(session_id);
        game_start.game_mode = game_mode;
        game_start.difficulty = difficulty;
        game_start.server_tick = htonl(0);
        game_start.level_seed = htonl(0);
        game_start.udp_port = htons(udp_port_);  // Send the actual UDP port
        send_tcp_packet(client_id, protocol::PacketType::SERVER_GAME_START, serialize(game_start));
    }
    game_sessions_[session_id] = std::move(session);
    std::cout << "[Server] GameSession " << session_id << " created\n";
}

// ============== Input Handling ==============

void Server::handle_client_input(uint32_t client_id, const protocol::ClientInputPayload& payload) {
    // For UDP packets, client_id is the UDP client ID
    // We need to find the corresponding TCP client
    uint32_t tcp_client_id = network_plugin_->get_tcp_client_from_udp(client_id);
    if (tcp_client_id == 0) {
        // Maybe client_id is already the TCP client ID
        tcp_client_id = client_id;
    }

    auto it = connected_clients_.find(tcp_client_id);
    if (it == connected_clients_.end()) {
        std::cerr << "[Server] Input from unknown client " << client_id << " (tcp=" << tcp_client_id << ")\n";
        return;
    }
    if (!it->second.in_game) {
        std::cerr << "[Server] Input from player not in game\n";
        return;
    }

    uint32_t session_id = it->second.session_id;
    auto session_it = game_sessions_.find(session_id);
    if (session_it == game_sessions_.end()) {
        std::cerr << "[Server] Input for non-existent session " << session_id << "\n";
        return;
    }

    // Debug: log inputs periodically
    static int input_count = 0;
    input_count++;
    uint16_t flags = ntohs(payload.input_flags);
    if (input_count % 60 == 1 || flags != 0) {  // Log every second or when there's actual input
        std::cout << "[Server] Input #" << input_count << " from player " << it->second.player_id
                  << " flags=0x" << std::hex << flags << std::dec << "\n";
    }

    session_it->second->handle_input(it->second.player_id, payload);
}

void Server::on_state_snapshot(uint32_t session_id, const std::vector<uint8_t>& snapshot) {
    broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_DELTA_SNAPSHOT, snapshot);
}

void Server::on_entity_spawn(uint32_t session_id, const std::vector<uint8_t>& spawn_data) {
    // Use TCP for spawns to ensure reliability (UDP might not be connected yet for initial spawns)
    broadcast_tcp_to_session(session_id, protocol::PacketType::SERVER_ENTITY_SPAWN, spawn_data);
}

void Server::on_entity_destroy(uint32_t session_id, uint32_t entity_id) {
    protocol::ServerEntityDestroyPayload destroy;
    destroy.entity_id = htonl(entity_id);
    destroy.reason = protocol::DestroyReason::KILLED;
    destroy.position_x = 0.0f;
    destroy.position_y = 0.0f;

    broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_ENTITY_DESTROY,
                             serialize(destroy));
}

void Server::on_game_over(uint32_t session_id, const std::vector<uint32_t>& player_ids) {
    std::cout << "[Server] Game over for session " << session_id << " (" << player_ids.size() << " players)\n";

    // Get player IDs from the session if list is empty (for victory case)
    std::vector<uint32_t> actual_player_ids = player_ids;
    if (actual_player_ids.empty()) {
        auto session_it = game_sessions_.find(session_id);
        if (session_it != game_sessions_.end()) {
            actual_player_ids = session_it->second->get_player_ids();
        }
    }

    // Send game over packet via TCP (reliable) to all players in session
    protocol::ServerGameOverPayload game_over;
    game_over.result = protocol::GameResult::VICTORY;  // Could be DEFEAT based on context
    game_over.total_time = htonl(0);  // TODO: Track actual game time
    game_over.enemies_killed = htonl(0);  // TODO: Track enemies killed

    for (uint32_t player_id : actual_player_ids) {
        auto client_it = player_to_client_.find(player_id);
        if (client_it == player_to_client_.end())
            continue;

        uint32_t client_id = client_it->second;
        send_tcp_packet(client_id, protocol::PacketType::SERVER_GAME_OVER, serialize(game_over));

        auto& player_info = connected_clients_[client_id];
        player_info.in_game = false;
        player_info.session_id = 0;
        std::cout << "[Server] Sent game over to player " << player_id << "\n";
    }

    game_sessions_.erase(session_id);
}

void Server::update_game_sessions(float delta_time) {
    std::vector<uint32_t> sessions_to_remove;

    for (auto& [session_id, session] : game_sessions_) {
        session->update(delta_time);
        if (!session->is_active())
            sessions_to_remove.push_back(session_id);
    }
    for (uint32_t session_id : sessions_to_remove) {
        std::cout << "[Server] Removing inactive session " << session_id << "\n";
        game_sessions_.erase(session_id);
    }
}

// ============== Utilities ==============

uint32_t Server::generate_player_id() {
    return next_player_id_++;
}

uint32_t Server::generate_session_id() {
    return next_session_id_++;
}

}
