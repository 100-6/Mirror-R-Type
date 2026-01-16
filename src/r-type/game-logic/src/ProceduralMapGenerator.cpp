#include "ProceduralMapGenerator.hpp"
#include <algorithm>
#include <iostream>

namespace rtype {

ProceduralMapGenerator::ProceduralMapGenerator(uint32_t seed)
{
    if (seed == 0) {
        // Use true randomness
        std::random_device rd;
        seed = rd();
    }

    rng_.seed(seed);

    // Initialize with default center passage
    lastExitState_.topY = DEFAULT_HEIGHT / 2 - 22;
    lastExitState_.bottomY = DEFAULT_HEIGHT / 2 + 22;
}

void ProceduralMapGenerator::reset()
{
    // Reset to default center passage
    lastExitState_.topY = DEFAULT_HEIGHT / 2 - 22;
    lastExitState_.bottomY = DEFAULT_HEIGHT / 2 + 22;
}

SegmentData ProceduralMapGenerator::generateSegment(
    int segmentId,
    const PathState* entryState,
    const GenerationParams& params
)
{
    std::cout << "[ProceduralGen] Generating segment " << segmentId << std::endl;

    SegmentData segment;
    segment.segmentId = segmentId;
    segment.width = DEFAULT_WIDTH;
    segment.height = DEFAULT_HEIGHT;

    // Initialize empty tile grid
    segment.tiles.resize(DEFAULT_HEIGHT);
    for (auto& row : segment.tiles) {
        row.resize(DEFAULT_WIDTH, 0);  // 0 = empty
    }

    // Determine entry state
    PathState entry;
    if (entryState != nullptr) {
        entry = *entryState;
        std::cout << "[ProceduralGen] Using entry state: topY=" << entry.topY << ", bottomY=" << entry.bottomY << std::endl;
    } else {
        // First segment - start with centered passage
        int center = DEFAULT_HEIGHT / 2;
        int halfPassage = params.minPassageHeight / 2 + 3;  // Add buffer
        entry.topY = center - halfPassage;
        entry.bottomY = center + halfPassage;
        std::cout << "[ProceduralGen] First segment, centered passage: topY=" << entry.topY << ", bottomY=" << entry.bottomY << std::endl;
    }

    // Generate path through this segment
    std::vector<std::pair<int, int>> pathPoints = generatePath(entry, params);

    // Draw walls
    drawWalls(segment.tiles, pathPoints);

    // Add obstacles
    addObstacles(segment.tiles, pathPoints, params);

    // Store exit state for next segment
    lastExitState_.topY = pathPoints.back().first;
    lastExitState_.bottomY = pathPoints.back().second;

    std::cout << "[ProceduralGen] Segment " << segmentId << " complete. Exit state: topY="
              << lastExitState_.topY << ", bottomY=" << lastExitState_.bottomY << std::endl;

    return segment;
}

std::vector<std::pair<int, int>> ProceduralMapGenerator::generatePath(
    const PathState& entryState,
    const GenerationParams& params
)
{
    std::vector<std::pair<int, int>> pathPoints;
    pathPoints.reserve(DEFAULT_WIDTH);

    int currentTop = entryState.topY;
    int currentBottom = entryState.bottomY;

    // Random distributions
    std::uniform_int_distribution<int> centerShiftDist(-2, 2);
    std::uniform_int_distribution<int> widthChangeDist(-1, 1);

    for (int x = 0; x < DEFAULT_WIDTH; ++x) {
        // Vertical movement of entire passage
        int centerShift = centerShiftDist(rng_);

        // Width variation of passage
        int widthChange = widthChangeDist(rng_);

        // Apply gradual changes
        int newTop = currentTop + centerShift + widthChange;
        int newBottom = currentBottom + centerShift - widthChange;

        // Ensure minimum passage height
        int passageHeight = newBottom - newTop;
        if (passageHeight < params.minPassageHeight) {
            int extra = params.minPassageHeight - passageHeight;
            newTop -= extra / 2;
            newBottom += extra - (extra / 2);
        }

        // Keep within bounds (leave wall space at top and bottom)
        const int minWall = 3;
        const int maxTop = DEFAULT_HEIGHT - params.minPassageHeight - minWall;
        newTop = clamp(newTop, minWall, maxTop);
        newBottom = clamp(newBottom, newTop + params.minPassageHeight, DEFAULT_HEIGHT - minWall);

        // Smooth transitions (prevent abrupt changes)
        const int maxChange = 2;
        if (std::abs(newTop - currentTop) > maxChange) {
            newTop = currentTop + maxChange * (newTop > currentTop ? 1 : -1);
        }
        if (std::abs(newBottom - currentBottom) > maxChange) {
            newBottom = currentBottom + maxChange * (newBottom > currentBottom ? 1 : -1);
        }

        pathPoints.push_back({newTop, newBottom});
        currentTop = newTop;
        currentBottom = newBottom;
    }

    return pathPoints;
}

void ProceduralMapGenerator::drawWalls(
    std::vector<std::vector<int>>& tiles,
    const std::vector<std::pair<int, int>>& pathPoints
)
{
    int wallTilesCount = 0;
    for (size_t x = 0; x < pathPoints.size() && x < tiles[0].size(); ++x) {
        int topY = pathPoints[x].first;
        int bottomY = pathPoints[x].second;

        // Draw top wall
        for (int y = 0; y < topY; ++y) {
            tiles[y][x] = 1;  // 1 = wall
            wallTilesCount++;
        }

        // Draw bottom wall
        for (int y = bottomY; y < DEFAULT_HEIGHT; ++y) {
            tiles[y][x] = 1;  // 1 = wall
            wallTilesCount++;
        }
    }
    std::cout << "[ProceduralGen] Drew " << wallTilesCount << " wall tiles" << std::endl;
}

void ProceduralMapGenerator::addObstacles(
    std::vector<std::vector<int>>& tiles,
    const std::vector<std::pair<int, int>>& pathPoints,
    const GenerationParams& params
)
{
    std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
    std::uniform_int_distribution<int> lengthDist(1, params.maxStalactiteLength);

    // Skip edges to avoid blocking entrance/exit
    for (size_t x = 2; x < pathPoints.size() - 2 && x < tiles[0].size() - 2; ++x) {
        int topY = pathPoints[x].first;
        int bottomY = pathPoints[x].second;
        int passageHeight = bottomY - topY;

        // Only add obstacles if passage is wide enough
        if (passageHeight < params.minPassageHeight + 4) {
            continue;
        }

        // Calculate safe zone to preserve minimum passage
        int maxObstacleLength = (passageHeight - params.minPassageHeight) / 2;
        if (maxObstacleLength <= 0) {
            continue;
        }

        // Stalactite from ceiling
        if (chanceDist(rng_) < params.stalactiteChance) {
            int length = std::min(lengthDist(rng_), maxObstacleLength);
            for (int dy = 0; dy < length; ++dy) {
                int y = topY + dy;
                if (y >= 0 && y < DEFAULT_HEIGHT && y < bottomY - params.minPassageHeight) {
                    tiles[y][x] = 1;
                }
            }
        }

        // Stalagmite from floor
        if (chanceDist(rng_) < params.stalactiteChance) {
            int length = std::min(lengthDist(rng_), maxObstacleLength);
            for (int dy = 0; dy < length; ++dy) {
                int y = bottomY - 1 - dy;
                if (y >= 0 && y < DEFAULT_HEIGHT && y > topY + params.minPassageHeight) {
                    tiles[y][x] = 1;
                }
            }
        }
    }
}

} // namespace rtype
