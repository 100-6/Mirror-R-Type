/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MapConfigLoader - Implementation
*/

#include "systems/MapConfigLoader.hpp"
#include "AssetsPaths.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace rtype {

std::vector<MapInfo> MapConfigLoader::loadMapIndex(const std::string& path) {
    std::vector<MapInfo> maps;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open map index: " << path << std::endl;
        return maps;
    }
    
    try {
        json j = json::parse(file);
        
        if (j.contains("maps")) {
            for (const auto& mapEntry : j["maps"]) {
                MapInfo info;
                info.id = mapEntry.value("id", "");
                info.name = mapEntry.value("name", info.id);
                info.description = mapEntry.value("description", "");
                info.difficulty = mapEntry.value("difficulty", 1);
                info.thumbnailPath = mapEntry.value("thumbnail", "");
                info.wavesConfigPath = mapEntry.value("wavesConfig", "");
                maps.push_back(info);
            }
        }
        
        std::cout << "[MapConfigLoader] Loaded " << maps.size() << " maps from index" << std::endl;
        
    } catch (const json::exception& e) {
        std::cerr << "JSON parse error in map index: " << e.what() << std::endl;
    }
    
    return maps;
}

MapConfig MapConfigLoader::loadMapById(const std::string& mapId, const std::string& mapsBasePath) {
    std::string mapFolder = mapsBasePath + "/" + mapId;
    std::string configPath = mapFolder + "/map.json";
    
    MapConfig config;
    
    std::ifstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open map config: " << configPath << std::endl;
        return config;
    }
    
    try {
        json j = json::parse(file);
        
        config.id = j.value("id", mapId);
        config.name = j.value("name", mapId);
        config.basePath = mapFolder;
        config.tileSize = j.value("tileSize", 16);
        config.chunkWidth = j.value("chunkWidth", 30);
        
        if (j.contains("scroll")) {
            config.baseScrollSpeed = j["scroll"].value("baseSpeed", 60.0f);
        }
        
        // Tilesheet with relative path resolution
        if (j.contains("tileSheet")) {
            auto& ts = j["tileSheet"];
            std::string relativePath = ts.value("path", "tiles/TileSheet.png");
            config.tileSheetPath = mapFolder + "/" + relativePath;
            
            if (ts.contains("walls")) {
                for (auto& [key, val] : ts["walls"].items()) {
                    SourceRect rect;
                    rect.x = val.value("x", 0);
                    rect.y = val.value("y", 0);
                    rect.w = val.value("w", 16);
                    rect.h = val.value("h", 16);
                    config.wallSourceRects[key] = rect;
                }
            }
        }
        
        // Parallax layers with relative path resolution
        if (j.contains("parallaxLayers")) {
            for (auto& layer : j["parallaxLayers"]) {
                ParallaxLayerConfig pl;
                std::string relativePath = layer.value("path", "");
                pl.path = mapFolder + "/" + relativePath;
                pl.speedFactor = layer.value("speedFactor", 1.0f);
                config.parallaxLayers.push_back(pl);
            }
        }

        // Procedural generation configuration
        if (j.contains("procedural")) {
            auto& proc = j["procedural"];
            config.procedural.enabled = proc.value("enabled", false);
            config.procedural.seed = proc.value("seed", 0u);

            if (proc.contains("params")) {
                auto& params = proc["params"];
                config.procedural.minPassageHeight = params.value("minPassageHeight", 45);
                config.procedural.stalactiteChance = params.value("stalactiteChance", 0.25f);
                config.procedural.maxStalactiteLength = params.value("maxStalactiteLength", 6);
                config.procedural.pathVariation = params.value("pathVariation", 5);
            }

            if (config.procedural.enabled) {
                std::cout << "[MapConfigLoader] Procedural generation enabled (seed: "
                          << config.procedural.seed << ")" << std::endl;
            }
        }

        std::cout << "[MapConfigLoader] Loaded map: " << config.name << " from " << mapFolder << std::endl;
        
    } catch (const json::exception& e) {
        std::cerr << "JSON parse error in map config: " << e.what() << std::endl;
    }
    
    return config;
}

MapConfig MapConfigLoader::loadConfig(const std::string& path) {
    MapConfig config;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << path << std::endl;
        return config;
    }
    
    try {
        json j = json::parse(file);
        
        // Tile settings
        config.tileSize = j.value("tileSize", 16);
        config.chunkWidth = j.value("chunkWidth", 30);
        
        // Scroll settings
        if (j.contains("scroll")) {
            config.baseScrollSpeed = j["scroll"].value("baseSpeed", 60.0f);
        }
        
        // Tilesheet settings
        if (j.contains("tileSheet")) {
            auto& ts = j["tileSheet"];
            config.tileSheetPath = ts.value("path", "assets/sprite/tiles/tilesheet.png");
            
            if (ts.contains("walls")) {
                auto& walls = ts["walls"];
                for (auto& [key, val] : walls.items()) {
                    SourceRect rect;
                    rect.x = val.value("x", 0);
                    rect.y = val.value("y", 0);
                    rect.w = val.value("w", 16);
                    rect.h = val.value("h", 16);
                    config.wallSourceRects[key] = rect;
                }
            }
        }
        
        // Parallax layers
        if (j.contains("parallaxLayers")) {
            for (auto& layer : j["parallaxLayers"]) {
                ParallaxLayerConfig pl;
                pl.path = layer.value("path", "");
                pl.speedFactor = layer.value("speedFactor", 1.0f);
                config.parallaxLayers.push_back(pl);
            }
        }
        
    } catch (const json::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }
    
    return config;
}

SegmentData MapConfigLoader::loadSegment(const std::string& path) {
    SegmentData segment;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open segment file: " << path << std::endl;
        return segment;
    }
    
    try {
        json j = json::parse(file);
        
        segment.segmentId = j.value("segmentId", 0);
        segment.width = j.value("width", 30);
        segment.height = j.value("height", 68);
        
        if (j.contains("tiles")) {
            segment.tiles = j["tiles"].get<std::vector<std::vector<int>>>();
        }
        
    } catch (const json::exception& e) {
        std::cerr << "JSON parse error in segment: " << e.what() << std::endl;
    }
    
    return segment;
}

std::vector<std::string> MapConfigLoader::getSegmentPaths(const std::string& configDir) {
    std::vector<std::string> paths;
    
    std::string segmentsDir = configDir + "/segments";
    
    if (!fs::exists(segmentsDir)) {
        // Try without /segments subdirectory
        segmentsDir = configDir;
        if (!fs::exists(segmentsDir)) {
            std::cerr << "Segments directory not found: " << segmentsDir << std::endl;
            return paths;
        }
    }
    
    for (const auto& entry : fs::directory_iterator(segmentsDir)) {
        if (entry.path().extension() == ".json") {
            paths.push_back(entry.path().string());
        }
    }
    
    // Sort to ensure consistent loading order
    std::sort(paths.begin(), paths.end());
    
    return paths;
}

} // namespace rtype
