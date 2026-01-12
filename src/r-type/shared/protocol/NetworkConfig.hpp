#pragma once

#include <cstdint>

namespace rtype::protocol {

/**
 * @brief Network configuration constants shared between client and server
 */
namespace config {

constexpr uint16_t DEFAULT_TCP_PORT = 4242;  // TCP port for connections/lobby
constexpr uint16_t DEFAULT_UDP_PORT = 4243;  // UDP port for gameplay

constexpr bool ENABLE_COMPRESSION = true;           // Master switch for packet compression
constexpr size_t MIN_COMPRESSION_SIZE = 128;        // Minimum payload size (bytes) to attempt compression
constexpr float MIN_COMPRESSION_GAIN = 0.10f;       // Minimum compression ratio gain (10%) to use compressed data
constexpr size_t MAX_COMPRESSION_TIME_US = 500;     // Maximum compression time in microseconds (safety limit)

}

}
