/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Server implementation
*/

#include "Server.hpp"
#include "ServerConfig.hpp"
#include "protocol/ProtocolEncoder.hpp"
#include "plugin_manager/PluginPaths.hpp"
#include "NetworkUtils.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>

namespace rtype::server {

using netutils::ByteOrder;

namespace {
std::string hash_password(const std::string& password)
{
    std::hash<std::string> hasher;
    size_t hash_value = hasher(password);

    std::ostringstream oss;
    oss << std::hex << hash_value;
    return oss.str();
}
}

Server::Server(uint16_t tcp_port, uint16_t udp_port,
               bool listen_on_all_interfaces, const std::string& admin_password)
    : network_plugin_(nullptr)
    , tcp_port_(tcp_port)
    , udp_port_(udp_port)
    , listen_on_all_interfaces_(listen_on_all_interfaces)
    , running_(false)
    , next_player_id_(1)
    , next_session_id_(1)
    , total_connections_(0)
{
    if (!admin_password.empty()) {
        std::string password_hash = hash_password(admin_password);
        admin_manager_ = std::make_unique<AdminManager>(this, password_hash);
        std::cout << "[Server] Admin system enabled\n";
    }
}

Server::~Server()
{
    stop();
}

bool Server::start()
{
    std::cout << "[Server] Starting hybrid server - TCP:" << tcp_port_
              << " UDP:" << udp_port_ << " (listening on "
              << (listen_on_all_interfaces_ ? "all interfaces (0.0.0.0)" : "localhost (127.0.0.1)")
              << ")...\n";
    try {
        network_plugin_ = plugin_manager_.load_plugin<engine::INetworkPlugin>(
            engine::PluginPaths::get_plugin_path(engine::PluginPaths::ASIO_NETWORK),
            "create_network_plugin");
        if (!network_plugin_) {
            std::cerr << "[Server] Failed to load network plugin\n";
            return false;
        }
        std::cout << "[Server] Network plugin loaded successfully\n";
    } catch (const std::exception& e) {
        std::cerr << "[Server] Exception loading plugin: " << e.what() << "\n";
        return false;
    }
    if (!network_plugin_->start_server(tcp_port_, udp_port_, listen_on_all_interfaces_)) {
        std::cerr << "[Server] Failed to start hybrid server\n";
        return false;
    }
    network_handler_ = std::make_unique<NetworkHandler>(network_plugin_);
    packet_sender_ = std::make_unique<PacketSender>(network_plugin_);
    session_manager_ = std::make_unique<GameSessionManager>();
    global_leaderboard_manager_ = std::make_unique<GlobalLeaderboardManager>("data/global_leaderboard.json");
    global_leaderboard_manager_->load();
    network_handler_->set_listener(this);
    lobby_manager_.set_listener(this);
    room_manager_.set_listener(this);
    session_manager_->set_listener(this);
    network_plugin_->set_on_client_disconnected([this](uint32_t client_id) {
        on_tcp_client_disconnected(client_id);
    });

    server_start_time_ = std::chrono::steady_clock::now();

    running_ = true;
    std::cout << "[Server] Server started successfully\n";
    std::cout << "[Server] TCP port " << tcp_port_ << " - Connections, Lobby\n";
    std::cout << "[Server] UDP port " << udp_port_ << " - Gameplay\n";
    if (admin_manager_) {
        std::cout << "[Server] Admin interface enabled\n";
    }
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
        network_handler_->process_packets();
        lobby_manager_.update();
        room_manager_.update();
        session_manager_->update_all(delta_time);
        broadcast_all_session_events();
        session_manager_->cleanup_inactive_sessions();
        auto tick_end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tick_end - tick_start);
        if (elapsed < tick_duration)
            std::this_thread::sleep_for(tick_duration - elapsed);
    }
}

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
    accept.assigned_player_id = ByteOrder::host_to_net32(player_id);
    accept.server_tick_rate = config::SERVER_TICK_RATE;
    accept.max_players = 4;
    accept.map_id = ByteOrder::host_to_net16(0);
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
    pong.server_timestamp = ByteOrder::host_to_net32(static_cast<uint32_t>(
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
    uint32_t player_id = ByteOrder::net_to_host32(payload.player_id);
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
    uint32_t player_id = ByteOrder::net_to_host32(payload.player_id);
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
    uint32_t player_id = ByteOrder::net_to_host32(payload.player_id);
    uint32_t session_id = ByteOrder::net_to_host32(payload.session_id);

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
    uint32_t player_id, session_id;
    {
        std::lock_guard lock(connected_clients_mutex_);
        auto it = connected_clients_.find(tcp_client_id);
        if (it == connected_clients_.end())
            return;
        if (!it->second.in_game)
            return;
        player_id = it->second.player_id;
        session_id = it->second.session_id;
    }
    auto* session = session_manager_->get_session(session_id);
    if (!session)
        return;
    session->handle_input(player_id, payload);
}

void Server::on_lobby_state_changed(uint32_t lobby_id, const std::vector<uint8_t>& payload)
{
    std::cout << "[Server] Broadcasting lobby state for lobby/room " << lobby_id << " (payload size: " << payload.size() << ")\n";

    // If payload is empty (from RoomManager), build it ourselves with player info
    std::vector<uint8_t> actual_payload = payload;
    if (actual_payload.empty()) {
        const auto* room = room_manager_.get_room(lobby_id);
        if (room) {
            // Build room state with player info
            protocol::ServerLobbyStatePayload header;
            header.lobby_id = ByteOrder::host_to_net32(lobby_id);
            header.game_mode = protocol::GameMode::DUO;
            header.difficulty = protocol::Difficulty::NORMAL;
            header.current_player_count = static_cast<uint8_t>(room->player_ids.size());
            header.required_player_count = room->max_players;

            // Start with header
            const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&header);
            actual_payload.assign(header_bytes, header_bytes + sizeof(header));

            // Add each player entry
            for (uint32_t player_id : room->player_ids) {
                protocol::PlayerLobbyEntry entry;
                entry.player_id = ByteOrder::host_to_net32(player_id);

                // Get player info from connected_clients_
                auto client_it = player_to_client_.find(player_id);
                if (client_it != player_to_client_.end()) {
                    auto info_it = connected_clients_.find(client_it->second);
                    if (info_it != connected_clients_.end()) {
                        entry.set_name(info_it->second.player_name);
                        entry.skin_id = info_it->second.skin_id;
                        std::cout << "[Server] Adding player " << player_id << " to lobby state: name='"
                                  << info_it->second.player_name << "' skin_id=" << static_cast<int>(info_it->second.skin_id) << "\n";
                    } else {
                        entry.set_name("Player " + std::to_string(player_id));
                        entry.skin_id = 0;
                        std::cout << "[Server] Adding player " << player_id << " to lobby state: (not found in connected_clients)\n";
                    }
                } else {
                    entry.set_name("Player " + std::to_string(player_id));
                    entry.skin_id = 0;
                    std::cout << "[Server] Adding player " << player_id << " to lobby state: (not found in player_to_client)\n";
                }
                entry.player_level = 0;

                const uint8_t* entry_bytes = reinterpret_cast<const uint8_t*>(&entry);
                actual_payload.insert(actual_payload.end(), entry_bytes, entry_bytes + sizeof(entry));
            }
        }
    }

    if (!actual_payload.empty()) {
        // Broadcast to room members
        const auto* room = room_manager_.get_room(lobby_id);
        if (room) {
            for (uint32_t player_id : room->player_ids) {
                auto client_it = player_to_client_.find(player_id);
                if (client_it != player_to_client_.end()) {
                    packet_sender_->send_tcp_packet(client_it->second,
                                                   protocol::PacketType::SERVER_LOBBY_STATE,
                                                   actual_payload);
                }
            }
        } else {
            // Fallback to lobby broadcast
            packet_sender_->broadcast_tcp_to_lobby(lobby_id, protocol::PacketType::SERVER_LOBBY_STATE,
                                                  actual_payload, lobby_manager_, connected_clients_);
        }
    }
}

