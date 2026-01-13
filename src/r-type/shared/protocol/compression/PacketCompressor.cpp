#include "PacketCompressor.hpp"
#include <lz4.h>
#include <stdexcept>
#include <cstring>

namespace rtype::protocol {

// Define which packet types are eligible for compression
const std::unordered_set<PacketType> PacketCompressor::COMPRESSIBLE_TYPES = {
    PacketType::SERVER_SNAPSHOT,        // 0xA0 - Entity state arrays (up to 800+ bytes)
    PacketType::SERVER_DELTA_SNAPSHOT,  // 0xA1 - Delta state updates
    PacketType::SERVER_ROOM_LIST,       // 0x91 - Room listings (can be large)
    PacketType::SERVER_LOBBY_STATE,     // 0x87 - Lobby state with player list
    PacketType::SERVER_GAME_START,      // 0x8A - Game start packet with configuration
};

PacketCompressor::CompressionResult PacketCompressor::compress(const std::vector<uint8_t>& payload) {
    CompressionResult result;
    result.original_size = payload.size();
    auto start_time = std::chrono::high_resolution_clock::now();
    int max_compressed_size = LZ4_compressBound(static_cast<int>(payload.size()));
    std::vector<uint8_t> compressed_buffer(max_compressed_size);
    int compressed_size = LZ4_compress_default(
        reinterpret_cast<const char*>(payload.data()),
        reinterpret_cast<char*>(compressed_buffer.data()),
        static_cast<int>(payload.size()),
        max_compressed_size
    );
    auto end_time = std::chrono::high_resolution_clock::now();
    result.compression_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

    if (compressed_size <= 0) {
        result.data = payload;
        result.compressed_size = payload.size();
        result.ratio = 1.0f;
        result.used_compression = false;
        return result;
    }
    compressed_buffer.resize(compressed_size);
    result.compressed_size = compressed_size;
    result.ratio = static_cast<float>(compressed_size) / static_cast<float>(payload.size());
    if (result.ratio < (1.0f - config::MIN_COMPRESSION_GAIN)) {
        result.data = std::move(compressed_buffer);
        result.used_compression = true;
    } else {
        result.data = payload;
        result.compressed_size = payload.size();
        result.ratio = 1.0f;
        result.used_compression = false;
    }
    return result;
}

std::vector<uint8_t> PacketCompressor::decompress(const uint8_t* compressed_data,
                                                    size_t compressed_size,
                                                    size_t original_size) {
    std::vector<uint8_t> decompressed_buffer(original_size);
    int decompressed_size = LZ4_decompress_safe(
        reinterpret_cast<const char*>(compressed_data),
        reinterpret_cast<char*>(decompressed_buffer.data()),
        static_cast<int>(compressed_size),
        static_cast<int>(original_size)
    );

    if (decompressed_size < 0)
        throw std::runtime_error("LZ4 decompression failed: corrupted or invalid compressed data");
    if (static_cast<size_t>(decompressed_size) != original_size) {
        throw std::runtime_error("LZ4 decompression size mismatch: expected " +
                                 std::to_string(original_size) + " bytes, got " +
                                 std::to_string(decompressed_size) + " bytes");
    }
    return decompressed_buffer;
}

bool PacketCompressor::should_compress(PacketType type, size_t payload_size) {
    if (!config::ENABLE_COMPRESSION)
        return false;
    if (payload_size < config::MIN_COMPRESSION_SIZE)
        return false;
    if (!is_compressible_type(type))
        return false;
    return true;
}

bool PacketCompressor::is_compressible_type(PacketType type) {
    return COMPRESSIBLE_TYPES.find(type) != COMPRESSIBLE_TYPES.end();
}

}
