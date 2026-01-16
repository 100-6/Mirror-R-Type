#pragma once

#include "components/MapTypes.hpp"
#include <random>
#include <vector>
#include <utility>
#include <cstdint>

namespace rtype {

/**
 * Procedural map generator for creating dynamic map segments at runtime.
 * Generates navigable tile grids with stalactites/stalagmites and ensures
 * continuity between consecutive segments.
 */
class ProceduralMapGenerator {
public:
    /**
     * Configuration parameters for procedural generation.
     */
    struct GenerationParams {
        int minPassageHeight = 45;      // Minimum vertical space for ship passage
        float stalactiteChance = 0.25f; // Probability of stalactite/stalagmite (0.0-1.0)
        int maxStalactiteLength = 6;    // Maximum length of obstacles
        int pathVariation = 5;          // Amount of vertical path variation
    };

    /**
     * Represents the passage state at the edge of a segment.
     * Used to ensure smooth transitions between segments.
     */
    struct PathState {
        int topY;      // Y coordinate of top boundary
        int bottomY;   // Y coordinate of bottom boundary

        int centerY() const { return (topY + bottomY) / 2; }
        int passageHeight() const { return bottomY - topY; }
    };

    /**
     * Constructor with optional seed for reproducibility.
     * @param seed Random seed (0 = use random_device for true randomness)
     */
    explicit ProceduralMapGenerator(uint32_t seed = 0);

    /**
     * Generate a map segment with the given parameters.
     *
     * @param segmentId Unique identifier for this segment
     * @param entryState Path state from previous segment (nullptr for first segment)
     * @param params Generation parameters
     * @return Generated segment data with tile grid
     */
    SegmentData generateSegment(
        int segmentId,
        const PathState* entryState,
        const GenerationParams& params
    );

    /**
     * Get the exit state of the last generated segment.
     * Used to maintain continuity when generating the next segment.
     */
    const PathState& getLastExitState() const { return lastExitState_; }

    /**
     * Reset the generator to start fresh.
     */
    void reset();

private:
    static constexpr int DEFAULT_WIDTH = 60;
    static constexpr int DEFAULT_HEIGHT = 68;

    std::mt19937 rng_;              // Random number generator (Mersenne Twister)
    PathState lastExitState_;       // Exit state of last generated segment

    /**
     * Generate path points defining the navigable corridor through the segment.
     * Returns vector of (topY, bottomY) pairs for each column.
     */
    std::vector<std::pair<int, int>> generatePath(
        const PathState& entryState,
        const GenerationParams& params
    );

    /**
     * Draw wall tiles above and below the path.
     */
    void drawWalls(
        std::vector<std::vector<int>>& tiles,
        const std::vector<std::pair<int, int>>& pathPoints
    );

    /**
     * Add stalactites (ceiling) and stalagmites (floor) obstacles.
     */
    void addObstacles(
        std::vector<std::vector<int>>& tiles,
        const std::vector<std::pair<int, int>>& pathPoints,
        const GenerationParams& params
    );

    /**
     * Clamp value between min and max.
     */
    template<typename T>
    static T clamp(T value, T min, T max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
};

} // namespace rtype