void Server::on_countdown_tick(uint32_t lobby_id, uint8_t seconds_remaining)
{
    protocol::ServerGameStartCountdownPayload countdown;
    countdown.lobby_id = ByteOrder::host_to_net32(lobby_id);
    countdown.countdown_value = seconds_remaining;

    std::cout << "[Server] Countdown tick for lobby/room " << lobby_id << ": "
              << static_cast<int>(seconds_remaining) << "s\n";
    const auto* room = room_manager_.get_room(lobby_id);
    if (room) {
        for (uint32_t player_id : room->player_ids) {
            auto client_it = player_to_client_.find(player_id);
            if (client_it != player_to_client_.end()) {
                packet_sender_->send_tcp_packet(client_it->second,
                                               protocol::PacketType::SERVER_GAME_START_COUNTDOWN,
                                               serialize(countdown));
            }
        }
    } else {
        packet_sender_->broadcast_tcp_to_lobby(lobby_id, protocol::PacketType::SERVER_GAME_START_COUNTDOWN,
                                              serialize(countdown), lobby_manager_, connected_clients_);
    }
}

void Server::on_game_start(uint32_t lobby_id, const std::vector<uint32_t>& player_ids)
{
    std::cout << "[Server] Game starting for lobby/room " << lobby_id
              << " with " << player_ids.size() << " players\n";
    const auto* room = room_manager_.get_room(lobby_id);
    const auto* lobby = room ? nullptr : lobby_manager_.get_lobby(lobby_id);

    if (!room && !lobby) {
        std::cerr << "[Server] Cannot start game - lobby/room " << lobby_id << " not found\n";
        return;
    }
    protocol::GameMode game_mode = room ? room->game_mode : lobby->game_mode;
    protocol::Difficulty difficulty = room ? room->difficulty : lobby->difficulty;
    uint16_t map_id = room ? room->map_id : 1;
    uint32_t session_id = generate_session_id();
    auto* session = session_manager_->create_session(session_id, game_mode, difficulty, map_id);
    for (uint32_t player_id : player_ids) {
        auto client_it = player_to_client_.find(player_id);
        if (client_it == player_to_client_.end())
            continue;
        uint32_t client_id = client_it->second;
        auto& player_info = connected_clients_[client_id];

        // Remove player from any previous session before adding to new one
        uint32_t old_session_id = player_info.session_id;
        if (old_session_id != 0) {
            auto* old_session = session_manager_->get_session(old_session_id);
            if (old_session) {
                std::cout << "[Server] Removing player " << player_id << " from old session " << old_session_id << "\n";
                old_session->remove_player(player_id);
            }
        }

        player_info.in_lobby = false;
        player_info.lobby_id = 0;
        player_info.in_game = true;
        player_info.session_id = session_id;
        session->add_player(player_id, player_info.player_name, player_info.skin_id);
        protocol::ServerGameStartPayload game_start;
        game_start.game_session_id = ByteOrder::host_to_net32(session_id);
        game_start.game_mode = game_mode;
        game_start.difficulty = difficulty;
        game_start.server_tick = ByteOrder::host_to_net32(0);
        game_start.level_seed = ByteOrder::host_to_net32(session->get_map_seed());
        game_start.udp_port = ByteOrder::host_to_net16(udp_port_);
        game_start.map_id = ByteOrder::host_to_net16(map_id);
        game_start.scroll_speed = session->get_scroll_speed();
        packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_GAME_START,
                                       serialize(game_start));
    }

    // Broadcast player names to all players in the session to ensure proper HUD display
    for (uint32_t target_pid : player_ids) {
        auto target_client_it = player_to_client_.find(target_pid);
        if (target_client_it == player_to_client_.end()) continue;
        uint32_t target_client_id = target_client_it->second;

        for (uint32_t source_pid : player_ids) {
            auto source_client_it = player_to_client_.find(source_pid);
            if (source_client_it == player_to_client_.end()) continue;
            uint32_t source_client_id = source_client_it->second;
            
            protocol::ServerPlayerNameUpdatedPayload update;
            update.player_id = ByteOrder::host_to_net32(source_pid);
            update.set_name(connected_clients_[source_client_id].player_name);
            update.room_id = ByteOrder::host_to_net32(lobby_id);

            packet_sender_->send_tcp_packet(target_client_id, 
                                           protocol::PacketType::SERVER_PLAYER_NAME_UPDATED, 
                                           serialize(update));
        }
    }
    if (room)
        room_manager_.leave_room(player_ids[0]);
    std::cout << "[Server] GameSession " << session_id << " created\n";
}

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
    destroy.entity_id = ByteOrder::host_to_net32(entity_id);
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

