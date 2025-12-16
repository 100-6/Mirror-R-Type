/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** INetworkPlugin - Network plugin interface
*/

#pragma once

#include "IPlugin.hpp"
#include "CommonTypes.hpp"
#include "PluginExport.hpp"
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace engine {

/**
 * @brief Network protocol type
 */
enum class NetworkProtocol {
    UDP,
    TCP
};

/**
 * @brief Network packet structure
 */
struct NetworkPacket {
    std::vector<uint8_t> data;
    ClientId sender_id = 0;
    uint32_t packet_id = 0;
    uint64_t timestamp = 0;
    NetworkProtocol protocol = NetworkProtocol::TCP;

    NetworkPacket() = default;

    NetworkPacket(const std::vector<uint8_t>& data) : data(data) {}

    NetworkPacket(const void* buffer, size_t size)
        : data(static_cast<const uint8_t*>(buffer),
               static_cast<const uint8_t*>(buffer) + size) {}
};

/**
 * @brief Network plugin interface
 *
 * This interface defines the contract for all network plugins.
 * Supports hybrid TCP/UDP architecture:
 * - TCP: Reliable communication for connections, lobbies, authentication
 * - UDP: Low-latency communication for gameplay
 */
class INetworkPlugin : public IPlugin {
public:
    virtual ~INetworkPlugin() = default;

    // ============== Server Operations ==============

    /**
     * @brief Start a hybrid server with TCP and UDP
     * @param tcp_port Port for TCP connections (lobby, auth)
     * @param udp_port Port for UDP communication (gameplay)
     * @return true if server started successfully
     */
    virtual bool start_server(uint16_t tcp_port, uint16_t udp_port) = 0;

    /**
     * @brief Stop the server (both TCP and UDP)
     */
    virtual void stop_server() = 0;

    /**
     * @brief Check if server is running
     * @return true if server is running
     */
    virtual bool is_server_running() const = 0;

    // ============== Client Operations ==============

    /**
     * @brief Connect to server via TCP (for lobby, authentication)
     * @param host Server hostname or IP address
     * @param port TCP port
     * @return true if connection succeeded
     */
    virtual bool connect_tcp(const std::string& host, uint16_t port) = 0;

    /**
     * @brief Connect to server via UDP (for gameplay, called after TCP connection)
     * @param host Server hostname or IP address
     * @param port UDP port
     * @return true if connection succeeded
     */
    virtual bool connect_udp(const std::string& host, uint16_t port) = 0;

    /**
     * @brief Disconnect from the server (both TCP and UDP)
     */
    virtual void disconnect() = 0;

    /**
     * @brief Check if connected via TCP
     * @return true if TCP connected
     */
    virtual bool is_tcp_connected() const = 0;

    /**
     * @brief Check if connected via UDP
     * @return true if UDP connected
     */
    virtual bool is_udp_connected() const = 0;

    // ============== Client Communication ==============

    /**
     * @brief Send a packet via TCP (client mode)
     * @param packet Packet to send
     * @return true if packet was sent successfully
     */
    virtual bool send_tcp(const NetworkPacket& packet) = 0;

    /**
     * @brief Send a packet via UDP (client mode)
     * @param packet Packet to send
     * @return true if packet was sent successfully
     */
    virtual bool send_udp(const NetworkPacket& packet) = 0;

    // ============== Server Communication ==============

    /**
     * @brief Send a TCP packet to a specific client (server mode)
     * @param packet Packet to send
     * @param client_id Target client ID
     * @return true if packet was sent successfully
     */
    virtual bool send_tcp_to(const NetworkPacket& packet, ClientId client_id) = 0;

    /**
     * @brief Send a UDP packet to a specific client (server mode)
     * @param packet Packet to send
     * @param client_id Target client ID (must be associated via associate_udp_client)
     * @return true if packet was sent successfully
     */
    virtual bool send_udp_to(const NetworkPacket& packet, ClientId client_id) = 0;

    /**
     * @brief Broadcast a TCP packet to all connected clients (server mode)
     * @param packet Packet to broadcast
     * @return Number of clients the packet was sent to
     */
    virtual size_t broadcast_tcp(const NetworkPacket& packet) = 0;

    /**
     * @brief Broadcast a UDP packet to all associated clients (server mode)
     * @param packet Packet to broadcast
     * @return Number of clients the packet was sent to
     */
    virtual size_t broadcast_udp(const NetworkPacket& packet) = 0;

    /**
     * @brief Broadcast TCP packet to all clients except one (server mode)
     * @param packet Packet to broadcast
     * @param exclude_client_id Client to exclude
     * @return Number of clients the packet was sent to
     */
    virtual size_t broadcast_tcp_except(const NetworkPacket& packet, ClientId exclude_client_id) = 0;

    /**
     * @brief Broadcast UDP packet to all clients except one (server mode)
     * @param packet Packet to broadcast
     * @param exclude_client_id Client to exclude
     * @return Number of clients the packet was sent to
     */
    virtual size_t broadcast_udp_except(const NetworkPacket& packet, ClientId exclude_client_id) = 0;

    // ============== UDP Client Association ==============

    /**
     * @brief Associate a UDP endpoint with a TCP client (server mode)
     * Called when receiving UDP handshake from a client
     * @param tcp_client_id The TCP client ID
     * @param udp_client_id The UDP client ID (from UDP packet sender_id)
     */
    virtual void associate_udp_client(ClientId tcp_client_id, ClientId udp_client_id) = 0;

    /**
     * @brief Get the TCP client ID associated with a UDP client
     * @param udp_client_id The UDP client ID
     * @return TCP client ID, or 0 if not found
     */
    virtual ClientId get_tcp_client_from_udp(ClientId udp_client_id) const = 0;

    /**
     * @brief Check if a TCP client has an associated UDP connection
     * @param tcp_client_id The TCP client ID
     * @return true if UDP is associated
     */
    virtual bool has_udp_association(ClientId tcp_client_id) const = 0;

    // ============== Receiving ==============

    /**
     * @brief Receive available packets (from both TCP and UDP)
     * Check packet.protocol to determine the source
     * @return Vector of received packets
     */
    virtual std::vector<NetworkPacket> receive() = 0;

    /**
     * @brief Update the network plugin (poll for events, check timeouts)
     * @param delta_time Time elapsed since last update
     */
    virtual void update(float delta_time) = 0;

    // ============== Callbacks ==============

    /**
     * @brief Set callback for when a client connects via TCP (server mode)
     * @param callback Function to call with client ID
     */
    virtual void set_on_client_connected(std::function<void(ClientId)> callback) = 0;

    /**
     * @brief Set callback for when a client disconnects (server mode)
     * @param callback Function to call with client ID
     */
    virtual void set_on_client_disconnected(std::function<void(ClientId)> callback) = 0;

    /**
     * @brief Set callback for when a packet is received
     * @param callback Function to call with client ID and packet
     */
    virtual void set_on_packet_received(std::function<void(ClientId, const NetworkPacket&)> callback) = 0;

    /**
     * @brief Set callback for when TCP connection to server is established (client mode)
     * @param callback Function to call
     */
    virtual void set_on_connected(std::function<void()> callback) = 0;

    /**
     * @brief Set callback for when TCP connection to server is lost (client mode)
     * @param callback Function to call
     */
    virtual void set_on_disconnected(std::function<void()> callback) = 0;

    // ============== Statistics ==============

    /**
     * @brief Get number of TCP connected clients (server mode)
     * @return Number of connected clients
     */
    virtual size_t get_client_count() const = 0;

    /**
     * @brief Get list of TCP connected client IDs (server mode)
     * @return Vector of client IDs
     */
    virtual std::vector<ClientId> get_client_ids() const = 0;

    /**
     * @brief Get ping/latency to a client (server mode)
     * @param client_id Client ID
     * @return Ping in milliseconds, or -1 if unknown
     */
    virtual int get_client_ping(ClientId client_id) const = 0;

    /**
     * @brief Get ping/latency to server (client mode)
     * @return Ping in milliseconds, or -1 if unknown
     */
    virtual int get_server_ping() const = 0;
};

}

// Forward declaration for C linkage
namespace engine {
    class INetworkPlugin;
}

// Plugin factory function signatures
extern "C" {
    /**
     * @brief Factory function to create a network plugin instance
     * @return Pointer to the created plugin
     */
    PLUGIN_API engine::INetworkPlugin* create_network_plugin();

    /**
     * @brief Destroy a network plugin instance
     * @param plugin Plugin to destroy
     */
    PLUGIN_API void destroy_network_plugin(engine::INetworkPlugin* plugin);
}
