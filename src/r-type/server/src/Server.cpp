#include "Server.hpp"
#include "ServerConfig.hpp"
#include "protocol/ProtocolEncoder.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

namespace rtype::server {

Server::Server(uint16_t port)
    : network_plugin_(nullptr)
    , port_(port)
    , running_(false)
    , next_player_id_(1)
    , next_session_id_(1) {
}

Server::~Server() {
    stop();
}

bool Server::start() {
    std::cout << "[Server] Starting server on port " << port_ << "...\n";

    try {
        network_plugin_ = plugin_manager_.load_plugin<engine::INetworkPlugin>("plugins/asio_network.so", "create_network_plugin");
        if (!network_plugin_) {
            std::cerr << "[Server] Failed to load network plugin\n";
            return false;
        }
        std::cout << "[Server] Network plugin loaded successfully\n";
    } catch (const std::exception& e) {
        std::cerr << "[Server] Exception loading plugin: " << e.what() << "\n";
        return false;
    }
    if (!network_plugin_->start_server(port_, engine::NetworkProtocol::UDP)) {
        std::cerr << "[Server] Failed to start server on port " << port_ << "\n";
        return false;
    }
    network_plugin_->set_on_client_disconnected([this](uint32_t client_id) {
        on_client_disconnected(client_id);
    });
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
    std::cout << "[Server] Server started successfully on UDP port " << port_ << "\n";
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
    const auto tick_duration = std::chrono::milliseconds(config::TICK_INTERVAL_MS); // 32 TPS = ~31ms per tick
    auto last_tick_time = std::chrono::steady_clock::now();

    std::cout << "[Server] Running at " << config::SERVER_TICK_RATE << " TPS (tick interval: "
              << config::TICK_INTERVAL_MS << "ms)\n";

    while (running_) {
        auto tick_start = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(tick_start - last_tick_time);
        float delta_time = delta.count() / 1000000.0f; // Convert to seconds
        last_tick_time = tick_start;

        // Process network packets
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
        route_packet(packet.sender_id, header, payload);
    }
}

void Server::route_packet(uint32_t client_id, const protocol::PacketHeader& header, const std::vector<uint8_t>& payload) {
    auto packet_type = static_cast<protocol::PacketType>(header.type);

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
            if (payload.size() != sizeof(protocol::ClientInputPayload)) {
                std::cerr << "[Server] Invalid CLIENT_INPUT payload size\n";
                return;
            }
            protocol::ClientInputPayload input_payload;
            std::memcpy(&input_payload, payload.data(), sizeof(input_payload));
            handle_client_input(client_id, input_payload);
            break;
        }
        default:
            std::cerr << "[Server] Unhandled packet type: 0x" << std::hex
                      << static_cast<int>(header.type) << std::dec << "\n";
            break;
    }
}