void Server::on_explosion(uint32_t session_id, const std::vector<uint8_t>& explosion_data)
{
    auto* session = session_manager_->get_session(session_id);

    if (!session)
        return;
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_EXPLOSION_EVENT,
                                            explosion_data, session->get_player_ids(), connected_clients_);
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

void Server::on_score_update(uint32_t session_id, const std::vector<uint8_t>& score_data)
{
    auto* session = session_manager_->get_session(session_id);

    if (!session)
        return;
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_SCORE_UPDATE,
                                            score_data, session->get_player_ids(), connected_clients_);
}

void Server::on_powerup_collected(uint32_t session_id, const std::vector<uint8_t>& powerup_data)
{
    auto* session = session_manager_->get_session(session_id);

    if (!session)
        return;
    std::cout << "[Server] Broadcasting powerup collected to session " << session_id << std::endl;
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_POWERUP_COLLECTED,
                                            powerup_data, session->get_player_ids(), connected_clients_);
}

void Server::on_player_respawn(uint32_t session_id, const std::vector<uint8_t>& respawn_data)
{
    auto* session = session_manager_->get_session(session_id);

    if (!session)
        return;
    std::cout << "[Server] Broadcasting player respawn to session " << session_id << std::endl;
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_PLAYER_RESPAWN,
                                            respawn_data, session->get_player_ids(), connected_clients_);
}

void Server::on_player_level_up(uint32_t session_id, const std::vector<uint8_t>& level_up_data)
{
    auto* session = session_manager_->get_session(session_id);

    if (!session)
        return;
    std::cout << "[Server] Broadcasting player level-up to session " << session_id << std::endl;
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_PLAYER_LEVEL_UP,
                                            level_up_data, session->get_player_ids(), connected_clients_);
}

void Server::on_leaderboard(uint32_t session_id, const std::vector<uint8_t>& leaderboard_data)
{
    auto* session = session_manager_->get_session(session_id);

    if (!session)
        return;
    std::cout << "[Server] Broadcasting leaderboard to session " << session_id << std::endl;
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_LEADERBOARD,
                                            leaderboard_data, session->get_player_ids(), connected_clients_);
}

