#pragma once

#include <cstdint>

// Cross-platform packed struct support
#ifdef _MSC_VER
    #define PACK_START __pragma(pack(push, 1))
    #define PACK_END __pragma(pack(pop))
    #define PACKED
#else
    #define PACK_START
    #define PACK_END
    #define PACKED __attribute__((packed))
#endif

namespace rtype::protocol {

/**
 * @brief Protocol version constant
 * Current version: 1.0
 */
constexpr uint8_t PROTOCOL_VERSION = 0x01;

/**
 * @brief Packet flags bitfield
 *
 * Bit 0 (0x01): COMPRESSED - Indicates payload is compressed with LZ4
 * Bits 1-7: Reserved for future use
 */
constexpr uint8_t PACKET_FLAG_COMPRESSED = 0x01;

/**
 * @brief Maximum packet size (MTU-safe)
 * Maximum total packet size including header
 */
constexpr uint16_t MAX_PACKET_SIZE = 1400;

/**
 * @brief Base packet header size (without compression metadata)
 */
constexpr uint16_t HEADER_SIZE = 9;

/**
 * @brief Additional size when packet is compressed (uncompressed_size field)
 */
constexpr uint16_t COMPRESSED_HEADER_EXTRA = 4;

/**
 * @brief Maximum payload size (accounting for largest possible header)
 */
constexpr uint16_t MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - HEADER_SIZE - COMPRESSED_HEADER_EXTRA; // (1387 bytes)

/**
 * @brief Packet header structure (9 bytes base, 13 bytes if compressed)
 *
 * All multi-byte fields are in network byte order (big-endian).
 * This structure uses compiler-specific packing to prevent padding.
 *
 * Layout (base header - 9 bytes):
 * - Offset 0 (1 byte):  version
 * - Offset 1 (1 byte):  type
 * - Offset 2 (1 byte):  flags (bit 0 = COMPRESSED)
 * - Offset 3 (2 bytes): payload_length (big-endian)
 * - Offset 5 (4 bytes): sequence_number (big-endian)
 *
 * Additional fields if PACKET_FLAG_COMPRESSED is set (+ 4 bytes):
 * - Offset 9 (4 bytes): uncompressed_size (big-endian)
 */
PACK_START
struct PACKED PacketHeader {
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
     * @brief Packet flags bitfield
     * - Bit 0 (0x01): COMPRESSED - Payload is compressed
     * - Bits 1-7: Reserved
     */
    uint8_t flags;

    /**
     * @brief Payload length in bytes (big-endian)
     * If COMPRESSED flag set: this is the compressed size
     * Must not exceed MAX_PAYLOAD_SIZE
     */
    uint16_t payload_length;

    /**
     * @brief Monotonic sequence number (big-endian)
     * Used for packet ordering and loss detection
     * Wraps around at 2^32
     */
    uint32_t sequence_number;

    /**
     * @brief Original uncompressed size (only valid if flags & PACKET_FLAG_COMPRESSED)
     * This field is NOT part of the fixed header structure but follows it in the packet
     */
    uint32_t uncompressed_size;

    /**
     * @brief Default constructor
     */
    PacketHeader()
        : version(PROTOCOL_VERSION)
        , type(0)
        , flags(0)
        , payload_length(0)
        , sequence_number(0)
        , uncompressed_size(0)
    {}

    /**
     * @brief Parameterized constructor
     *
     * @param type Packet type
     * @param payload_length Payload size in bytes (compressed if flags set)
     * @param sequence_number Sequence number
     * @param flags Packet flags (default 0)
     */
    PacketHeader(uint8_t type, uint16_t payload_length, uint32_t sequence_number, uint8_t flags = 0)
        : version(PROTOCOL_VERSION)
        , type(type)
        , flags(flags)
        , payload_length(payload_length)
        , sequence_number(sequence_number)
        , uncompressed_size(0)
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
        uint16_t header_size = HEADER_SIZE;

        if (flags & PACKET_FLAG_COMPRESSED)
            header_size += COMPRESSED_HEADER_EXTRA;
        return header_size + payload_length;
    }

    /**
     * @brief Get actual header size for this packet
     *
     * @return Header size in bytes (9 or 13 depending on compression flag)
     */
    uint16_t get_header_size() const {
        return (flags & PACKET_FLAG_COMPRESSED) ? (HEADER_SIZE + COMPRESSED_HEADER_EXTRA) : HEADER_SIZE;
    }

    /**
     * @brief Check if packet is compressed
     *
     * @return true if COMPRESSED flag is set
     */
    bool is_compressed() const {
        return (flags & PACKET_FLAG_COMPRESSED) != 0;
    }
};
PACK_END

// IMPORTANT !!
// Note: PacketHeader struct size is now 13 bytes due to uncompressed_size field
// The actual wire format is 9 bytes base + 4 bytes conditional (when compressed)
static_assert(alignof(PacketHeader) == 1, "PacketHeader must have no alignment requirements");

}
