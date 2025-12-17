/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** NetworkHandler - Handles network packet routing and processing
*/

#pragma once

#include <cstdint>
#include <vector>

#include "protocol/PacketHeader.hpp"
#include "protocol/PacketTypes.hpp"
#include "protocol/Payloads.hpp"
#include "plugin_manager/INetworkPlugin.hpp"
#include "interfaces/INetworkListener.hpp"

namespace rtype::server {

/**
 * @brief Handles network packet routing and processing
 *
 * Simple class that:
 * - Receives packets from the network plugin
 * - Decodes packet headers
 * - Routes to appropriate handler based on type
 * - Notifies listener of events
 */
class NetworkHandler {
public:
    /**
     * @brief Construct a new NetworkHandler
     * @param network_plugin Pointer to the network plugin
     */
    explicit NetworkHandler(engine::INetworkPlugin* network_plugin);

    // === Configuration ===

    /**
     * @brief Set the listener for network events
     */
    void set_listener(INetworkListener* listener) { listener_ = listener; }

    // === Processing ===

    /**
     * @brief Process all pending packets from the network
     * Call this every server tick
     */
    void process_packets();

private:
    engine::INetworkPlugin* network_plugin_;
    INetworkListener* listener_ = nullptr;

    void route_packet(uint32_t client_id, const protocol::PacketHeader& header,
                     const std::vector<uint8_t>& payload, engine::NetworkProtocol protocol);

    void handle_tcp_packet(uint32_t client_id, protocol::PacketType type,
                          const std::vector<uint8_t>& payload);

    void handle_udp_packet(uint32_t client_id, protocol::PacketType type,
                          const std::vector<uint8_t>& payload);
};

}