void Server::on_game_over(uint32_t session_id, const std::vector<uint32_t>& player_ids, bool is_victory)
{
    std::cout << "[Server] Game over for session " << session_id
              << (is_victory ? " - VICTORY!" : " - DEFEAT!") << "\n";

    // Add player scores to global leaderboard
    auto* session = session_manager_->get_session(session_id);
    if (session && global_leaderboard_manager_) {
        auto player_scores = session->get_player_scores();
        for (const auto& [name, score] : player_scores) {
            global_leaderboard_manager_->try_add_score(name, score);
        }
    }

    protocol::ServerGameOverPayload game_over;
    game_over.result = is_victory ? protocol::GameResult::VICTORY : protocol::GameResult::DEFEAT;

    // Send message using player_ids parameter directly (session might be deleted by cleanup)
    packet_sender_->broadcast_udp_to_session(session_id, protocol::PacketType::SERVER_GAME_OVER,
                                            serialize(game_over), player_ids,
                                            connected_clients_);

    for (uint32_t player_id : player_ids) {
        auto client_it = player_to_client_.find(player_id);
        if (client_it == player_to_client_.end())
            continue;
        auto& player_info = connected_clients_[client_it->second];
        player_info.in_game = false;
        player_info.session_id = 0;
    }
    for (auto& [tcp_client_id, player_info] : connected_clients_) {
        if (player_info.session_id == session_id) {
            player_info.in_game = false;
            player_info.session_id = 0;
        }
    }
}

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

void Server::on_client_create_room(uint32_t client_id, const protocol::ClientCreateRoomPayload& payload)
{
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end()) {
        std::cerr << "[Server] CREATE_ROOM from unknown client " << client_id << "\n";
        return;
    }
    uint32_t player_id = ByteOrder::net_to_host32(payload.player_id);
    if (player_id != it->second.player_id) {
        std::cerr << "[Server] Player ID mismatch in CREATE_ROOM\n";
        return;
    }
    if (lobby_manager_.get_player_lobby(player_id) != 0) {
        std::cout << "[Server] Player " << player_id << " already in matchmaking lobby\n";
        protocol::ServerRoomErrorPayload error;
        error.error_code = protocol::RoomError::ALREADY_IN_ROOM;
        error.set_message("Already in matchmaking lobby");
        packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_ROOM_ERROR,
                                        serialize(error));
        return;
    }
    if (room_manager_.get_player_room(player_id) != 0) {
        std::cout << "[Server] Player " << player_id << " already in a room\n";
        protocol::ServerRoomErrorPayload error;
        error.error_code = protocol::RoomError::ALREADY_IN_ROOM;
        error.set_message("Already in a room");
        packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_ROOM_ERROR,
                                        serialize(error));
        return;
    }
    std::string room_name(payload.room_name, strnlen(payload.room_name, sizeof(payload.room_name)));
    std::string password_hash(payload.password_hash, strnlen(payload.password_hash, sizeof(payload.password_hash)));
    protocol::GameMode game_mode = static_cast<protocol::GameMode>(payload.game_mode);
    protocol::Difficulty difficulty = static_cast<protocol::Difficulty>(payload.difficulty);
    uint16_t map_id = ByteOrder::net_to_host16(payload.map_id);
    uint8_t max_players = payload.max_players;
    std::cout << "[Server] Player " << player_id << " (" << it->second.player_name
              << ") creating room: '" << (room_name.empty() ? "unnamed" : room_name)
              << "' (mode: " << protocol::game_mode_to_string(game_mode)
              << ", difficulty: " << protocol::difficulty_to_string(difficulty)
              << ", max_players: " << static_cast<int>(max_players)
              << ", private: " << (!password_hash.empty() ? "yes" : "no") << ")\n";
    uint32_t room_id = room_manager_.create_room(
        player_id, room_name, password_hash,
        game_mode, difficulty, map_id, max_players
    );
    if (room_id == 0) {
        std::cerr << "[Server] Failed to create room for player " << player_id << "\n";
        protocol::ServerRoomErrorPayload error;
        error.error_code = protocol::RoomError::INVALID_CONFIGURATION;
        error.set_message("Failed to create room");
        packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_ROOM_ERROR,
                                        serialize(error));
        return;
    }
    const auto* room = room_manager_.get_room(room_id);
    if (room) {
        protocol::ServerRoomCreatedPayload response;
        response.room_id = ByteOrder::host_to_net32(room_id);
        response.set_room_name(room->room_name);
        packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_ROOM_CREATED,
                                        serialize(response));
        std::cout << "[Server] Room " << room_id << " created successfully\n";
    }
}

