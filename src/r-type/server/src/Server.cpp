/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Server implementation
*/

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
    , next_session_id_(1)
{
}

Server::~Server()
{
    stop();
}

bool Server::start()
{
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

    if (!network_plugin_->start_server(tcp_port_, udp_port_)) {
        std::cerr << "[Server] Failed to start hybrid server\n";
        return false;
    }

    // Initialize components
    network_handler_ = std::make_unique<NetworkHandler>(network_plugin_);
    packet_sender_ = std::make_unique<PacketSender>(network_plugin_);
    session_manager_ = std::make_unique<GameSessionManager>();

    // Connect listeners (simple: just set "this" as listener for everything)
    network_handler_->set_listener(this);
    lobby_manager_.set_listener(this);
    session_manager_->set_listener(this);

    // Set callback for TCP client disconnection
    network_plugin_->set_on_client_disconnected([this](uint32_t client_id) {
        on_tcp_client_disconnected(client_id);
    });

    running_ = true;
    std::cout << "[Server] Server started successfully\n";
    std::cout << "[Server] TCP port " << tcp_port_ << " - Connections, Lobby\n";
    std::cout << "[Server] UDP port " << udp_port_ << " - Gameplay\n";
    std::cout << "[Server] Waiting for connections...\n";
    return true;
}

void Server::stop()
{
    if (!running_)
        return;
    std::cout << "[Server] Stopping server...\n";
    running_ = false;

    if (network_plugin_)
        network_plugin_->stop_server();
    std::cout << "[Server] Server stopped\n";
}

void Server::run()
{
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

        // 1. Process network packets (TCP + UDP)
        network_handler_->process_packets();

        // 2. Update lobby system
        lobby_manager_.update();

        // 3. Update all active game sessions
        session_manager_->update_all(delta_time);
        session_manager_->cleanup_inactive_sessions();

        // 4. Sleep to maintain tick rate
        auto tick_end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tick_end - tick_start);
        if (elapsed < tick_duration)
            std::this_thread::sleep_for(tick_duration - elapsed);
    }
}

// === INetworkListener Implementation ===

void Server::on_client_connect(uint32_t client_id, const protocol::ClientConnectPayload& payload)
{
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

    packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_ACCEPT,
                                    serialize(accept));

    std::cout << "[Server] Client " << client_id << " accepted with player ID " << player_id << "\n";
    std::cout << "[Server] Total connected clients: " << connected_clients_.size() << "\n";
}

void Server::on_client_disconnect(uint32_t client_id, const protocol::ClientDisconnectPayload& payload)
{
    auto it = connected_clients_.find(client_id);
    if (it == connected_clients_.end())
        return;

    std::cout << "[Server] Client " << client_id << " (" << it->second.player_name << ") disconnecting\n";

    player_to_client_.erase(it->second.player_id);
    connected_clients_.erase(it);
    std::cout << "[Server] Total connected clients: " << connected_clients_.size() << "\n";
}

void Server::on_client_ping(uint32_t client_id, const protocol::ClientPingPayload& payload)
{
    if (connected_clients_.find(client_id) == connected_clients_.end())
        return;

    protocol::ServerPongPayload pong;
    pong.client_timestamp = payload.client_timestamp;
    pong.server_timestamp = htonl(static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    ));
    packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_PONG, serialize(pong));
}

void Server::on_client_join_lobby(uint32_t client_id, const protocol::ClientJoinLobbyPayload& payload)
{
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

void Server::on_client_leave_lobby(uint32_t client_id, const protocol::ClientLeaveLobbyPayload& payload)
{
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

void Server::on_udp_handshake(uint32_t udp_client_id, const protocol::ClientUdpHandshakePayload& payload)
{
    uint32_t player_id = ntohl(payload.player_id);
    uint32_t session_id = ntohl(payload.session_id);

    std::cout << "[Server] UDP handshake from player " << player_id
              << " for session " << session_id << "\n";

    for (auto& [tcp_client_id, player_info] : connected_clients_) {
        if (player_info.player_id == player_id) {
            network_plugin_->associate_udp_client(tcp_client_id, udp_client_id);
            player_info.udp_client_id = udp_client_id;

            std::cout << "[Server] UDP associated: TCP client " << tcp_client_id
                      << " <-> UDP client " << udp_client_id << "\n";

            auto* session = session_manager_->get_session(session_id);
            if (session) {
                std::cout << "[Server] Resynchronizing player " << player_id << " with existing entities\n";
                session->resync_client(player_id, tcp_client_id);
            }
            return;
        }
    }

    std::cerr << "[Server] UDP handshake failed: player " << player_id << " not found\n";
}

void Server::on_client_input(uint32_t client_id, const protocol::ClientInputPayload& payload)
{
    uint32_t tcp_client_id = network_plugin_->get_tcp_client_from_udp(client_id);
    if (tcp_client_id == 0)
        tcp_client_id = client_id;

    auto it = connected_clients_.find(tcp_client_id);
    if (it == connected_clients_.end())
        return;

    if (!it->second.in_game)
        return;

    uint32_t session_id = it->second.session_id;
    auto* session = session_manager_->get_session(session_id);
    if (!session)
        return;

    session->handle_input(it->second.player_id, payload);
}

// === ILobbyListener Implementation ===

void Server::on_lobby_state_changed(uint32_t lobby_id, const std::vector<uint8_t>& payload)
{
    std::cout << "[Server] Broadcasting lobby state for lobby " << lobby_id << "\n";
    packet_sender_->broadcast_tcp_to_lobby(lobby_id, protocol::PacketType::SERVER_LOBBY_STATE,
                                          payload, lobby_manager_, connected_clients_);
}

void Server::on_countdown_tick(uint32_t lobby_id, uint8_t seconds_remaining)
{
    protocol::ServerGameStartCountdownPayload countdown;
    countdown.lobby_id = htonl(lobby_id);
    countdown.countdown_value = seconds_remaining;

    std::cout << "[Server] Countdown tick for lobby " << lobby_id << ": "
              << static_cast<int>(seconds_remaining) << "s\n";
    packet_sender_->broadcast_tcp_to_lobby(lobby_id, protocol::PacketType::SERVER_GAME_START_COUNTDOWN,
                                          serialize(countdown), lobby_manager_, connected_clients_);
}

void Server::on_game_start(uint32_t lobby_id, const std::vector<uint32_t>& player_ids)
{
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

    auto* session = session_manager_->create_session(session_id, game_mode, difficulty, 0);

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
        game_start.udp_port = htons(udp_port_);

        packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_GAME_START,
                                       serialize(game_start));
    }

    std::cout << "[Server] GameSession " << session_id << " created\n";
}

