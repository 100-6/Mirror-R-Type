#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <vector>

#include "plugin_manager/INetworkPlugin.hpp"
#include "plugin_manager/PluginManager.hpp"
#include "BagarioClientNetworkHandler.hpp"
#include "BagarioClientPacketSender.hpp"
#include "ClientNetworkCallbacks.hpp"
#include "LocalGameState.hpp"

namespace bagario::client {

/**
 * @brief Connection state machine states
 */
enum class ConnectionState {
    DISCONNECTED,    // Not connected
    CONNECTING,      // TCP connection in progress
    CONNECTED,       // TCP connected, waiting for SERVER_ACCEPT
    IN_GAME          // Fully connected and playing
};

/**
 * @brief High-level network manager for the Bagario client
 *
 * Owns the network plugin, handler, and sender.
 * Manages connection state and provides a simple API for gameplay.
 */
class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    // Prevent copying
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    /**
     * @brief Initialize the network subsystem (load ENet plugin)
     * @return true if initialization succeeded
     */
    bool initialize();

    /**
     * @brief Shutdown the network subsystem
     */
    void shutdown();

    /**
     * @brief Connect to a game server
     * @param host Server hostname or IP
     * @param tcp_port TCP port (default 5002)
     * @param udp_port UDP port (default 5003)
     * @return true if connection was initiated successfully
     */
    bool connect(const std::string& host, uint16_t tcp_port = 5002, uint16_t udp_port = 5003);

    /**
     * @brief Disconnect from the server
     */
    void disconnect();

    /**
     * @brief Send join request with player name and skin
     * @param player_name Player's display name
     * @param skin Player's skin configuration
     */
    void request_join(const std::string& player_name, const PlayerSkin& skin);

    /**
     * @brief Update network (process packets, handle callbacks)
     * Call this every frame
     * @param dt Delta time since last update
     */
    void update(float dt);

    // ============== Gameplay Input ==============

    /**
     * @brief Send mouse position as movement target
     * @param target_x World X coordinate
     * @param target_y World Y coordinate
     */
    void send_input(float target_x, float target_y);

    /**
     * @brief Request to split cells
     */
    void send_split();

    /**
     * @brief Request to eject mass
     * @param dir_x Direction X (normalized)
     * @param dir_y Direction Y (normalized)
     */
    void send_eject_mass(float dir_x, float dir_y);

    /**
     * @brief Send skin data to server
     * @param skin Skin configuration to send
     */
    void send_skin(const PlayerSkin& skin);

    /**
     * @brief Send ping to measure latency
     */
    void send_ping();

    // ============== Callbacks ==============

    /**
     * @brief Set callbacks for server events
     */
    void set_callbacks(const ClientNetworkCallbacks& callbacks);

    // ============== State Queries ==============

    bool is_connected() const { return m_state == ConnectionState::IN_GAME; }
    bool is_connecting() const { return m_state == ConnectionState::CONNECTING || m_state == ConnectionState::CONNECTED; }
    ConnectionState get_state() const { return m_state; }

    uint32_t get_player_id() const { return m_player_id; }
    float get_map_width() const { return m_map_width; }
    float get_map_height() const { return m_map_height; }
    int get_ping_ms() const { return m_ping_ms; }

    const std::string& get_connection_error() const { return m_connection_error; }

private:
    void setup_internal_callbacks();
    void handle_accept(const protocol::ServerAcceptPayload& payload);
    void handle_reject(const protocol::ServerRejectPayload& payload);
    void handle_pong(const protocol::ServerPongPayload& payload);
    void handle_disconnected();

    uint32_t get_current_timestamp_ms() const;

    // Plugin management
    engine::PluginManager m_plugin_manager;
    engine::INetworkPlugin* m_network = nullptr;

    // Handler and sender
    std::unique_ptr<BagarioClientNetworkHandler> m_handler;
    std::unique_ptr<BagarioClientPacketSender> m_sender;

    // Connection state
    ConnectionState m_state = ConnectionState::DISCONNECTED;
    std::string m_server_host;
    uint16_t m_tcp_port = 0;
    uint16_t m_udp_port = 0;
    std::string m_connection_error;

    // Player state (received from server)
    uint32_t m_player_id = 0;
    float m_map_width = 5000.0f;
    float m_map_height = 5000.0f;

    // Input sequencing
    uint32_t m_input_sequence = 0;

    // Ping tracking
    uint32_t m_last_ping_time = 0;
    int m_ping_ms = -1;
    float m_ping_timer = 0.0f;
    static constexpr float PING_INTERVAL = 1.0f;  // Send ping every second

    // External callbacks
    ClientNetworkCallbacks m_external_callbacks;

    // Pending skin to send after accept
    std::vector<uint8_t> m_pending_skin_data;
};

}  // namespace bagario::client
