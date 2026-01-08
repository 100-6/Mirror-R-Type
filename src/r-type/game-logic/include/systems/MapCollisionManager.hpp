/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MapCollisionManager - Server-side collision checking for maps (headless)
*/

#ifndef MAP_COLLISION_MANAGER_HPP_
#define MAP_COLLISION_MANAGER_HPP_

#include "components/MapTypes.hpp"
#include <vector>
#include <string>

namespace rtype {

/**
 * @brief Server-side map collision manager (headless, no graphics)
 * 
 * This class loads map segment data and provides collision checking
 * for server-side anti-cheat validation.
 */
class MapCollisionManager {
public:
    MapCollisionManager() = default;
    
    /**
     * @brief Load map configuration and segments
     * @param configPath Path to map_config.json
     * @param segmentsDir Path to segments directory
     * @return true if loaded successfully
     */
    bool loadMap(const std::string& configPath, const std::string& segmentsDir);
    
    /**
     * @brief Check if a world position collides with a wall tile
     * @param worldX World X coordinate
     * @param worldY World Y coordinate
     * @param scrollX Current scroll position
     * @return true if the position is inside a wall
     */
    bool isWallAt(float worldX, float worldY, float scrollX) const;
    
    /**
     * @brief Check if a rectangle collides with any wall tiles
     * @param x Left edge
     * @param y Top edge
     * @param width Rectangle width
     * @param height Rectangle height
     * @param scrollX Current scroll position
     * @return true if any part of the rectangle overlaps a wall
     */
    bool checkCollision(float x, float y, float width, float height, float scrollX) const;
    
    /**
     * @brief Get tile type at world position
     * @param worldX World X coordinate
     * @param worldY World Y coordinate  
     * @param scrollX Current scroll position
     * @return TileType at that position (EMPTY if out of bounds)
     */
    TileType getTileAt(float worldX, float worldY, float scrollX) const;
    
    /**
     * @brief Get map config
     */
    const MapConfig& getConfig() const { return m_config; }
    
    /**
     * @brief Get total map width in pixels
     */
    float getTotalWidth() const;
    
    /**
     * @brief Check if map is loaded
     */
    bool isLoaded() const { return m_loaded; }

private:
    MapConfig m_config;
    std::vector<SegmentData> m_segments;
    bool m_loaded = false;
    
    // Cache for quick segment lookup
    std::vector<int> m_segmentStartX;  // Starting X position of each segment in tiles
};

} // namespace rtype

#endif /* !MAP_COLLISION_MANAGER_HPP_ */
