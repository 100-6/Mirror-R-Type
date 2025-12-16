#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <atomic>

#include "plugin_manager/INetworkPlugin.hpp"
#include "plugin_manager/PluginManager.hpp"
#include "PlayerInfo.hpp"
#include "LobbyManager.hpp"
#include "GameSession.hpp"
#include "ServerConfig.hpp"
#include "protocol/PacketHeader.hpp"
#include "protocol/PacketTypes.hpp"
#include "protocol/Payloads.hpp"

namespace rtype::server {

/**
 * @brief Main server class - handles network communication and player management
 *
 * This is the entry point for the R-Type server. It manages:
 * - Network plugin loading and lifecycle
 * - Player connections and disconnections via TCP
 * - Gameplay communication via UDP
 * - Packet receiving and routing
 * - Lobby management
 * - Game sessions
 *
 * Network Architecture:
 * - TCP (port DEFAULT_TCP_PORT): Connection, lobby, authentication
 * - UDP (port DEFAULT_UDP_PORT): Gameplay, inputs, snapshots
 */
class Server {
public:
    /**
     * @brief Construct a new Server
     * @param tcp_port TCP port for connections/lobby (default: config::DEFAULT_TCP_PORT)
     * @param udp_port UDP port for gameplay (default: config::DEFAULT_UDP_PORT)
     */
    explicit Server(uint16_t tcp_port = config::DEFAULT_TCP_PORT, uint16_t udp_port = config::DEFAULT_UDP_PORT);

    /**
     * @brief Destructor - stops server and cleans up resources
     */
    ~Server();

    /**
     * @brief Start the server
     * @return true if server started successfully, false otherwise
     */
    bool start();

    /**
     * @brief Stop the server gracefully
     */
    void stop();

    /**
     * @brief Main server loop - runs until stop() is called
     *
     * This loop:
     * - Processes incoming packets
     * - Updates lobbies and game sessions
     * - Sends periodic snapshots
     */
    void run();

    /**
     * @brief Check if server is currently running
     */
    bool is_running() const { return running_; }

private:
    engine::PluginManager plugin_manager_;
    engine::INetworkPlugin* network_plugin_;
    uint16_t tcp_port_;
    uint16_t udp_port_;
    std::atomic<bool> running_;

    std::unordered_map<uint32_t, PlayerInfo> connected_clients_;
    std::unordered_map<uint32_t, uint32_t> player_to_client_;  // player_id -> client_id (O(1) lookup)
    uint32_t next_player_id_;

    LobbyManager lobby_manager_;
    std::unordered_map<uint32_t, std::unique_ptr<GameSession>> game_sessions_;
    uint32_t next_session_id_;

    void handle_packets();
    void route_packet(uint32_t client_id, const protocol::PacketHeader& header,
                     const std::vector<uint8_t>& payload, engine::NetworkProtocol protocol);

    // TCP packet handlers (connection, lobby)
    void handle_client_connect(uint32_t client_id, const protocol::ClientConnectPayload& payload);
    void handle_client_disconnect(uint32_t client_id, const protocol::ClientDisconnectPayload& payload);
    void handle_client_ping(uint32_t client_id, const protocol::ClientPingPayload& payload);
    void handle_client_join_lobby(uint32_t client_id, const protocol::ClientJoinLobbyPayload& payload);
    void handle_client_leave_lobby(uint32_t client_id, const protocol::ClientLeaveLobbyPayload& payload);

    // UDP packet handlers (gameplay)
    void handle_udp_handshake(uint32_t udp_client_id, const protocol::ClientUdpHandshakePayload& payload);
    void handle_client_input(uint32_t client_id, const protocol::ClientInputPayload& payload);

    // TCP sending (reliable)
    void send_tcp_packet(uint32_t client_id, protocol::PacketType type, const std::vector<uint8_t>& payload);
    void broadcast_tcp_packet(protocol::PacketType type, const std::vector<uint8_t>& payload);
    void broadcast_tcp_to_lobby(uint32_t lobby_id, protocol::PacketType type, const std::vector<uint8_t>& payload);

    // UDP sending (fast)
    void send_udp_packet(uint32_t client_id, protocol::PacketType type, const std::vector<uint8_t>& payload);
    void broadcast_udp_to_session(uint32_t session_id, protocol::PacketType type, const std::vector<uint8_t>& payload);
    void broadcast_tcp_to_session(uint32_t session_id, protocol::PacketType type, const std::vector<uint8_t>& payload);

    void on_client_disconnected(uint32_t client_id);

    void on_lobby_state_changed(uint32_t lobby_id, const std::vector<uint8_t>& payload);
    void on_countdown_tick(uint32_t lobby_id, uint8_t seconds_remaining);
    void on_game_start(uint32_t lobby_id, const std::vector<uint32_t>& player_ids);

    void on_state_snapshot(uint32_t session_id, const std::vector<uint8_t>& snapshot);
    void on_entity_spawn(uint32_t session_id, const std::vector<uint8_t>& spawn_data);
    void on_entity_destroy(uint32_t session_id, uint32_t entity_id);
    void on_game_over(uint32_t session_id, const std::vector<uint32_t>& player_ids);

    void update_game_sessions(float delta_time);

    uint32_t generate_player_id();
    uint32_t generate_session_id();
};

}