void Server::handle_client_connect(uint32_t client_id, const protocol::ClientConnectPayload& payload) {
    std::string player_name(payload.player_name);
    player_name = player_name.substr(0, player_name.find('\0'));

    std::cout << "[Server] Client " << client_id << " connecting as '" << player_name << "'\n";
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
    send_packet(client_id, protocol::PacketType::SERVER_ACCEPT, serialize(accept));
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
    send_packet(client_id, protocol::PacketType::SERVER_PONG, serialize(pong));
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

void Server::send_packet(uint32_t client_id, protocol::PacketType type, const std::vector<uint8_t>& payload) {
    protocol::PacketHeader header;
    header.version = protocol::PROTOCOL_VERSION;
    header.type = static_cast<uint8_t>(type);
    header.payload_length = static_cast<uint16_t>(payload.size());
    header.sequence_number = 0; // TODO: Implement proper sequence tracking

    std::vector<uint8_t> packet_data(protocol::HEADER_SIZE);
    protocol::ProtocolEncoder::encode_header(header, packet_data.data());
    packet_data.insert(packet_data.end(), payload.begin(), payload.end());

    engine::NetworkPacket packet;
    packet.data = packet_data;
    network_plugin_->send_to(packet, client_id);
}

void Server::broadcast_packet(protocol::PacketType type, const std::vector<uint8_t>& payload) {
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
    network_plugin_->broadcast(packet);
}

uint32_t Server::generate_player_id() {
    return next_player_id_++;
}

void Server::handle_client_join_lobby(uint32_t client_id, const protocol::ClientJoinLobbyPayload& payload) {
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

void Server::handle_client_leave_lobby(uint32_t client_id, const protocol::ClientLeaveLobbyPayload& payload) {
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
    std::cout << "[Server] Player " << player_id << " (" << it->second.player_name << ") leaving lobby\n";
    bool left = lobby_manager_.leave_lobby(player_id);
    if (left) {
        it->second.in_lobby = false;
        it->second.lobby_id = 0;
    }
}

void Server::broadcast_to_lobby(uint32_t lobby_id, protocol::PacketType type, const std::vector<uint8_t>& payload) {
    auto player_ids = lobby_manager_.get_lobby_players(lobby_id);

    for (uint32_t player_id : player_ids) {
        auto it = player_to_client_.find(player_id);
        if (it != player_to_client_.end())
            send_packet(it->second, type, payload);
    }
}

void Server::on_lobby_state_changed(uint32_t lobby_id, const std::vector<uint8_t>& payload) {
    std::cout << "[Server] Broadcasting lobby state for lobby " << lobby_id << "\n";
    broadcast_to_lobby(lobby_id, protocol::PacketType::SERVER_LOBBY_STATE, payload);
}

void Server::on_countdown_tick(uint32_t lobby_id, uint8_t seconds_remaining) {
    protocol::ServerGameStartCountdownPayload countdown;
    countdown.lobby_id = htonl(lobby_id);
    countdown.countdown_value = seconds_remaining;

    std::cout << "[Server] Countdown tick for lobby " << lobby_id << ": " << static_cast<int>(seconds_remaining) << "s\n";
    broadcast_to_lobby(lobby_id, protocol::PacketType::SERVER_GAME_START_COUNTDOWN, serialize(countdown));
}

void Server::on_game_start(uint32_t lobby_id, const std::vector<uint32_t>& player_ids) {
    std::cout << "[Server] Game starting for lobby " << lobby_id << " with " << player_ids.size() << " players\n";
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
        send_packet(client_id, protocol::PacketType::SERVER_GAME_START, serialize(game_start));
    }
    game_sessions_[session_id] = std::move(session);
    std::cout << "[Server] GameSession " << session_id << " created\n";
}

void Server::handle_client_input(uint32_t client_id, const protocol::ClientInputPayload& payload) {
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end())
        return;
    if (!it->second.in_game)
        return;
    uint32_t session_id = it->second.session_id;
    auto session_it = game_sessions_.find(session_id);
    if (session_it == game_sessions_.end())
        return;
    session_it->second->handle_input(it->second.player_id, payload);
}

void Server::broadcast_to_session(uint32_t session_id, protocol::PacketType type, const std::vector<uint8_t>& payload) {
    auto session_it = game_sessions_.find(session_id);

    if (session_it == game_sessions_.end())
        return;
    auto player_ids = session_it->second->get_player_ids();
    for (uint32_t player_id : player_ids) {
        auto it = player_to_client_.find(player_id);
        if (it != player_to_client_.end())
            send_packet(it->second, type, payload);
    }
}

void Server::on_state_snapshot(uint32_t session_id, const std::vector<uint8_t>& snapshot) {
    broadcast_to_session(session_id, protocol::PacketType::SERVER_DELTA_SNAPSHOT, snapshot);
}

void Server::on_entity_spawn(uint32_t session_id, const std::vector<uint8_t>& spawn_data) {
    broadcast_to_session(session_id, protocol::PacketType::SERVER_ENTITY_SPAWN, spawn_data);
}

void Server::on_entity_destroy(uint32_t session_id, uint32_t entity_id) {
    protocol::ServerEntityDestroyPayload destroy;
    destroy.entity_id = htonl(entity_id);
    destroy.reason = protocol::DestroyReason::KILLED;
    destroy.position_x = 0.0f;
    destroy.position_y = 0.0f;

    broadcast_to_session(session_id, protocol::PacketType::SERVER_ENTITY_DESTROY, serialize(destroy));
}

void Server::on_game_over(uint32_t session_id, const std::vector<uint32_t>& player_ids) {
    std::cout << "[Server] Game over for session " << session_id << "\n";

    for (uint32_t player_id : player_ids) {
        auto client_it = player_to_client_.find(player_id);
        if (client_it == player_to_client_.end())
            continue;
        auto& player_info = connected_clients_[client_it->second];
        player_info.in_game = false;
        player_info.session_id = 0;
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

uint32_t Server::generate_session_id() {
    return next_session_id_++;
}

}