void Server::on_client_join_room(uint32_t client_id, const protocol::ClientJoinRoomPayload& payload)
{
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end()) {
        std::cerr << "[Server] JOIN_ROOM from unknown client " << client_id << "\n";
        return;
    }
    uint32_t player_id = ByteOrder::net_to_host32(payload.player_id);
    if (player_id != it->second.player_id) {
        std::cerr << "[Server] Player ID mismatch in JOIN_ROOM\n";
        return;
    }
    uint32_t room_id = ByteOrder::net_to_host32(payload.room_id);
    std::string password_hash(payload.password_hash, strnlen(payload.password_hash, sizeof(payload.password_hash)));
    if (lobby_manager_.get_player_lobby(player_id) != 0 ||
        room_manager_.get_player_room(player_id) != 0) {
        protocol::ServerRoomErrorPayload error;
        error.error_code = protocol::RoomError::ALREADY_IN_ROOM;
        error.set_message("Already in a lobby or room");
        packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_ROOM_ERROR,
                                        serialize(error));
        return;
    }
    std::cout << "[Server] Player " << player_id << " (" << it->second.player_name
              << ") attempting to join room " << room_id << "\n";
    bool joined = room_manager_.join_room(player_id, room_id, password_hash);
    if (!joined) {
        const auto* room = room_manager_.get_room(room_id);
        protocol::RoomError error_code;
        std::string error_msg;
        if (!room) {
            error_code = protocol::RoomError::ROOM_NOT_FOUND;
            error_msg = "Room not found";
        } else if (room->is_full()) {
            error_code = protocol::RoomError::ROOM_FULL;
            error_msg = "Room is full";
        } else if (room->status != protocol::RoomStatus::WAITING) {
            error_code = protocol::RoomError::ALREADY_STARTED;
            error_msg = "Game already started";
        } else if (room->is_private() && room->password_hash != password_hash) {
            error_code = protocol::RoomError::WRONG_PASSWORD;
            error_msg = "Wrong password";
        } else {
            error_code = protocol::RoomError::ROOM_NOT_FOUND;
            error_msg = "Cannot join room";
        }
        std::cout << "[Server] Player " << player_id << " failed to join room: " << error_msg << "\n";
        protocol::ServerRoomErrorPayload error;
        error.error_code = error_code;
        error.set_message(error_msg);
        packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_ROOM_ERROR,
                                        serialize(error));
        return;
    }
    protocol::ServerRoomJoinedPayload response;
    response.room_id = ByteOrder::host_to_net32(room_id);
    packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_ROOM_JOINED,
                                        serialize(response));
    std::cout << "[Server] Player " << player_id << " joined room " << room_id << " successfully\n";
}

void Server::on_client_leave_room(uint32_t client_id, const protocol::ClientLeaveRoomPayload& payload)
{
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end()) {
        std::cerr << "[Server] LEAVE_ROOM from unknown client " << client_id << "\n";
        return;
    }
    uint32_t player_id = ByteOrder::net_to_host32(payload.player_id);
    if (player_id != it->second.player_id) {
        std::cerr << "[Server] Player ID mismatch in LEAVE_ROOM\n";
        return;
    }
    std::cout << "[Server] Player " << player_id << " (" << it->second.player_name << ") leaving room\n";
    bool left = room_manager_.leave_room(player_id);
    if (!left) {
        std::cout << "[Server] Player " << player_id << " was not in any room\n";
    } else {
        std::cout << "[Server] Player " << player_id << " left room successfully\n";
    }
}

void Server::on_client_request_room_list(uint32_t client_id)
{
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end()) {
        std::cerr << "[Server] REQUEST_ROOM_LIST from unknown client " << client_id << "\n";
        return;
    }
    std::cout << "[Server] Player " << it->second.player_id << " requesting room list\n";
    auto rooms = room_manager_.get_public_rooms();
    std::vector<uint8_t> payload_data;
    protocol::ServerRoomListPayload header;
    header.room_count = ByteOrder::host_to_net16(static_cast<uint16_t>(rooms.size()));
    payload_data.insert(payload_data.end(),
                        reinterpret_cast<const uint8_t*>(&header),
                        reinterpret_cast<const uint8_t*>(&header) + sizeof(header));
    for (auto& room_info : rooms) {
        room_info.room_id = ByteOrder::host_to_net32(room_info.room_id);
        room_info.map_id = ByteOrder::host_to_net16(room_info.map_id);
        payload_data.insert(payload_data.end(),
                            reinterpret_cast<const uint8_t*>(&room_info),
                            reinterpret_cast<const uint8_t*>(&room_info) + sizeof(room_info));
    }
    packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_ROOM_LIST,
                                        payload_data);
    std::cout << "[Server] Sent room list with " << rooms.size() << " rooms to player "
              << it->second.player_id << "\n";
}

