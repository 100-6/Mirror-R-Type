/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AutoTiler - Implementation
*/

#include "systems/AutoTiler.hpp"

namespace rtype {

void AutoTiler::setWallSourceRects(const std::unordered_map<std::string, SourceRect>& rects) {
    m_wallSourceRects = rects;
}

bool AutoTiler::hasWall(const TileGrid& grid, int x, int y) const {
    // Out of bounds is considered as "no wall" for edge detection
    if (y < 0 || y >= static_cast<int>(grid.size())) {
        return false;
    }
    if (x < 0 || x >= static_cast<int>(grid[y].size())) {
        return false;
    }
    // Any non-empty tile counts as a wall for connectivity
    return grid[y][x] != TileType::EMPTY;
}

SourceRect AutoTiler::getWallSourceRect(const TileGrid& grid, int x, int y) const {
    // Check all 4 neighbors
    bool top    = hasWall(grid, x, y - 1);
    bool bottom = hasWall(grid, x, y + 1);
    bool left   = hasWall(grid, x - 1, y);
    bool right  = hasWall(grid, x + 1, y);

    // 0. Isolated (No neighbors)
    if (!top && !bottom && !left && !right) {
        auto it = m_wallSourceRects.find("isolated");
        return it != m_wallSourceRects.end() ? it->second : m_wallSourceRects.at("center");
    }

    // 1. Linear segments (Tubes) - Horizontal & Vertical
    // Vertical: Neighbors Top & Bottom, but NOT Left/Right
    if (top && bottom && !left && !right) {
        auto it = m_wallSourceRects.find("vertical");
        if (it != m_wallSourceRects.end()) return it->second;
    }
    // Horizontal: Neighbors Left & Right, but NOT Top/Bottom
    if (left && right && !top && !bottom) {
        auto it = m_wallSourceRects.find("horizontal");
        if (it != m_wallSourceRects.end()) return it->second;
    }

    // 2. Ends (Caps)
    // Top End (connected only to bottom)
    if (!top && bottom && !left && !right) {
        auto it = m_wallSourceRects.find("verticalTop");
        if (it != m_wallSourceRects.end()) return it->second;
    }
    // Bottom End (connected only to top)
    if (top && !bottom && !left && !right) {
        auto it = m_wallSourceRects.find("verticalBottom");
        if (it != m_wallSourceRects.end()) return it->second;
    }
    // Left End (connected only to right)
    if (!top && !bottom && !left && right) {
        auto it = m_wallSourceRects.find("horizontalLeft");
        if (it != m_wallSourceRects.end()) return it->second;
    }
    // Right End (connected only to left)
    if (!top && !bottom && left && !right) {
        auto it = m_wallSourceRects.find("horizontalRight");
        if (it != m_wallSourceRects.end()) return it->second;
    }

    // 3. Standard 9-slice (Outer Corners & Edges) logic
    // Corners
    if (!top && !left) {
        auto it = m_wallSourceRects.find("topLeft");
        return it != m_wallSourceRects.end() ? it->second : SourceRect{};
    }
    if (!top && !right) {
        auto it = m_wallSourceRects.find("topRight");
        return it != m_wallSourceRects.end() ? it->second : SourceRect{};
    }
    if (!bottom && !left) {
        auto it = m_wallSourceRects.find("bottomLeft");
        return it != m_wallSourceRects.end() ? it->second : SourceRect{};
    }
    if (!bottom && !right) {
        auto it = m_wallSourceRects.find("bottomRight");
        return it != m_wallSourceRects.end() ? it->second : SourceRect{};
    }
    
    // Edges
    if (!top) {
        auto it = m_wallSourceRects.find("top");
        return it != m_wallSourceRects.end() ? it->second : SourceRect{};
    }
    if (!bottom) {
        auto it = m_wallSourceRects.find("bottom");
        return it != m_wallSourceRects.end() ? it->second : SourceRect{};
    }
    if (!left) {
        auto it = m_wallSourceRects.find("left");
        return it != m_wallSourceRects.end() ? it->second : SourceRect{};
    }
    if (!right) {
        auto it = m_wallSourceRects.find("right");
        return it != m_wallSourceRects.end() ? it->second : SourceRect{};
    }
    
    // Center
    auto it = m_wallSourceRects.find("center");
    return it != m_wallSourceRects.end() ? it->second : SourceRect{};
}

std::vector<std::vector<Tile>> AutoTiler::processTileGrid(const TileGrid& grid) const {
    std::vector<std::vector<Tile>> result;
    result.resize(grid.size());
    
    for (int y = 0; y < static_cast<int>(grid.size()); ++y) {
        result[y].resize(grid[y].size());
        for (int x = 0; x < static_cast<int>(grid[y].size()); ++x) {
            Tile& tile = result[y][x];
            tile.type = grid[y][x];
            
            if (tile.type == TileType::WALL) {
                // Determine orientation for walls
                tile.sourceRect = getWallSourceRect(grid, x, y);
            } 
            else if (tile.type == TileType::DIAGONAL || tile.type == TileType::DIAGONAL_MIRROR) {
                // Check all neighbors (walls and other diagonals count)
                bool top    = hasWall(grid, x, y - 1);
                bool bottom = hasWall(grid, x, y + 1);
                bool left   = hasWall(grid, x - 1, y);
                bool right  = hasWall(grid, x + 1, y);
                
                // Check if neighbors are specifically WALLS (not diagonals)
                auto isStrictWall = [&](int nx, int ny) {
                    if (ny < 0 || ny >= static_cast<int>(grid.size())) return false;
                    if (nx < 0 || nx >= static_cast<int>(grid[ny].size())) return false;
                    return grid[ny][nx] == TileType::WALL;
                };
                bool topWall    = isStrictWall(x, y - 1);
                bool bottomWall = isStrictWall(x, y + 1);
                bool leftWall   = isStrictWall(x - 1, y);
                bool rightWall  = isStrictWall(x + 1, y);
                
                std::string key = "";

                if (tile.type == TileType::DIAGONAL) {
                    // Type 2: Standard Spikes
                    if (bottomWall && rightWall) { key = "spikeTop"; }
                    else if (bottomWall && leftWall) { key = "spikeTop"; tile.flipH = true; }
                    else if (bottomWall) key = "spikeTop";
                    else if (leftWall) { key = "spikeTop"; tile.flipH = true; }
                    else if (topWall) key = "spikeBottom";
                    else if (rightWall) { key = "spikeBottom"; tile.flipH = true; }
                    // Fallback to any neighbor
                    else if (bottom) key = "spikeTop";
                    else if (left) { key = "spikeTop"; tile.flipH = true; }
                    else if (top) key = "spikeBottom";
                    else if (right) { key = "spikeBottom"; tile.flipH = true; }
                    else key = "spikeTop";
                }
                else { // DIAGONAL_MIRROR (3)
                    // Type 3: Vertically mirrored version
                    tile.flipV = true;
                    if (topWall && rightWall) { key = "spikeTop"; }
                    else if (topWall && leftWall) { key = "spikeTop"; tile.flipH = true; }
                    else if (topWall) key = "spikeTop";
                    else if (rightWall) { key = "spikeTop"; tile.flipH = true; }
                    else if (bottomWall) key = "spikeBottom";
                    else if (leftWall) { key = "spikeBottom"; tile.flipH = true; }
                    // Fallback to any neighbor
                    else if (top) key = "spikeTop";
                    else if (right) { key = "spikeTop"; tile.flipH = true; }
                    else if (bottom) key = "spikeBottom";
                    else if (left) { key = "spikeBottom"; tile.flipH = true; }
                    else key = "spikeTop";
                }

                auto it = m_wallSourceRects.find(key);
                tile.sourceRect = it != m_wallSourceRects.end() ? it->second : SourceRect{};
            }
        }
    }
    
    return result;
}

} // namespace rtype