// === IGameSessionListener Implementation ===

void Server::on_state_snapshot(uint32_t session_id, const std::vector<uint8_t>& snapshot)
{
    auto* session = session_manager_->get_session(session_id);
    if (!session)
        return;
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_DELTA_SNAPSHOT,
                                            snapshot, session->get_player_ids(), connected_clients_);
}

void Server::on_entity_spawn(uint32_t session_id, const std::vector<uint8_t>& spawn_data)
{
    auto* session = session_manager_->get_session(session_id);
    if (!session)
        return;
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_ENTITY_SPAWN,
                                            spawn_data, session->get_player_ids(), connected_clients_);
}

void Server::on_entity_destroy(uint32_t session_id, uint32_t entity_id)
{
    protocol::ServerEntityDestroyPayload destroy;
    destroy.entity_id = htonl(entity_id);
    destroy.reason = protocol::DestroyReason::KILLED;
    destroy.position_x = 0.0f;
    destroy.position_y = 0.0f;

    auto* session = session_manager_->get_session(session_id);
    if (!session)
        return;
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_ENTITY_DESTROY,
                                            serialize(destroy), session->get_player_ids(), connected_clients_);
}

void Server::on_projectile_spawn(uint32_t session_id, const std::vector<uint8_t>& projectile_data)
{
    auto* session = session_manager_->get_session(session_id);
    if (!session)
        return;
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_PROJECTILE_SPAWN,
                                            projectile_data, session->get_player_ids(), connected_clients_);
}

void Server::on_wave_start(uint32_t session_id, const std::vector<uint8_t>& wave_data)
{
    auto* session = session_manager_->get_session(session_id);
    if (!session)
        return;
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_WAVE_START,
                                            wave_data, session->get_player_ids(), connected_clients_);
}

void Server::on_wave_complete(uint32_t session_id, const std::vector<uint8_t>& wave_data)
{
    auto* session = session_manager_->get_session(session_id);
    if (!session)
        return;
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_WAVE_COMPLETE,
                                            wave_data, session->get_player_ids(), connected_clients_);
}

void Server::on_game_over(uint32_t session_id, const std::vector<uint32_t>& player_ids, bool is_victory)
{
    std::cout << "[Server] Game over for session " << session_id
              << (is_victory ? " - VICTORY!" : " - DEFEAT!") << "\n";

    auto* session = session_manager_->get_session(session_id);

    protocol::ServerGameOverPayload game_over;
    game_over.result = is_victory ? protocol::GameResult::VICTORY : protocol::GameResult::DEFEAT;

    if (session) {
        packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_GAME_OVER,
                                                serialize(game_over), session->get_player_ids(),
                                                connected_clients_);
    }

    // Clean up player states
    for (uint32_t player_id : player_ids) {
        auto client_it = player_to_client_.find(player_id);
        if (client_it == player_to_client_.end())
            continue;
        auto& player_info = connected_clients_[client_it->second];
        player_info.in_game = false;
        player_info.session_id = 0;
    }

    // Also clean up any remaining players in the session
    for (auto& [tcp_client_id, player_info] : connected_clients_) {
        if (player_info.session_id == session_id) {
            player_info.in_game = false;
            player_info.session_id = 0;
        }
    }

    // Don't remove session here - it will be cleaned up by cleanup_inactive_sessions()
    // The session is already marked as inactive (is_active_ = false)
}

// === Internal Methods ===

void Server::on_tcp_client_disconnected(uint32_t client_id)
{
    auto it = connected_clients_.find(client_id);
    if (it == connected_clients_.end())
        return;

    std::cout << "[Server] Client " << client_id << " (" << it->second.player_name
              << ") disconnected (timeout or network error)\n";

    uint32_t player_id = it->second.player_id;
    lobby_manager_.leave_lobby(player_id);

    player_to_client_.erase(player_id);
    connected_clients_.erase(it);
    std::cout << "[Server] Total connected clients: " << connected_clients_.size() << "\n";
}

uint32_t Server::generate_player_id()
{
    return next_player_id_++;
}

uint32_t Server::generate_session_id()
{
    return next_session_id_++;
}

}
