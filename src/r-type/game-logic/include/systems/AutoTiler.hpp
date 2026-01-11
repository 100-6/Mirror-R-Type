/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AutoTiler - Computes tile source rects based on neighbor analysis
*/

#ifndef AUTO_TILER_HPP_
#define AUTO_TILER_HPP_

#include "components/MapTypes.hpp"
#include <vector>
#include <unordered_map>

namespace rtype {

/**
 * @brief Auto-tiler that computes source rectangles for tiles based on neighbors
 * 
 * This class analyzes the neighboring tiles to determine which sprite
 * variant to use (corners, edges, centers, etc.)
 */
class AutoTiler {
public:
    AutoTiler() = default;
    
    /**
     * @brief Set the source rects for wall parts
     * @param rects Map of wall part names to source rectangles
     */
    void setWallSourceRects(const std::unordered_map<std::string, SourceRect>& rects);
    
    /**
     * @brief Get the appropriate source rect for a wall tile based on neighbors
     * @param grid The tile grid
     * @param x X position in grid
     * @param y Y position in grid
     * @return Source rectangle for the tile
     */
    SourceRect getWallSourceRect(const TileGrid& grid, int x, int y) const;
    
    /**
     * @brief Process an entire grid and compute source rects for all tiles
     * @param grid The input tile grid
     * @return Grid of processed tiles with source rects
     */
    std::vector<std::vector<Tile>> processTileGrid(const TileGrid& grid) const;

private:
    bool hasWall(const TileGrid& grid, int x, int y) const;
    
    std::unordered_map<std::string, SourceRect> m_wallSourceRects;
};

} // namespace rtype

#endif /* !AUTO_TILER_HPP_ */
