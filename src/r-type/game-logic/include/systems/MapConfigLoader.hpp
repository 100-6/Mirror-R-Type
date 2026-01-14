/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MapConfigLoader - Loads map configuration from JSON files
*/

#ifndef MAP_CONFIG_LOADER_HPP_
#define MAP_CONFIG_LOADER_HPP_

#include "components/MapTypes.hpp"
#include <string>
#include <vector>

namespace rtype {

/**
 * @brief Static utility class to load map configuration from JSON files
 */
class MapConfigLoader {
public:
    /**
     * @brief Load the map index listing all available maps
     * @param path Path to the index.json file (default: assets/maps/index.json)
     * @return Vector of MapInfo for all available maps
     */
    static std::vector<MapInfo> loadMapIndex(const std::string& path = "assets/maps/index.json");
    
    /**
     * @brief Load map configuration for a specific map by ID
     * @param mapId Map folder ID (e.g., "nebula_outpost")
     * @param mapsBasePath Base path to maps folder (default: "assets/maps")
     * @return Populated MapConfig struct with resolved absolute paths
     */
    static MapConfig loadMapById(const std::string& mapId, const std::string& mapsBasePath = "assets/maps");
    
    /**
     * @brief Load map configuration from a JSON file (legacy)
     * @param path Path to the config.json file
     * @return Populated MapConfig struct
     */
    static MapConfig loadConfig(const std::string& path);
    
    /**
     * @brief Load segment data from a JSON file
     * @param path Path to the segment JSON file
     * @return Populated SegmentData struct
     */
    static SegmentData loadSegment(const std::string& path);
    
    /**
     * @brief Get sorted list of segment file paths from a directory
     * @param configDir Path to the config directory containing segments/
     * @return Sorted vector of segment file paths
     */
    static std::vector<std::string> getSegmentPaths(const std::string& configDir);
};

} // namespace rtype

#endif /* !MAP_CONFIG_LOADER_HPP_ */
