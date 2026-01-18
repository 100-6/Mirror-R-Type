/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MapCollisionManager - Implementation
*/

#include "systems/MapCollisionManager.hpp"
#include "systems/MapConfigLoader.hpp"
#include <iostream>
#include <algorithm>

namespace rtype {

bool MapCollisionManager::loadMap(const std::string& configPath, const std::string& segmentsDir) {
    // Load config
    m_config = MapConfigLoader::loadConfig(configPath);
    
    // Load segments
    auto segmentPaths = MapConfigLoader::getSegmentPaths(segmentsDir);
    if (segmentPaths.empty()) {
        std::cerr << "[MapCollisionManager] No segments found in " << segmentsDir << std::endl;
        return false;
    }
    
    m_segments.clear();
    m_segmentStartX.clear();
    int currentStartX = 0;
    
    for (const auto& path : segmentPaths) {
        SegmentData segment = MapConfigLoader::loadSegment(path);
        m_segmentStartX.push_back(currentStartX);
        currentStartX += segment.width;
        m_segments.push_back(segment);
        
        std::cout << "[MapCollisionManager] Loaded segment " << segment.segmentId 
                  << " (" << segment.width << "x" << segment.height << ")" << std::endl;
    }
    
    m_loaded = true;
    std::cout << "[MapCollisionManager] Map loaded: " << m_segments.size() 
              << " segments, total width: " << currentStartX << " tiles" << std::endl;
    
    return true;
}

float MapCollisionManager::getTotalWidth() const {
    if (m_segments.empty()) return 0.0f;
    
    int totalTiles = 0;
    for (const auto& seg : m_segments) {
        totalTiles += seg.width;
    }
    return static_cast<float>(totalTiles * m_config.tileSize);
}

TileType MapCollisionManager::getTileAt(float worldX, float worldY, float scrollX) const {
    if (!m_loaded || m_segments.empty()) {
        return TileType::EMPTY;
    }
    
    // Convert world position to tile coordinates
    // Note: worldX is relative to scroll, so we need to add scrollX
    float absoluteX = worldX + scrollX;
    int tileX = static_cast<int>(absoluteX / m_config.tileSize);
    int tileY = static_cast<int>(worldY / m_config.tileSize);
    
    if (tileX < 0 || tileY < 0) {
        return TileType::EMPTY;
    }
    
    // Find which segment this tile belongs to
    int segmentIndex = -1;
    int localTileX = 0;
    
    for (size_t i = 0; i < m_segments.size(); ++i) {
        int segStart = m_segmentStartX[i];
        int segEnd = segStart + m_segments[i].width;
        
        if (tileX >= segStart && tileX < segEnd) {
            segmentIndex = static_cast<int>(i);
            localTileX = tileX - segStart;
            break;
        }
    }
    
    if (segmentIndex < 0) {
        return TileType::EMPTY;  // Past end of map
    }
    
    const SegmentData& segment = m_segments[segmentIndex];
    
    // Check bounds
    if (tileY >= segment.height || localTileX >= segment.width) {
        return TileType::EMPTY;
    }
    
    if (tileY >= static_cast<int>(segment.tiles.size()) ||
        localTileX >= static_cast<int>(segment.tiles[tileY].size())) {
        return TileType::EMPTY;
    }
    
    return static_cast<TileType>(segment.tiles[tileY][localTileX]);
}

bool MapCollisionManager::isWallAt(float worldX, float worldY, float scrollX) const {
    TileType tile = getTileAt(worldX, worldY, scrollX);
    return tile != TileType::EMPTY;
}

bool MapCollisionManager::checkCollision(float x, float y, float width, float height, float scrollX) const {
    if (!m_loaded) {
        return false;
    }
    
    int tileSize = m_config.tileSize;
    
    // Check all tile positions that the rectangle might overlap
    int startTileX = static_cast<int>((x + scrollX) / tileSize);
    int endTileX = static_cast<int>((x + scrollX + width) / tileSize);
    int startTileY = static_cast<int>(y / tileSize);
    int endTileY = static_cast<int>((y + height) / tileSize);
    
    for (int ty = startTileY; ty <= endTileY; ++ty) {
        for (int tx = startTileX; tx <= endTileX; ++tx) {
            float tileWorldX = static_cast<float>(tx * tileSize) - scrollX;
            float tileWorldY = static_cast<float>(ty * tileSize);
            
            if (isWallAt(tileWorldX, tileWorldY, scrollX)) {
                return true;
            }
        }
    }
    
    return false;
}

} // namespace rtype