void Server::on_client_start_game(uint32_t client_id, const protocol::ClientStartGamePayload& payload)
{
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end()) {
        std::cerr << "[Server] START_GAME from unknown client " << client_id << "\n";
        return;
    }
    uint32_t player_id = ByteOrder::net_to_host32(payload.player_id);
    if (player_id != it->second.player_id) {
        std::cerr << "[Server] Player ID mismatch in START_GAME\n";
        return;
    }
    uint32_t room_id = ByteOrder::net_to_host32(payload.room_id);
    std::cout << "[Server] Player " << player_id << " requesting to start game in room " << room_id << "\n";
    bool started = room_manager_.start_game(room_id, player_id);
    if (!started) {
        const auto* room = room_manager_.get_room(room_id);
        protocol::RoomError error_code;
        std::string error_msg;
        if (!room) {
            error_code = protocol::RoomError::ROOM_NOT_FOUND;
            error_msg = "Room not found";
        } else if (!room->is_host(player_id)) {
            error_code = protocol::RoomError::NOT_HOST;
            error_msg = "Only the host can start the game";
        } else if (room->status != protocol::RoomStatus::WAITING) {
            error_code = protocol::RoomError::ALREADY_STARTED;
            error_msg = "Game already started";
        } else {
            error_code = protocol::RoomError::INVALID_CONFIGURATION;
            error_msg = "Cannot start game";
        }
        std::cout << "[Server] Failed to start game: " << error_msg << "\n";
        protocol::ServerRoomErrorPayload error;
        error.error_code = error_code;
        error.set_message(error_msg);
        packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_ROOM_ERROR,
                                        serialize(error));
        return;
    }
    std::cout << "[Server] Game start countdown initiated for room " << room_id << "\n";
}

void Server::on_client_set_player_name(uint32_t client_id, const protocol::ClientSetPlayerNamePayload& payload)
{
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end()) {
        std::cerr << "[Server] SET_PLAYER_NAME from unknown client " << client_id << "\n";
        return;
    }
    uint32_t player_id = ByteOrder::net_to_host32(payload.player_id);
    if (player_id != it->second.player_id) {
        std::cerr << "[Server] Player ID mismatch in SET_PLAYER_NAME\n";
        return;
    }
    std::string new_name(payload.new_name, strnlen(payload.new_name, sizeof(payload.new_name)));
    std::string old_name = it->second.player_name;
    it->second.player_name = new_name;
    std::cout << "[Server] Player " << player_id << " changed name from '" << old_name
              << "' to '" << new_name << "'\n";

    // Broadcast name change to room members if player is in a room
    uint32_t room_id = room_manager_.get_player_room(player_id);
    if (room_id != 0) {
        protocol::ServerPlayerNameUpdatedPayload update;
        update.player_id = ByteOrder::host_to_net32(player_id);
        update.set_name(new_name);
        update.room_id = ByteOrder::host_to_net32(room_id);

        auto room_players = room_manager_.get_room_players(room_id);
        for (uint32_t room_player_id : room_players) {
            auto room_client_it = player_to_client_.find(room_player_id);
            if (room_client_it != player_to_client_.end()) {
                packet_sender_->send_tcp_packet(room_client_it->second,
                                               protocol::PacketType::SERVER_PLAYER_NAME_UPDATED,
                                               serialize(update));
            }
        }
    }
}

void Server::on_client_set_player_skin(uint32_t client_id, const protocol::ClientSetPlayerSkinPayload& payload)
{
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end()) {
        std::cerr << "[Server] SET_PLAYER_SKIN from unknown client " << client_id << "\n";
        return;
    }
    uint32_t player_id = ByteOrder::net_to_host32(payload.player_id);
    if (player_id != it->second.player_id) {
        std::cerr << "[Server] Player ID mismatch in SET_PLAYER_SKIN\n";
        return;
    }
    // Validate skin_id (0-14 for 3 colors x 5 ship types)
    uint8_t skin_id = payload.skin_id;
    if (skin_id > 14) {
        std::cerr << "[Server] Invalid skin_id " << static_cast<int>(skin_id) << ", clamping to 14\n";
        skin_id = 14;
    }
    uint8_t old_skin = it->second.skin_id;
    it->second.skin_id = skin_id;
    std::cout << "[Server] Player " << player_id << " changed skin from " << static_cast<int>(old_skin)
              << " to " << static_cast<int>(skin_id) << "\n";

    // Broadcast skin change to room members if player is in a room
    uint32_t room_id = room_manager_.get_player_room(player_id);
    if (room_id != 0) {
        protocol::ServerPlayerSkinUpdatedPayload update;
        update.player_id = ByteOrder::host_to_net32(player_id);
        update.skin_id = skin_id;
        update.room_id = ByteOrder::host_to_net32(room_id);

        auto room_players = room_manager_.get_room_players(room_id);
        for (uint32_t room_player_id : room_players) {
            auto room_client_it = player_to_client_.find(room_player_id);
            if (room_client_it != player_to_client_.end()) {
                packet_sender_->send_tcp_packet(room_client_it->second,
                                               protocol::PacketType::SERVER_PLAYER_SKIN_UPDATED,
                                               serialize(update));
            }
        }
    }
}

