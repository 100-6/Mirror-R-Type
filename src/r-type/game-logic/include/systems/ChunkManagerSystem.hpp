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
#include <unordered_map>

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
    virtual ~ChunkManagerSystem();  // Must be defined in .cpp where ProceduralMapGenerator is complete

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
     * @brief Render visible chunks using internal scroll position
     * Uses the internally tracked scroll position to ensure consistency
     * between chunk loading/unloading and rendering
     */
    void render() const;
    
    /**
     * @brief Get current render scroll position (may be extrapolated)
     */
    float getScrollX() const;

    /**
     * @brief Get confirmed scroll position from server
     */
    double getConfirmedScrollX() const;

    /**
     * @brief Set scroll speed and update existing wall velocities
     */
    void setScrollSpeed(float speed, Registry* registry = nullptr);

    /**
     * @brief Get scroll speed
     */
    float getScrollSpeed() const;

    /**
     * @brief Update render scroll position incrementally for smooth visual interpolation
     * Uses double precision internally to avoid floating point accumulation errors
     */
    void advanceRenderScroll(float delta);

    /**
     * @brief Set confirmed scroll position from server
     * This is the authoritative scroll used for chunk loading/unloading decisions
     * Also resets render scroll to this value for synchronization
     */
    void setConfirmedScrollX(double scroll);

    /**
     * @brief Legacy method - calls setConfirmedScrollX
     * @deprecated Use setConfirmedScrollX instead
     */
    void setScrollX(double scroll);

    /**
     * @brief Check if chunk manager is initialized and ready to render
     */
    bool isInitialized() const;

    /**
     * @brief Set procedural seed (for client-server synchronization)
     * @param seed Seed to use for procedural generation (0 = random)
     */
    void setProceduralSeed(uint32_t seed);

private:
    void loadChunk(Registry& registry, int segmentId, int chunkIndex);
    void unloadChunk(Registry& registry, int chunkIndex);
    int getChunksNeeded() const;

    // Procedural generation helper
    SegmentData* getOrGenerateSegment(int segmentId);
    
    engine::IGraphicsPlugin& m_graphics;
    int m_screenWidth;
    int m_screenHeight;
    
    MapConfig m_config;
    AutoTiler m_autoTiler;
    engine::TextureHandle m_tileSheetHandle = engine::INVALID_HANDLE;

    std::vector<SegmentData> m_segments;  // For static maps
    std::unordered_map<int, SegmentData> m_generatedSegments;  // For procedural maps
    std::deque<Chunk> m_activeChunks;

    // Procedural generation
    bool m_proceduralEnabled = false;
    std::unique_ptr<class ProceduralMapGenerator> m_generator;
    ProceduralConfig m_proceduralConfig;

    // Two scroll positions to prevent visual stuttering:
    // - m_confirmedScrollX: Authoritative scroll from server, used for chunk loading decisions
    // - m_renderScrollX: Interpolated scroll for smooth rendering (may drift slightly)
    // Both use double precision to avoid floating point errors over long play sessions
    double m_confirmedScrollX = 0.0;  // Server-authoritative
    double m_renderScrollX = 0.0;     // For smooth rendering
    float m_scrollSpeed = 60.0f;
    
    int m_currentSegment = 0;
    int m_nextChunkIndex = 0;

    bool m_initialized = false;
    bool m_transitionLock = false;  // Prevents chunk loading during level transitions
};

} // namespace rtype

#endif /* !CHUNK_MANAGER_SYSTEM_HPP_ */
