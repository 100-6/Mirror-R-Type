#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
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
    IMAGE = 5       // Custom image with inline data
};

/**
 * @brief Player skin configuration
 *
 * Network serialization format (variable size):
 * - Header (17 bytes fixed):
 *   - 1 byte:  pattern (uint8_t)
 *   - 4 bytes: primary color (RGBA)
 *   - 4 bytes: secondary color (RGBA)
 *   - 4 bytes: tertiary color (RGBA)
 *   - 4 bytes: image_data_size (uint32_t, 0 if no image)
 * - Image data (variable, only if pattern == IMAGE and image_data_size > 0):
 *   - N bytes: raw image file data (PNG/JPG bytes)
 *
 * Maximum recommended image size: 256x256, ~100KB
 */
struct PlayerSkin {
    static constexpr size_t HEADER_SIZE = 17;
    static constexpr size_t MAX_IMAGE_SIZE = 256 * 1024;  // 256 KB max

    SkinPattern pattern = SkinPattern::SOLO;
    engine::Color primary   = {76, 175, 80, 255};   // Green
    engine::Color secondary = {33, 150, 243, 255};  // Blue
    engine::Color tertiary  = {244, 67, 54, 255};   // Red
    std::string image_path;                         // Path to local image file (client-side only)
    std::vector<uint8_t> image_data;                // Raw image bytes for network sync

    /**
     * @brief Load image data from file into image_data vector
     * @param path Path to image file (PNG, JPG, etc.)
     * @return true if loaded successfully, false on error or file too large
     */
    bool load_image_from_file(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return false;
        }

        std::streamsize size = file.tellg();
        if (size <= 0 || static_cast<size_t>(size) > MAX_IMAGE_SIZE) {
            return false;  // File too large or empty
        }

        file.seekg(0, std::ios::beg);
        image_data.resize(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(image_data.data()), size)) {
            image_data.clear();
            return false;
        }

        image_path = path;
        return true;
    }

    /**
     * @brief Check if image data is loaded and ready for network transmission
     */
    bool has_image_data() const {
        return pattern == SkinPattern::IMAGE && !image_data.empty();
    }

    /**
     * @brief Get total serialized size including image data
     */
    size_t get_serialized_size() const {
        return HEADER_SIZE + (has_image_data() ? image_data.size() : 0);
    }

    /**
     * @brief Serialize skin data to bytes for network transmission
     * @return Vector of bytes representing the skin (header + optional image data)
     */
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> data;
        data.reserve(get_serialized_size());

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

        // Image data size (4 bytes, little-endian)
        uint32_t img_size = has_image_data() ? static_cast<uint32_t>(image_data.size()) : 0;
        data.push_back(static_cast<uint8_t>(img_size & 0xFF));
        data.push_back(static_cast<uint8_t>((img_size >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>((img_size >> 16) & 0xFF));
        data.push_back(static_cast<uint8_t>((img_size >> 24) & 0xFF));

        // Append image data if present
        if (has_image_data()) {
            data.insert(data.end(), image_data.begin(), image_data.end());
        }

        return data;
    }

    /**
     * @brief Deserialize skin data from bytes
     * @param data Byte vector (must be at least HEADER_SIZE bytes)
     * @return true if deserialization succeeded
     */
    bool deserialize(const std::vector<uint8_t>& data) {
        if (data.size() < HEADER_SIZE) {
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

        // Image data size (4 bytes, little-endian)
        uint32_t img_size = static_cast<uint32_t>(data[offset]) |
                           (static_cast<uint32_t>(data[offset + 1]) << 8) |
                           (static_cast<uint32_t>(data[offset + 2]) << 16) |
                           (static_cast<uint32_t>(data[offset + 3]) << 24);
        offset += 4;

        // Read image data if present
        image_data.clear();
        if (img_size > 0) {
            if (img_size > MAX_IMAGE_SIZE || data.size() < offset + img_size) {
                return false;  // Invalid size or not enough data
            }
            image_data.assign(data.begin() + offset, data.begin() + offset + img_size);
        }

        return true;
    }

    /**
     * @brief Deserialize from raw pointer (useful for network packet parsing)
     * @param data Pointer to at least HEADER_SIZE bytes
     * @param size Available size
     * @return true if deserialization succeeded
     */
    bool deserialize(const uint8_t* data, size_t size) {
        if (size < HEADER_SIZE || data == nullptr) {
            return false;
        }
        return deserialize(std::vector<uint8_t>(data, data + size));
    }
};

/**
 * @brief Local game state (stored in memory, no server)
 */
struct LocalGameState {
    std::string username = "Player";
    std::string server_ip = "127.0.0.1";
    int music_volume = 70;
    int sfx_volume = 80;
    bool fullscreen = false;
    PlayerSkin skin;
};

}  // namespace bagario

