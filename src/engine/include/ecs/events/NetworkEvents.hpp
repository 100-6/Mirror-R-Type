/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** NetworkEvents - Network-related events
*/

#pragma once

#include "core/event/Event.hpp"
#include <vector>
#include <cstdint>
#include <string>

namespace ecs {

/**
 * @brief Event published when data is received from the network
 */
struct NetworkReceiveEvent : public core::Event {
    std::string address;          // Source address (IP:port for UDP, or client ID)
    std::vector<uint8_t> data;    // Raw packet data

    NetworkReceiveEvent(const std::string& addr, const std::vector<uint8_t>& packet_data)
        : address(addr), data(packet_data) {}
};

/**
 * @brief Event published when data should be sent over the network
 */
struct NetworkSendEvent : public core::Event {
    std::string address;          // Destination address
    std::vector<uint8_t> data;    // Raw packet data to send

    NetworkSendEvent(const std::string& addr, const std::vector<uint8_t>& packet_data)
        : address(addr), data(packet_data) {}
};

/**
 * @brief Event published when a client connects (server-side)
 */
struct ClientConnectedEvent : public core::Event {
    std::string address;          // Client address

    explicit ClientConnectedEvent(const std::string& addr)
        : address(addr) {}
};

/**
 * @brief Event published when a client disconnects (server-side)
 */
struct ClientDisconnectedEvent : public core::Event {
    std::string address;          // Client address

    explicit ClientDisconnectedEvent(const std::string& addr)
        : address(addr) {}
};

}
