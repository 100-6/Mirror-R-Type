#pragma once

#include <memory>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <chrono>
#include <random>
#include <string>
#include <vector>

#include "plugin_manager/INetworkPlugin.hpp"
#include "plugin_manager/PluginManager.hpp"

#include "BagarioConfig.hpp"
#include "PacketTypes.hpp"
#include "Payloads.hpp"

namespace bagario::server {

// Forward declarations
class BagarioSession;
class BagarioNetworkHandler;
class BagarioPacketSender;

/**
 * @brief Player information stored server-side
 */
struct PlayerInfo {
    uint32_t client_id = 0;
    uint32_t player_id = 0;
    std::string name;
    uint32_t color = 0xFFFFFFFF;
    bool in_game = false;
    std::chrono::steady_clock::time_point last_activity;
    std::vector<uint8_t> skin_data;  // Serialized skin for network sync
};

/**
 * @brief Main Bagario server class
 *
 * Architecture:
 * ```
 * BagarioServer
 *    ├── NetworkHandler → receives packets → calls Server
 *    ├── BagarioSession → game logic (ECS)
 *    └── PacketSender → sends packets to clients
 * ```
 *
 * Uses ENet plugin for networking (not ASIO like R-Type)
 */
class BagarioServer {
public:
    explicit BagarioServer(
        uint16_t tcp_port = config::DEFAULT_TCP_PORT,
        uint16_t udp_port = config::DEFAULT_UDP_PORT,
        bool listen_on_all_interfaces = true
    );
    ~BagarioServer();

    /**
     * @brief Start the server
     * @return true if server started successfully
     */
    bool start();

    /**
     * @brief Stop the server
     */
    void stop();

    /**
     * @brief Main server loop - call this in a loop
     */
    void run();

    /**
     * @brief Check if server is running
     */
    bool is_running() const { return m_running; }

    /**
     * @brief Get connected player count
     */
    size_t get_player_count() const;

private:
    // Network callbacks
    void on_client_connected(uint32_t client_id);
    void on_client_disconnected(uint32_t client_id);
    void on_packet_received(uint32_t client_id, const engine::NetworkPacket& packet);

    // Packet handlers
    void handle_client_connect(uint32_t client_id, const protocol::ClientConnectPayload& payload);
    void handle_client_disconnect(uint32_t client_id, const protocol::ClientDisconnectPayload& payload);
    void handle_client_ping(uint32_t client_id, const protocol::ClientPingPayload& payload);
    void handle_client_input(uint32_t client_id, const protocol::ClientInputPayload& payload);
    void handle_client_split(uint32_t client_id, const protocol::ClientSplitPayload& payload);
    void handle_client_eject_mass(uint32_t client_id, const protocol::ClientEjectMassPayload& payload);
    void handle_client_set_skin(uint32_t client_id, uint32_t player_id, const std::vector<uint8_t>& skin_data);

    // Helper methods
    uint32_t generate_player_id();
    uint32_t generate_color();
    void spawn_player_in_session(uint32_t player_id, const std::string& name, uint32_t color);
    void remove_player_from_session(uint32_t player_id);
    void broadcast_snapshot();
    void broadcast_leaderboard();

    // Plugin manager and network
    engine::PluginManager m_plugin_manager;
    engine::INetworkPlugin* m_network = nullptr;

    // Server components
    std::unique_ptr<BagarioSession> m_session;
    std::unique_ptr<BagarioNetworkHandler> m_network_handler;
    std::unique_ptr<BagarioPacketSender> m_packet_sender;

    // Configuration
    uint16_t m_tcp_port;
    uint16_t m_udp_port;
    bool m_listen_on_all_interfaces;

    // State
    std::atomic<bool> m_running{false};
    mutable std::mutex m_players_mutex;
    std::unordered_map<uint32_t, PlayerInfo> m_players;  // client_id -> PlayerInfo
    std::unordered_map<uint32_t, uint32_t> m_player_to_client;  // player_id -> client_id

    uint32_t m_next_player_id = 1;

    // Timing
    std::chrono::steady_clock::time_point m_last_snapshot_time;
    std::chrono::steady_clock::time_point m_last_leaderboard_time;
    std::chrono::steady_clock::time_point m_last_tick_time;

    // Random number generation for colors
    std::mt19937 m_rng{std::random_device{}()};
};

}
