/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MapTypes - Types for the layered map system
*/

#ifndef MAP_TYPES_HPP_
#define MAP_TYPES_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace rtype {

/**
 * @brief Tile type in the map grid
 */
enum class TileType {
    EMPTY = 0,
    WALL = 1,
    DIAGONAL = 2,
    DIAGONAL_MIRROR = 3
};

/**
 * @brief Source rectangle in a sprite sheet
 */
struct SourceRect {
    int x = 0;
    int y = 0;
    int w = 16;
    int h = 16;
};

/**
 * @brief A single tile with its computed source rect
 */
struct Tile {
    TileType type = TileType::EMPTY;
    SourceRect sourceRect;
    bool flipV = false;  // Vertical flip for mirrored diagonals
    bool flipH = false;  // Horizontal flip for left/right spikes
};

/**
 * @brief Position of a tile in the grid
 */
struct TilePosition {
    int x;
    int y;
};

/**
 * @brief Parallax layer configuration from JSON
 */
struct ParallaxLayerConfig {
    std::string path;
    float speedFactor = 1.0f;
};

/**
 * @brief Map information from the registry index
 */
struct MapInfo {
    std::string id;
    std::string name;
    std::string description;
    int difficulty = 1;
    std::string thumbnailPath;
    std::string wavesConfigPath;
};

/**
 * @brief Procedural generation configuration
 */
struct ProceduralConfig {
    bool enabled = false;
    uint32_t seed = 0;  // 0 = random

    // Generation parameters
    int minPassageHeight = 45;
    float stalactiteChance = 0.25f;
    int maxStalactiteLength = 6;
    int pathVariation = 5;
};

/**
 * @brief Map configuration loaded from JSON
 */
struct MapConfig {
    std::string id;              // Map folder ID
    std::string name;            // Display name
    std::string basePath;        // Base path to map folder
    int tileSize = 16;
    int chunkWidth = 30;
    float baseScrollSpeed = 60.0f;
    std::string tileSheetPath;
    std::unordered_map<std::string, SourceRect> wallSourceRects;
    std::vector<ParallaxLayerConfig> parallaxLayers;
    ProceduralConfig procedural;  // Procedural generation config
};

/**
 * @brief Raw segment data loaded from JSON
 */
struct SegmentData {
    int segmentId = 0;
    int width = 30;
    int height = 68;
    std::vector<std::vector<int>> tiles;  // Raw tile type IDs
};

/**
 * @brief A chunk of processed tiles ready for rendering
 */
struct Chunk {
    int segmentId = 0;           // ID of the segment this chunk belongs to
    int chunkIndex = 0;          // Index within the segment
    double worldX = 0.0;         // X position in world coordinates (double for precision)
    int width = 30;              // Width in tiles
    int height = 68;             // Height in tiles (1080 / 16 = 67.5, rounded up)
    
    std::vector<std::vector<Tile>> tiles;  // Processed tiles with source rects
    
    struct ChunkEntity {
        std::size_t id;
        float localX;
        float localY;
    };
    std::vector<ChunkEntity> entities;     // ECS entities for this chunk (colliders)
    
    bool isLoaded = false;
};

/**
 * @brief 2D grid of tile types for auto-tiling
 */
using TileGrid = std::vector<std::vector<TileType>>;

} // namespace rtype

#endif /* !MAP_TYPES_HPP_ */
