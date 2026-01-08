/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ChunkManagerSystem - Implementation
*/

#include "systems/ChunkManagerSystem.hpp"
#include "systems/MapConfigLoader.hpp"
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "components/GameComponents.hpp"
#include <iostream>
#include <algorithm>

namespace rtype {

ChunkManagerSystem::ChunkManagerSystem(engine::IGraphicsPlugin& graphics, int screenWidth, int screenHeight)
    : m_graphics(graphics)
    , m_screenWidth(screenWidth)
    , m_screenHeight(screenHeight)
{
}

void ChunkManagerSystem::init(Registry& registry) {
    (void)registry;
    // Initialization is done via initWithConfig()
}

void ChunkManagerSystem::shutdown() {
    m_activeChunks.clear();
    m_segments.clear();
    m_initialized = false;
}

void ChunkManagerSystem::initWithConfig(const MapConfig& config) {
    m_config = config;
    m_scrollSpeed = config.baseScrollSpeed;
    m_autoTiler.setWallSourceRects(config.wallSourceRects);
    m_initialized = true;
}

bool ChunkManagerSystem::loadTileSheet(const std::string& path) {
    m_tileSheetHandle = m_graphics.load_texture(path);
    if (m_tileSheetHandle == engine::INVALID_HANDLE) {
        std::cerr << "Failed to load tile sheet: " << path << std::endl;
        return false;
    }
    std::cout << "Loaded tile sheet: " << path << std::endl;
    return true;
}

void ChunkManagerSystem::loadSegments(const std::vector<std::string>& segmentPaths) {
    m_segments.clear();
    
    for (const auto& path : segmentPaths) {
        SegmentData segment = MapConfigLoader::loadSegment(path);
        m_segments.push_back(segment);
        std::cout << "Loaded segment " << segment.segmentId 
                  << " (" << segment.width << "x" << segment.height << ")" << std::endl;
    }
    
    // Initial chunk loading will be handled in update() since we need registry
    // to spawn entities
}

int ChunkManagerSystem::getChunksNeeded() const {
    int chunkPixelWidth = m_config.chunkWidth * m_config.tileSize;
    int chunksOnScreen = (m_screenWidth / chunkPixelWidth) + 1;
    return chunksOnScreen + 2; // +2 for buffer
}

void ChunkManagerSystem::loadChunk(Registry& registry, int segmentId, int chunkIndex) {
    if (segmentId >= static_cast<int>(m_segments.size())) {
        return;
    }
    
    const SegmentData& segmentData = m_segments[segmentId];
    
    int startX = chunkIndex * m_config.chunkWidth;
    int endX = std::min(startX + m_config.chunkWidth, segmentData.width);
    
    if (startX >= segmentData.width) {
        return;
    }

    Chunk chunk;
    chunk.segmentId = segmentId;
    chunk.chunkIndex = chunkIndex;
    chunk.width = endX - startX;
    chunk.height = segmentData.height;
    chunk.worldX = static_cast<float>(m_nextChunkIndex * m_config.chunkWidth * m_config.tileSize);
    
    // Create padded grid for auto-tiling context
    TileGrid paddedGrid;
    paddedGrid.resize(segmentData.height);
    int paddedWidth = chunk.width + 2;
    
    for (int y = 0; y < segmentData.height; ++y) {
        paddedGrid[y].resize(paddedWidth);
        
        // Left padding
        if (startX > 0) {
            paddedGrid[y][0] = static_cast<TileType>(segmentData.tiles[y][startX - 1]);
        } else if (segmentId > 0) {
            const auto& prevSeg = m_segments[segmentId - 1];
            if (y < static_cast<int>(prevSeg.tiles.size()) && !prevSeg.tiles[y].empty()) {
                paddedGrid[y][0] = static_cast<TileType>(prevSeg.tiles[y].back());
            } else {
                paddedGrid[y][0] = TileType::EMPTY;
            }
        } else {
            paddedGrid[y][0] = TileType::EMPTY;
        }
        
        // Center (actual chunk data)
        for (int x = 0; x < chunk.width; ++x) {
            int srcX = startX + x;
            if (y < static_cast<int>(segmentData.tiles.size()) && 
                srcX < static_cast<int>(segmentData.tiles[y].size())) {
                paddedGrid[y][x + 1] = static_cast<TileType>(segmentData.tiles[y][srcX]);
            } else {
                paddedGrid[y][x + 1] = TileType::EMPTY;
            }
        }
        
        // Right padding
        if (endX < segmentData.width) {
            if (y < static_cast<int>(segmentData.tiles.size()) && 
                endX < static_cast<int>(segmentData.tiles[y].size())) {
                paddedGrid[y][paddedWidth - 1] = static_cast<TileType>(segmentData.tiles[y][endX]);
            } else {
                paddedGrid[y][paddedWidth - 1] = TileType::EMPTY;
            }
        } else if (segmentId + 1 < static_cast<int>(m_segments.size())) {
            const auto& nextSeg = m_segments[segmentId + 1];
            if (y < static_cast<int>(nextSeg.tiles.size()) && !nextSeg.tiles[y].empty()) {
                paddedGrid[y][paddedWidth - 1] = static_cast<TileType>(nextSeg.tiles[y][0]);
            } else {
                paddedGrid[y][paddedWidth - 1] = TileType::EMPTY;
            }
        } else {
            paddedGrid[y][paddedWidth - 1] = TileType::EMPTY;
        }
    }
    
    // Process with auto-tiler
    auto processedPadded = m_autoTiler.processTileGrid(paddedGrid);
    
    // Extract actual chunk tiles (remove padding)
    chunk.tiles.resize(chunk.height);
    chunk.entities.clear();
    
    for (int y = 0; y < chunk.height; ++y) {
        chunk.tiles[y].resize(chunk.width);
        for (int x = 0; x < chunk.width; ++x) {
            auto& tile = processedPadded[y][x + 1];
            chunk.tiles[y][x] = tile;
            
            // Spawn collider entity if tile is not empty
            if (tile.type != TileType::EMPTY) {
                auto entity = registry.spawn_entity();
                
                float size = static_cast<float>(m_config.tileSize);
                
                // Center-based coordinates for collision
                float localX = x * m_config.tileSize + size / 2.0f;
                float localY = y * m_config.tileSize + size / 2.0f;
                
                // Position will be updated in update()
                registry.add_component(entity, Position{ chunk.worldX + localX - m_scrollX, localY });
                registry.add_component(entity, Collider{ size, size });
                registry.add_component(entity, Wall{});
                
                chunk.entities.push_back({ entity, localX, localY });
            }
        }
    }
    
    chunk.isLoaded = true;
    m_activeChunks.push_back(chunk);
    m_nextChunkIndex++;
}

void ChunkManagerSystem::unloadChunk(Registry& registry, int chunkIndex) {
    auto it = std::find_if(m_activeChunks.begin(), m_activeChunks.end(),
        [chunkIndex](const Chunk& c) { return c.chunkIndex == chunkIndex; });
    
    if (it != m_activeChunks.end()) {
        // Destroy all entities associated with this chunk
        for (const auto& entityInfo : it->entities) {
            registry.kill_entity(entityInfo.id);
        }
        m_activeChunks.erase(it);
    }
}

void ChunkManagerSystem::update(Registry& registry, float dt) {
    (void)dt;
    
    if (!m_initialized || m_segments.empty()) {
        return;
    }

    int chunkPixelWidth = m_config.chunkWidth * m_config.tileSize;
    
    // Load more chunks ahead
    if (!m_activeChunks.empty()) {
        float furthestChunkEnd = m_activeChunks.back().worldX + chunkPixelWidth;
        float loadThreshold = m_scrollX + m_screenWidth + chunkPixelWidth;
        
        while (furthestChunkEnd < loadThreshold) {
            int currentSegment = m_activeChunks.back().segmentId;
            int nextChunkInSegment = m_activeChunks.back().chunkIndex + 1;
            
            int segWidth = m_segments[currentSegment].width;
            int chunksInSeg = (segWidth + m_config.chunkWidth - 1) / m_config.chunkWidth;
            
            if (nextChunkInSegment >= chunksInSeg) {
                currentSegment++;
                nextChunkInSegment = 0;
            }
            
            if (currentSegment >= static_cast<int>(m_segments.size())) {
                break; // End of map
            }
            
            size_t beforeCount = m_activeChunks.size();
            loadChunk(registry, currentSegment, nextChunkInSegment);
            
            if (m_activeChunks.size() > beforeCount) {
                furthestChunkEnd = m_activeChunks.back().worldX + chunkPixelWidth;
            } else {
                break;
            }
        }
        
        // Unload chunks behind
        float unloadThreshold = m_scrollX - chunkPixelWidth;
        while (!m_activeChunks.empty() && 
               m_activeChunks.front().worldX + chunkPixelWidth < unloadThreshold) {
            unloadChunk(registry, m_activeChunks.front().chunkIndex);
        }
    } else {
        // Initial load if empty
        loadChunk(registry, 0, 0);
    }
    
    // Update positions of all wall entities based on scroll
    auto& positions = registry.get_components<Position>();
    
    for (const auto& chunk : m_activeChunks) {
        float chunkScreenX = chunk.worldX - m_scrollX;
        
        for (const auto& entityInfo : chunk.entities) {
            if (positions.has_entity(entityInfo.id)) {
                auto& pos = positions[entityInfo.id];
                pos.x = chunkScreenX + entityInfo.localX;
                pos.y = entityInfo.localY;
            }
        }
    }
}

void ChunkManagerSystem::render(float scrollX) const {
    if (m_tileSheetHandle == engine::INVALID_HANDLE) {
        return;
    }
    
    float tileSize = static_cast<float>(m_config.tileSize);
    
    for (const auto& chunk : m_activeChunks) {
        float chunkScreenX = chunk.worldX - scrollX;
        
        // Skip if chunk is off-screen
        if (chunkScreenX + chunk.width * tileSize < 0 || 
            chunkScreenX > m_screenWidth) {
            continue;
        }
        
        // Render tiles
        for (int y = 0; y < static_cast<int>(chunk.tiles.size()); ++y) {
            for (int x = 0; x < static_cast<int>(chunk.tiles[y].size()); ++x) {
                const Tile& tile = chunk.tiles[y][x];
                
                if (tile.type == TileType::EMPTY) {
                    continue;
                }
                
                float drawX = chunkScreenX + x * tileSize;
                float drawY = static_cast<float>(y) * tileSize;
                
                // Skip off-screen tiles
                if (drawX + tileSize < 0 || drawX > m_screenWidth) {
                    continue;
                }
                
                // Create sprite for this tile
                engine::Sprite sprite;
                sprite.texture_handle = m_tileSheetHandle;
                sprite.size = {tileSize, tileSize};
                sprite.source_rect.x = static_cast<float>(tile.sourceRect.x);
                sprite.source_rect.y = static_cast<float>(tile.sourceRect.y);
                sprite.source_rect.width = static_cast<float>(tile.sourceRect.w);
                sprite.source_rect.height = static_cast<float>(tile.sourceRect.h);
                
                // Handle flips by negating source rect dimensions
                // Note: This may not work with all graphics backends
                // A proper solution would require the graphics plugin to support flipping
                if (tile.flipH) {
                    sprite.source_rect.x += sprite.source_rect.width;
                    sprite.source_rect.width = -sprite.source_rect.width;
                }
                if (tile.flipV) {
                    sprite.source_rect.y += sprite.source_rect.height;
                    sprite.source_rect.height = -sprite.source_rect.height;
                }
                
                m_graphics.draw_sprite(sprite, {drawX, drawY});
            }
        }
    }
}

} // namespace rtype
