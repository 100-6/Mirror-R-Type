/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ChunkManagerSystem - Manages tile chunks for streaming map rendering
*/

#ifndef CHUNK_MANAGER_SYSTEM_HPP_
#define CHUNK_MANAGER_SYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/MapTypes.hpp"
#include "systems/AutoTiler.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include <vector>
#include <deque>
#include <memory>
#include <string>

namespace rtype {

/**
 * @brief System that manages tile chunks for streaming map rendering
 * 
 * This system:
 * - Loads/unloads chunks based on scroll position
 * - Uses AutoTiler to process tiles
 * - Renders tiles directly (not as ECS entities for performance)
 */
class ChunkManagerSystem : public ISystem {
public:
    ChunkManagerSystem(engine::IGraphicsPlugin& graphics, int screenWidth, int screenHeight);
    virtual ~ChunkManagerSystem() = default;

    void init(Registry& registry) override;
    void shutdown() override;
    void update(Registry& registry, float dt) override;
    
    /**
     * @brief Initialize the chunk manager with configuration
     * @param config Map configuration
     */
    void initWithConfig(const MapConfig& config);

    /**
     * @brief Reset the chunk manager, destroying all wall entities
     * @param registry ECS registry to remove entities from
     *
     * Call this before initWithConfig() when restarting a game to ensure
     * old wall collision entities are properly cleaned up.
     */
    void reset(Registry& registry);
    
    /**
     * @brief Load tile sheet texture
     * @param path Path to tile sheet image
     * @return true if loaded successfully
     */
    bool loadTileSheet(const std::string& path);
    
    /**
     * @brief Load segment data from JSON files
     * @param segmentPaths Vector of paths to segment JSON files
     */
    void loadSegments(const std::vector<std::string>& segmentPaths);
    
    /**
     * @brief Render visible chunks
     * @param scrollX Current scroll position
     */
    void render(float scrollX) const;
    
    /**
     * @brief Get current scroll position
     */
    float getScrollX() const { return static_cast<float>(m_scrollX); }

    /**
     * @brief Set scroll speed
     */
    void setScrollSpeed(float speed) { m_scrollSpeed = speed; }

    /**
     * @brief Get scroll speed
     */
    float getScrollSpeed() const { return m_scrollSpeed; }

    /**
     * @brief Update scroll position
     * Uses double precision internally to avoid floating point accumulation errors
     */
    void advanceScroll(float delta) { m_scrollX += static_cast<double>(delta); }
    
    /**
     * @brief Check if chunk manager is initialized and ready to render
     */
    bool isInitialized() const { return m_initialized; }

private:
    void loadChunk(Registry& registry, int segmentId, int chunkIndex);
    void unloadChunk(Registry& registry, int chunkIndex);
    int getChunksNeeded() const;
    
    engine::IGraphicsPlugin& m_graphics;
    int m_screenWidth;
    int m_screenHeight;
    
    MapConfig m_config;
    AutoTiler m_autoTiler;
    engine::TextureHandle m_tileSheetHandle = engine::INVALID_HANDLE;
    
    std::vector<SegmentData> m_segments;
    std::deque<Chunk> m_activeChunks;

    // Use double for scroll position to avoid floating point precision errors
    // over long play sessions (float loses precision after ~100k pixels)
    double m_scrollX = 0.0;
    float m_scrollSpeed = 60.0f;
    
    int m_currentSegment = 0;
    int m_nextChunkIndex = 0;
    
    bool m_initialized = false;
};

} // namespace rtype

#endif /* !CHUNK_MANAGER_SYSTEM_HPP_ */