void Server::broadcast_all_session_events()
{
    auto session_ids = session_manager_->get_active_session_ids();

    for (uint32_t session_id : session_ids) {
        auto* session = session_manager_->get_session(session_id);
        if (!session)
            continue;
        auto* net_system = session->get_network_system();
        if (!net_system)
            continue;
        auto spawns = net_system->drain_pending_spawns();
        while (!spawns.empty()) {
            auto& payload = spawns.front();
            std::vector<uint8_t> data(reinterpret_cast<const uint8_t*>(&payload),
                                      reinterpret_cast<const uint8_t*>(&payload) + sizeof(payload));
            packet_sender_->broadcast_udp_to_session(session_id,
                protocol::PacketType::SERVER_ENTITY_SPAWN,
                data, session->get_player_ids(), connected_clients_);
            spawns.pop();
        }
        auto destroys = net_system->drain_pending_destroys();
        while (!destroys.empty()) {
            uint32_t entity_id = destroys.front();
            protocol::ServerEntityDestroyPayload destroy;
            destroy.entity_id = ByteOrder::host_to_net32(entity_id);
            destroy.reason = protocol::DestroyReason::KILLED;
            destroy.position_x = 0.0f;
            destroy.position_y = 0.0f;
            packet_sender_->broadcast_udp_to_session(session_id,
                protocol::PacketType::SERVER_ENTITY_DESTROY,
                serialize(destroy), session->get_player_ids(), connected_clients_);
            destroys.pop();
        }
        auto projectiles = net_system->drain_pending_projectiles();
        while (!projectiles.empty()) {
            auto& payload = projectiles.front();
            std::vector<uint8_t> data(reinterpret_cast<const uint8_t*>(&payload),
                                      reinterpret_cast<const uint8_t*>(&payload) + sizeof(payload));
            packet_sender_->broadcast_udp_to_session(session_id,
                protocol::PacketType::SERVER_PROJECTILE_SPAWN,
                data, session->get_player_ids(), connected_clients_);
            projectiles.pop();
        }
        auto explosions = net_system->drain_pending_explosions();
        while (!explosions.empty()) {
            auto& payload = explosions.front();
            std::vector<uint8_t> data(reinterpret_cast<const uint8_t*>(&payload),
                                      reinterpret_cast<const uint8_t*>(&payload) + sizeof(payload));
            packet_sender_->broadcast_udp_to_session(session_id,
                protocol::PacketType::SERVER_EXPLOSION_EVENT,
                data, session->get_player_ids(), connected_clients_);
            explosions.pop();
        }
        auto scores = net_system->drain_pending_scores();
        while (!scores.empty()) {
            auto& payload = scores.front();
            std::vector<uint8_t> data(reinterpret_cast<const uint8_t*>(&payload),
                                      reinterpret_cast<const uint8_t*>(&payload) + sizeof(payload));
            packet_sender_->broadcast_udp_to_session(session_id,
                protocol::PacketType::SERVER_SCORE_UPDATE,
                data, session->get_player_ids(), connected_clients_);
            scores.pop();
        }
    }
}

std::vector<Server::AdminPlayerInfo> Server::get_connected_players() const
{
    std::lock_guard lock(connected_clients_mutex_);
    std::vector<AdminPlayerInfo> result;

    for (const auto& [client_id, info] : connected_clients_) {
        result.push_back({
            info.player_id,
            info.client_id,
            info.player_name,
            info.in_game,
            info.session_id
        });
    }
    return result;
}

bool Server::kick_player(uint32_t player_id, const std::string& reason)
{
    std::lock_guard lock(connected_clients_mutex_);

    auto it = player_to_client_.find(player_id);
    if (it == player_to_client_.end())
        return false;

    uint32_t client_id = it->second;
    std::cout << "[Server] Kicking player " << player_id << " (client " << client_id << "): " << reason << "\n";
    protocol::ServerKickNotificationPayload kick_notification;
    kick_notification.set_reason(reason);
    packet_sender_->send_tcp_packet(client_id,
                                   protocol::PacketType::SERVER_KICK_NOTIFICATION,
                                   serialize(kick_notification));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    network_plugin_->disconnect_client(client_id);
    return true;
}

Server::ServerStats Server::get_server_stats() const
{
    auto now = std::chrono::steady_clock::now();
    auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
        now - server_start_time_
    ).count();
    std::lock_guard lock(connected_clients_mutex_);
    auto session_ids = session_manager_->get_active_session_ids();

    return {
        static_cast<uint32_t>(uptime),
        static_cast<uint32_t>(connected_clients_.size()),
        static_cast<uint32_t>(session_ids.size()),
        total_connections_
    };
}

uint32_t Server::pause_all_sessions()
{
    uint32_t count = 0;
    auto session_ids = session_manager_->get_active_session_ids();

    for (uint32_t session_id : session_ids) {
        auto* session = session_manager_->get_session(session_id);
        if (session) {
            session->pause();
            count++;
        }
    }
    std::cout << "[Server] Paused " << count << " game session(s)\n";
    return count;
}

uint32_t Server::resume_all_sessions()
{
    uint32_t count = 0;
    auto session_ids = session_manager_->get_active_session_ids();

    for (uint32_t session_id : session_ids) {
        auto* session = session_manager_->get_session(session_id);
        if (session) {
            session->resume();
            count++;
        }
    }
    std::cout << "[Server] Resumed " << count << " game session(s)\n";
    return count;
}

