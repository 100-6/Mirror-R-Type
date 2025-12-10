#pragma once

#include <cstdint>

namespace rtype::protocol {

/**
 * @brief Protocol version constant
 * Current version: 1.0
 */
constexpr uint8_t PROTOCOL_VERSION = 0x01;

/**
 * @brief Maximum packet size (MTU-safe)
 * Maximum total packet size including header
 */
constexpr uint16_t MAX_PACKET_SIZE = 1400;

/**
 * @brief Packet header size
 */
constexpr uint16_t HEADER_SIZE = 8;

/**
 * @brief Maximum payload size
 */
constexpr uint16_t MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - HEADER_SIZE; // (1392 bytes)

/**
 * @brief Packet header structure (8 bytes)
 *
 * All multi-byte fields are in network byte order (big-endian).
 * This structure uses __attribute__((packed)) to prevent compiler padding.
 *
 * Layout:
 * - Offset 0 (1 byte):  version
 * - Offset 1 (1 byte):  type
 * - Offset 2 (2 bytes): payload_length (big-endian)
 * - Offset 4 (4 bytes): sequence_number (big-endian)
 */
struct __attribute__((packed)) PacketHeader {
    /**
     * @brief Protocol version (must be 0x01)
     */
    uint8_t version;

    /**
     * @brief Packet type identifier
     * - 0x00-0x7F: Client-to-Server packets
     * - 0x80-0xFF: Server-to-Client packets
     */
    uint8_t type;

    /**
     * @brief Payload length in bytes (big-endian)
     * Must not exceed MAX_PAYLOAD_SIZE (1392 bytes)
     */
    uint16_t payload_length;

    /**
     * @brief Monotonic sequence number (big-endian)
     * Used for packet ordering and loss detection
     * Wraps around at 2^32
     */
    uint32_t sequence_number;

    /**
     * @brief Default constructor
     */
    PacketHeader()
        : version(PROTOCOL_VERSION)
        , type(0)
        , payload_length(0)
        , sequence_number(0)
    {}

    /**
     * @brief Parameterized constructor
     *
     * @param type Packet type
     * @param payload_length Payload size in bytes
     * @param sequence_number Sequence number
     */
    PacketHeader(uint8_t type, uint16_t payload_length, uint32_t sequence_number)
        : version(PROTOCOL_VERSION)
        , type(type)
        , payload_length(payload_length)
        , sequence_number(sequence_number)
    {}

    /**
     * @brief Check if header is valid
     *
     * @return true if version is correct and payload size is within limits
     */
    bool is_valid() const {
        return version == PROTOCOL_VERSION && payload_length <= MAX_PAYLOAD_SIZE;
    }

    /**
     * @brief Get total packet size (header + payload)
     *
     * @return Total size in bytes
     */
    uint16_t total_size() const {
        return HEADER_SIZE + payload_length;
    }
};

static_assert(sizeof(PacketHeader) == HEADER_SIZE, "PacketHeader size must be exactly 8 bytes");
static_assert(alignof(PacketHeader) == 1, "PacketHeader must have no alignment requirements");

}
