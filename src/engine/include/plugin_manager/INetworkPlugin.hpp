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
 * @brief Network packet structure
 */
struct NetworkPacket {
    std::vector<uint8_t> data;
    ClientId sender_id = 0;
    uint32_t packet_id = 0;
    uint64_t timestamp = 0;

    NetworkPacket() = default;
    
    NetworkPacket(const std::vector<uint8_t>& data) : data(data) {}
    
    NetworkPacket(const void* buffer, size_t size) 
        : data(static_cast<const uint8_t*>(buffer), 
               static_cast<const uint8_t*>(buffer) + size) {}
};

/**
 * @brief Network protocol type
 */
enum class NetworkProtocol {
    UDP,
    TCP
};

/**
 * @brief Network plugin interface
 * 
 * This interface defines the contract for all network plugins.
 * Implementations can use Boost.Asio, ENet, or any other networking library.
 */
class INetworkPlugin : public IPlugin {
public:
    virtual ~INetworkPlugin() = default;

    // Server operations
    /**
     * @brief Start a server on the specified port
     * @param port Port to listen on
     * @param protocol Protocol to use (UDP or TCP)
     * @return true if server started successfully
     */
    virtual bool start_server(uint16_t port, NetworkProtocol protocol = NetworkProtocol::UDP) = 0;

    /**
     * @brief Stop the server
     */
    virtual void stop_server() = 0;

    /**
     * @brief Check if server is running
     * @return true if server is running
     */
    virtual bool is_server_running() const = 0;

    // Client operations
    /**
     * @brief Connect to a server
     * @param host Server hostname or IP address
     * @param port Server port
     * @param protocol Protocol to use (UDP or TCP)
     * @return true if connection succeeded
     */
    virtual bool connect(const std::string& host, uint16_t port, 
                        NetworkProtocol protocol = NetworkProtocol::UDP) = 0;

    /**
     * @brief Disconnect from the server
     */
    virtual void disconnect() = 0;

    /**
     * @brief Check if connected to a server
     * @return true if connected
     */
    virtual bool is_connected() const = 0;

    // Communication
    /**
     * @brief Send a packet (client mode)
     * @param packet Packet to send
     * @return true if packet was sent successfully
     */
    virtual bool send(const NetworkPacket& packet) = 0;

    /**
     * @brief Send a packet to a specific client (server mode)
     * @param packet Packet to send
     * @param client_id Target client ID
     * @return true if packet was sent successfully
     */
    virtual bool send_to(const NetworkPacket& packet, ClientId client_id) = 0;

    /**
     * @brief Broadcast a packet to all connected clients (server mode)
     * @param packet Packet to broadcast
     * @return Number of clients the packet was sent to
     */
    virtual size_t broadcast(const NetworkPacket& packet) = 0;

    /**
     * @brief Broadcast a packet to all clients except one (server mode)
     * @param packet Packet to broadcast
     * @param exclude_client_id Client to exclude
     * @return Number of clients the packet was sent to
     */
    virtual size_t broadcast_except(const NetworkPacket& packet, ClientId exclude_client_id) = 0;

    /**
     * @brief Receive available packets
     * @return Vector of received packets
     */
    virtual std::vector<NetworkPacket> receive() = 0;

    /**
     * @brief Update the network plugin (poll for events)
     * @param delta_time Time elapsed since last update
     */
    virtual void update(float delta_time) = 0;

    // Callbacks
    /**
     * @brief Set callback for when a client connects (server mode)
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
     * @brief Set callback for when connection to server is established (client mode)
     * @param callback Function to call
     */
    virtual void set_on_connected(std::function<void()> callback) = 0;

    /**
     * @brief Set callback for when connection to server is lost (client mode)
     * @param callback Function to call
     */
    virtual void set_on_disconnected(std::function<void()> callback) = 0;

    // Statistics
    /**
     * @brief Get number of connected clients (server mode)
     * @return Number of connected clients
     */
    virtual size_t get_client_count() const = 0;

    /**
     * @brief Get list of connected client IDs (server mode)
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