uint32_t Server::clear_enemies_all_sessions()
{
    uint32_t count = 0;
    auto session_ids = session_manager_->get_active_session_ids();

    for (uint32_t session_id : session_ids) {
        auto* session = session_manager_->get_session(session_id);
        if (session) {
            session->clear_enemies();
            count++;
        }
    }
    std::cout << "[Server] Cleared enemies from " << count << " session(s)\n";
    return count;
}

bool Server::clear_enemies_in_session(uint32_t session_id)
{
    auto* session = session_manager_->get_session(session_id);
    if (!session) {
        std::cout << "[Server] Session " << session_id << " not found\n";
        return false;
    }

    session->clear_enemies();
    std::cout << "[Server] Cleared enemies from session " << session_id << "\n";
    return true;
}

void Server::on_client_request_global_leaderboard(uint32_t client_id)
{
    auto it = connected_clients_.find(client_id);

    if (it == connected_clients_.end()) {
        std::cerr << "[Server] REQUEST_GLOBAL_LEADERBOARD from unknown client " << client_id << "\n";
        return;
    }

    std::cout << "[Server] Player " << it->second.player_id << " requesting global leaderboard\n";

    if (!global_leaderboard_manager_) {
        std::cerr << "[Server] Global leaderboard manager not initialized\n";
        return;
    }

    auto entries = global_leaderboard_manager_->get_entries();

    // Build response payload
    std::vector<uint8_t> payload_data;

    protocol::ServerGlobalLeaderboardPayload header;
    header.entry_count = static_cast<uint8_t>(std::min(entries.size(), static_cast<size_t>(255)));

    // Add header
    payload_data.insert(payload_data.end(),
                        reinterpret_cast<const uint8_t*>(&header),
                        reinterpret_cast<const uint8_t*>(&header) + sizeof(header));

    // Add entries (already in network byte order from GlobalLeaderboardEntry)
    for (size_t i = 0; i < header.entry_count; ++i) {
        protocol::GlobalLeaderboardEntry entry = entries[i];
        // Convert to network byte order
        entry.score = ByteOrder::host_to_net32(entry.score);
        entry.timestamp = ByteOrder::host_to_net32(entry.timestamp);

        payload_data.insert(payload_data.end(),
                            reinterpret_cast<const uint8_t*>(&entry),
                            reinterpret_cast<const uint8_t*>(&entry) + sizeof(entry));
    }

    packet_sender_->send_tcp_packet(client_id, protocol::PacketType::SERVER_GLOBAL_LEADERBOARD,
                                    payload_data);

    std::cout << "[Server] Sent global leaderboard with " << static_cast<int>(header.entry_count)
              << " entries to player " << it->second.player_id << "\n";
}

void Server::on_admin_auth(uint32_t client_id,
                           const protocol::ClientAdminAuthPayload& payload)
{
    std::string password_hash(payload.password_hash,
                             strnlen(payload.password_hash, sizeof(payload.password_hash)));
    std::string username(payload.username,
                        strnlen(payload.username, sizeof(payload.username)));
    protocol::ServerAdminAuthResultPayload response;

    if (!admin_manager_) {
        response.success = 0;
        response.set_failure_reason("Admin system not enabled on server");
    } else if (!admin_manager_->verify_password(password_hash)) {
        response.success = 0;
        response.set_failure_reason("Invalid password");
        std::cout << "[Server] Failed admin auth from client " << client_id << "\n";
    } else {
        response.success = 1;
        response.admin_level = ByteOrder::host_to_net32(1);
        std::lock_guard lock(connected_clients_mutex_);
        auto it = connected_clients_.find(client_id);
        if (it != connected_clients_.end()) {
            it->second.is_admin = true;
            it->second.admin_username = username;
        }
        std::cout << "[Server] Admin authenticated: " << username << "\n";
    }
    packet_sender_->send_tcp_packet(client_id,
                                   protocol::PacketType::SERVER_ADMIN_AUTH_RESULT,
                                   serialize(response));
}

void Server::on_admin_command(uint32_t client_id,
                              const protocol::ClientAdminCommandPayload& payload)
{
    uint32_t admin_id = ByteOrder::net_to_host32(payload.admin_id);
    std::string command = payload.get_command();
    bool is_admin = false;

    {
        std::lock_guard lock(connected_clients_mutex_);
        auto it = connected_clients_.find(client_id);
        if (it != connected_clients_.end())
            is_admin = it->second.is_admin;
    }
    protocol::ServerAdminCommandResultPayload response;
    if (!is_admin) {
        response.success = 0;
        response.set_message("Not authenticated as admin");
    } else if (!admin_manager_) {
        response.success = 0;
        response.set_message("Admin system not enabled");
    } else {
        auto result = admin_manager_->execute_command(admin_id, command);
        response.success = result.success ? 1 : 0;
        response.set_message(result.message);
        std::cout << "[Server] Admin command: " << command
                  << " -> " << (result.success ? "OK" : "FAILED") << "\n";
    }
    packet_sender_->send_tcp_packet(client_id,
                                   protocol::PacketType::SERVER_ADMIN_COMMAND_RESULT,
                                   serialize(response));
}

}
