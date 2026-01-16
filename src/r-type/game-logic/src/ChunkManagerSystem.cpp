/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ChunkManagerSystem - Implementation
*/

#include "systems/ChunkManagerSystem.hpp"
#include "systems/MapConfigLoader.hpp"
#include "ecs/Registry.hpp"
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
    // Note: m_activeChunks is NOT cleared here - call reset() with registry first
    // to properly destroy wall entities before reinitializing
    m_confirmedScrollX = 0.0;
    m_renderScrollX = 0.0;
    m_nextChunkIndex = 0;
    m_currentSegment = 0;
}

void ChunkManagerSystem::reset(Registry& registry) {
    (void)registry;  // No entities to destroy - purely visual system
    m_activeChunks.clear();
    m_confirmedScrollX = 0.0;
    m_renderScrollX = 0.0;
    m_nextChunkIndex = 0;
    m_currentSegment = 0;
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

    // Calculate worldX by summing widths of all previous segments (same as server)
    // This ensures visual tiles align with server collision walls
    double segment_world_x = 0.0;
    for (int i = 0; i < segmentId; ++i) {
        segment_world_x += static_cast<double>(m_segments[i].width * m_config.tileSize);
    }
    // Add offset within current segment based on chunk index
    chunk.worldX = segment_world_x + static_cast<double>(startX * m_config.tileSize);
    
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
    
    // Track processed tiles for greedy merging
    std::vector<std::vector<bool>> processed(chunk.height, std::vector<bool>(chunk.width, false));

    // Populate actual tiles first
    for (int y = 0; y < chunk.height; ++y) {
        chunk.tiles[y].resize(chunk.width);
        for (int x = 0; x < chunk.width; ++x) { 
            chunk.tiles[y][x] = processedPadded[y][x + 1];
        }
    }

    // NO COLLISION ENTITIES ARE CREATED ON CLIENT
    // Wall collisions are handled server-side only
    // ChunkManagerSystem is purely for visual tile rendering
    // This eliminates all client/server wall position desync issues
    (void)processed;  // Unused now
    (void)registry;   // Unused for entity creation

    chunk.isLoaded = true;
    m_activeChunks.push_back(chunk);
    m_nextChunkIndex++;
}

void ChunkManagerSystem::unloadChunk(Registry& registry, int chunkIndex) {
    (void)registry;  // No entities to destroy - purely visual system

    auto it = std::find_if(m_activeChunks.begin(), m_activeChunks.end(),
        [chunkIndex](const Chunk& c) { return c.chunkIndex == chunkIndex; });

    if (it != m_activeChunks.end()) {
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
    // Use CONFIRMED scroll for chunk loading decisions (authoritative from server)
    // This ensures chunks are loaded based on server state, not extrapolated render state
    if (!m_activeChunks.empty()) {
        const Chunk& lastChunk = m_activeChunks.back();
        // Use actual chunk width, not config chunk width (chunks at segment end may be smaller)
        double lastChunkPixelWidth = static_cast<double>(lastChunk.width * m_config.tileSize);
        double furthestChunkEnd = lastChunk.worldX + lastChunkPixelWidth;
        double loadThreshold = m_confirmedScrollX + static_cast<double>(m_screenWidth + chunkPixelWidth);

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
                const Chunk& newLastChunk = m_activeChunks.back();
                double newLastChunkPixelWidth = static_cast<double>(newLastChunk.width * m_config.tileSize);
                furthestChunkEnd = newLastChunk.worldX + newLastChunkPixelWidth;
            } else {
                break;
            }
        }

        // Unload chunks behind - use actual chunk width with confirmed scroll
        double unloadThreshold = m_confirmedScrollX - static_cast<double>(chunkPixelWidth);
        while (!m_activeChunks.empty()) {
            const Chunk& frontChunk = m_activeChunks.front();
            double frontChunkPixelWidth = static_cast<double>(frontChunk.width * m_config.tileSize);
            if (frontChunk.worldX + frontChunkPixelWidth < unloadThreshold) {
                unloadChunk(registry, frontChunk.chunkIndex);
            } else {
                break;
            }
        }
    } else if (m_nextChunkIndex == 0) {
        // Initial load if empty
        loadChunk(registry, 0, 0);
    }

    // No entity position updates needed - purely visual system
    // Wall collisions are handled server-side only
}

void ChunkManagerSystem::setScrollSpeed(float speed, Registry* registry) {
    (void)registry;  // No entities to update - purely visual system
    m_scrollSpeed = speed;
}

float ChunkManagerSystem::getScrollX() const {
    return static_cast<float>(m_renderScrollX);
}

double ChunkManagerSystem::getConfirmedScrollX() const {
    return m_confirmedScrollX;
}

float ChunkManagerSystem::getScrollSpeed() const {
    return m_scrollSpeed;
}

void ChunkManagerSystem::advanceRenderScroll(float delta) {
    m_renderScrollX += static_cast<double>(delta);
}

void ChunkManagerSystem::setConfirmedScrollX(double scroll) {
    m_confirmedScrollX = scroll;
    m_renderScrollX = scroll;  // Snap render to confirmed
}

void ChunkManagerSystem::setScrollX(double scroll) {
    setConfirmedScrollX(scroll);
}

bool ChunkManagerSystem::isInitialized() const {
    return m_initialized;
}

void ChunkManagerSystem::render() const {
    if (m_tileSheetHandle == engine::INVALID_HANDLE) {
        return;
    }

    float tileSize = static_cast<float>(m_config.tileSize);
    // Use RENDER scroll for smooth visual display (may be slightly ahead of confirmed)
    float scrollX = static_cast<float>(m_renderScrollX);

    for (const auto& chunk : m_activeChunks) {
        float chunkScreenX = static_cast<float>(chunk.worldX) - scrollX;
        
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
