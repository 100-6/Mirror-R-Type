#pragma once

#include <cstdint>

namespace rtype::protocol {

/**
 * @brief Network configuration constants shared between client and server
 */
namespace config {

// === Network Ports ===
constexpr uint16_t DEFAULT_TCP_PORT = 4242;  // TCP port for connections/lobby
constexpr uint16_t DEFAULT_UDP_PORT = 4243;  // UDP port for gameplay

} // namespace config

} // namespace rtype::protocol
