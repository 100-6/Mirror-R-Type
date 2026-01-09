/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Server - Main server class
*/

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <atomic>
#include <mutex>

#include "plugin_manager/INetworkPlugin.hpp"
#include "plugin_manager/PluginManager.hpp"
#include "PlayerInfo.hpp"
#include "LobbyManager.hpp"
#include "RoomManager.hpp"
#include "GameSessionManager.hpp"
#include "NetworkHandler.hpp"
#include "PacketSender.hpp"
#include "ServerConfig.hpp"
#include "protocol/PacketHeader.hpp"
#include "protocol/PacketTypes.hpp"
#include "protocol/Payloads.hpp"

#include "interfaces/INetworkListener.hpp"
#include "interfaces/ILobbyListener.hpp"
#include "interfaces/IGameSessionListener.hpp"

namespace rtype::server {

/**
 * @brief Main server class
 *
 * Implements all listener interfaces to handle events from:
 * - NetworkHandler (client connections, inputs)
 * - LobbyManager (lobby state, countdown, game start)
 * - GameSession (snapshots, spawns, destroys, game over)
 *
 * Simple architecture:
 * ```
 * Server (implements all listeners)
 *    ├── NetworkHandler → receives packets → calls Server
 *    ├── LobbyManager → handles matchmaking → calls Server
 *    ├── GameSessionManager → manages game sessions
 *    │       └── GameSession → game logic → calls Server
 *    └── PacketSender → sends packets to clients
 * ```
 */
class Server : public INetworkListener, public ILobbyListener, public IGameSessionListener {
public:
    explicit Server(uint16_t tcp_port = config::DEFAULT_TCP_PORT,
                    uint16_t udp_port = config::DEFAULT_UDP_PORT,
                    bool listen_on_all_interfaces = false);
    ~Server();

    bool start();
    void stop();
    void run();
    bool is_running() const { return running_; }

private:
    void on_client_connect(uint32_t client_id, const protocol::ClientConnectPayload& payload) override;
    void on_client_disconnect(uint32_t client_id, const protocol::ClientDisconnectPayload& payload) override;
    void on_client_ping(uint32_t client_id, const protocol::ClientPingPayload& payload) override;
    void on_client_join_lobby(uint32_t client_id, const protocol::ClientJoinLobbyPayload& payload) override;
    void on_client_leave_lobby(uint32_t client_id, const protocol::ClientLeaveLobbyPayload& payload) override;
    void on_udp_handshake(uint32_t udp_client_id, const protocol::ClientUdpHandshakePayload& payload) override;
    void on_client_input(uint32_t client_id, const protocol::ClientInputPayload& payload) override;

    void on_client_create_room(uint32_t client_id, const protocol::ClientCreateRoomPayload& payload) override;
    void on_client_join_room(uint32_t client_id, const protocol::ClientJoinRoomPayload& payload) override;
    void on_client_leave_room(uint32_t client_id, const protocol::ClientLeaveRoomPayload& payload) override;
    void on_client_request_room_list(uint32_t client_id) override;
    void on_client_start_game(uint32_t client_id, const protocol::ClientStartGamePayload& payload) override;
    void on_client_set_player_name(uint32_t client_id, const protocol::ClientSetPlayerNamePayload& payload) override;

    void on_lobby_state_changed(uint32_t lobby_id, const std::vector<uint8_t>& payload) override;
    void on_countdown_tick(uint32_t lobby_id, uint8_t seconds_remaining) override;
    void on_game_start(uint32_t lobby_id, const std::vector<uint32_t>& player_ids) override;

    void on_state_snapshot(uint32_t session_id, const std::vector<uint8_t>& snapshot) override;
    void on_entity_spawn(uint32_t session_id, const std::vector<uint8_t>& spawn_data) override;
    void on_entity_destroy(uint32_t session_id, uint32_t entity_id) override;
    void on_projectile_spawn(uint32_t session_id, const std::vector<uint8_t>& projectile_data) override;
    void on_explosion(uint32_t session_id, const std::vector<uint8_t>& explosion_data) override;
    void on_wave_start(uint32_t session_id, const std::vector<uint8_t>& wave_data) override;
    void on_wave_complete(uint32_t session_id, const std::vector<uint8_t>& wave_data) override;
    void on_game_over(uint32_t session_id, const std::vector<uint32_t>& player_ids, bool is_victory) override;
    void on_score_update(uint32_t session_id, const std::vector<uint8_t>& score_data) override;

    void on_tcp_client_disconnected(uint32_t client_id);
    uint32_t generate_player_id();
    uint32_t generate_session_id();

    /**
     * @brief Broadcast all queued session events after barrier
     *
     * This method is called after session_manager_->update_all() completes.
     * It drains all queued events from ServerNetworkSystem and broadcasts them.
     */
    void broadcast_all_session_events();

    engine::PluginManager plugin_manager_;
    engine::INetworkPlugin* network_plugin_;
    std::unique_ptr<NetworkHandler> network_handler_;
    std::unique_ptr<PacketSender> packet_sender_;
    std::unique_ptr<GameSessionManager> session_manager_;

    uint16_t tcp_port_;
    uint16_t udp_port_;
    bool listen_on_all_interfaces_;
    std::atomic<bool> running_;

    std::mutex connected_clients_mutex_;
    std::unordered_map<uint32_t, PlayerInfo> connected_clients_;
    std::unordered_map<uint32_t, uint32_t> player_to_client_;
    uint32_t next_player_id_;

    LobbyManager lobby_manager_;
    RoomManager room_manager_;
    uint32_t next_session_id_;
};

}
