#pragma once

#include "../PacketTypes.hpp"
#include "../NetworkConfig.hpp"
#include <vector>
#include <cstdint>
#include <chrono>
#include <unordered_set>

namespace rtype::protocol {

/**
 * @brief Packet compression and decompression using LZ4
 *
 * Provides transparent compression/decompression for eligible packet types.
 * Uses LZ4 algorithm for fast, low-latency compression suitable for real-time gaming.
 *
 * Compression is applied selectively based on:
 * - Packet type (only specific types are compressed)
 * - Payload size (minimum threshold)
 * - Compression ratio (only use if sufficient gain)
 */
class PacketCompressor {
public:
    /**
     * @brief Result of a compression operation
     */
    struct CompressionResult {
        std::vector<uint8_t> data;              ///< Compressed data
        size_t original_size;                   ///< Original uncompressed size
        size_t compressed_size;                 ///< Final compressed size
        float ratio;                            ///< Compression ratio (compressed/original)
        std::chrono::microseconds compression_time;  ///< Time taken to compress
        bool used_compression;                  ///< True if compression was actually used

        CompressionResult()
            : original_size(0)
            , compressed_size(0)
            , ratio(1.0f)
            , compression_time(0)
            , used_compression(false)
        {}
    };

    /**
     * @brief Compress payload data using LZ4
     *
     * Attempts to compress the payload. If compression doesn't provide sufficient gain
     * (as defined by MIN_COMPRESSION_GAIN), returns the original data uncompressed.
     *
     * @param payload Input payload data
     * @return CompressionResult with compressed data and statistics
     */
    static CompressionResult compress(const std::vector<uint8_t>& payload);

    /**
     * @brief Decompress payload data using LZ4
     *
     * @param compressed_data Compressed payload data
     * @param compressed_size Size of compressed data in bytes
     * @param original_size Original uncompressed size in bytes
     * @return Decompressed data as byte vector
     * @throws std::runtime_error if decompression fails
     */
    static std::vector<uint8_t> decompress(const uint8_t* compressed_data,
                                            size_t compressed_size,
                                            size_t original_size);

    /**
     * @brief Determine if a packet should be compressed based on type and size
     *
     * Checks:
     * - Global compression enable flag
     * - Packet type is in compressible set
     * - Payload size meets minimum threshold
     *
     * @param type Packet type
     * @param payload_size Size of payload in bytes
     * @return true if compression should be attempted
     */
    static bool should_compress(PacketType type, size_t payload_size);

    /**
     * @brief Check if a specific packet type is eligible for compression
     *
     * @param type Packet type to check
     * @return true if type can be compressed
     */
    static bool is_compressible_type(PacketType type);

private:
    /**
     * @brief Set of packet types eligible for compression
     *
     * Includes:
     * - SERVER_SNAPSHOT (0xA0) - High priority: entity state arrays
     * - SERVER_DELTA_SNAPSHOT (0xA1) - High priority: delta state
     * - SERVER_ROOM_LIST (0x91) - Medium priority: room listings
     * - SERVER_LOBBY_STATE (0x87) - Medium priority: lobby state with players
     * - SERVER_GAME_START (0x8A) - Low priority: occasional large packet
     *
     * Excludes:
     * - CLIENT_INPUT (0x10) - Too small (14 bytes), sent frequently
     * - Connection/ping packets - Too small
     * - Single-entity events - Usually small
     */
    static const std::unordered_set<PacketType> COMPRESSIBLE_TYPES;
};

}
