#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "plugin_manager/IGraphicsPlugin.hpp"

namespace bagario {

/**
 * @brief Skin pattern types
 */
enum class SkinPattern : uint8_t {
    SOLO = 0,       // Single solid color
    STRIPES = 1,    // Vertical Two-color stripes
    ZIGZAG = 2,     // Two-color Zigzag pattern
    CIRCULAR = 3,   // Three-color ring pattern
    DOTS = 4,       // Two-color Polka dots
    IMAGE = 5       // Custom image (requires separate image upload)
};

/**
 * @brief Player skin configuration
 *
 * Network serialization format (17 bytes):
 * - 1 byte:  pattern (uint8_t)
 * - 4 bytes: primary color (RGBA)
 * - 4 bytes: secondary color (RGBA)
 * - 4 bytes: tertiary color (RGBA)
 * - 4 bytes: image_id (uint32_t, 0 if no image, server-assigned otherwise)
 */
struct PlayerSkin {
    static constexpr size_t SERIALIZED_SIZE = 17;

    SkinPattern pattern = SkinPattern::SOLO;
    engine::Color primary   = {76, 175, 80, 255};   // Green
    engine::Color secondary = {33, 150, 243, 255};  // Blue
    engine::Color tertiary  = {244, 67, 54, 255};   // Red
    std::string image_path;                         // Path to local image file
    uint32_t image_id = 0;                          // Server-assigned image ID (for network sync)

    /**
     * @brief Serialize skin data to bytes for network transmission
     * @return Vector of bytes representing the skin (17 bytes)
     */
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data;
        data.reserve(SERIALIZED_SIZE);

        // Pattern (1 byte)
        data.push_back(static_cast<uint8_t>(pattern));

        // Primary color (4 bytes: RGBA)
        data.push_back(primary.r);
        data.push_back(primary.g);
        data.push_back(primary.b);
        data.push_back(primary.a);

        // Secondary color (4 bytes: RGBA)
        data.push_back(secondary.r);
        data.push_back(secondary.g);
        data.push_back(secondary.b);
        data.push_back(secondary.a);

        // Tertiary color (4 bytes: RGBA)
        data.push_back(tertiary.r);
        data.push_back(tertiary.g);
        data.push_back(tertiary.b);
        data.push_back(tertiary.a);

        // Image ID (4 bytes, little-endian)
        data.push_back(static_cast<uint8_t>(image_id & 0xFF));
        data.push_back(static_cast<uint8_t>((image_id >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>((image_id >> 16) & 0xFF));
        data.push_back(static_cast<uint8_t>((image_id >> 24) & 0xFF));

        return data;
    }

    /**
     * @brief Deserialize skin data from bytes
     * @param data Byte vector (must be at least 17 bytes)
     * @return true if deserialization succeeded
     */
    bool deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < SERIALIZED_SIZE) {
            return false;
        }

        size_t offset = 0;

        // Pattern (1 byte)
        uint8_t pat = data[offset++];
        if (pat > static_cast<uint8_t>(SkinPattern::IMAGE)) {
            return false;
        }
        pattern = static_cast<SkinPattern>(pat);

        // Primary color (4 bytes)
        primary.r = data[offset++];
        primary.g = data[offset++];
        primary.b = data[offset++];
        primary.a = data[offset++];

        // Secondary color (4 bytes)
        secondary.r = data[offset++];
        secondary.g = data[offset++];
        secondary.b = data[offset++];
        secondary.a = data[offset++];

        // Tertiary color (4 bytes)
        tertiary.r = data[offset++];
        tertiary.g = data[offset++];
        tertiary.b = data[offset++];
        tertiary.a = data[offset++];

        // Image ID (4 bytes, little-endian)
        image_id = static_cast<uint32_t>(data[offset]) |
                   (static_cast<uint32_t>(data[offset + 1]) << 8) |
                   (static_cast<uint32_t>(data[offset + 2]) << 16) |
                   (static_cast<uint32_t>(data[offset + 3]) << 24);

        return true;
    }

    /**
     * @brief Deserialize from raw pointer (useful for network packet parsing)
     * @param data Pointer to at least 17 bytes
     * @param size Available size
     * @return true if deserialization succeeded
     */
    bool deserialize(const uint8_t* data, size_t size) {
        if (size < SERIALIZED_SIZE || data == nullptr) {
            return false;
        }
        return deserialize(std::vector<uint8_t>(data, data + SERIALIZED_SIZE));
    }
};

/**
 * @brief Local game state (stored in memory, no server)
 */
struct LocalGameState {
    std::string username = "Player";
    int music_volume = 70;
    int sfx_volume = 80;
    bool fullscreen = false;
    PlayerSkin skin;
};

}  // namespace bagario

