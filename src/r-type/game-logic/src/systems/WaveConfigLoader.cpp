/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** WaveConfigLoader implementation
*/

#include "systems/WaveConfigLoader.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace WaveLoader {

WaveConfiguration loadWaveConfig(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open wave configuration file: " + filepath);
    }

    json j;
    try {
        file >> j;
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
    }

    WaveConfiguration config;

    // Load global settings
    if (j.contains("defaultSpawnInterval")) {
        config.defaultSpawnInterval = j["defaultSpawnInterval"].get<float>();

        // Clamp to valid range
        if (config.defaultSpawnInterval < WAVE_MIN_SPAWN_INTERVAL) {
            config.defaultSpawnInterval = WAVE_MIN_SPAWN_INTERVAL;
        } else if (config.defaultSpawnInterval > WAVE_MAX_SPAWN_INTERVAL) {
            config.defaultSpawnInterval = WAVE_MAX_SPAWN_INTERVAL;
        }
    }

    if (j.contains("loopWaves")) {
        config.loopWaves = j["loopWaves"].get<bool>();
    }

    // Load waves
    if (!j.contains("waves") || !j["waves"].is_array()) {
        throw std::runtime_error("Wave configuration must contain 'waves' array");
    }

    const auto& wavesArray = j["waves"];
    if (wavesArray.size() > WAVE_MAX_ACTIVE_WAVES) {
        std::cerr << "Warning: Wave count exceeds maximum (" << WAVE_MAX_ACTIVE_WAVES
                  << "), truncating..." << std::endl;
    }

    for (size_t i = 0; i < wavesArray.size() && i < WAVE_MAX_ACTIVE_WAVES; ++i) {
        const auto& waveJson = wavesArray[i];
        Wave wave;

        // Parse wave number
        if (waveJson.contains("waveNumber")) {
            wave.waveNumber = waveJson["waveNumber"].get<int>();
        } else {
            wave.waveNumber = static_cast<int>(i + 1);  // Default to index + 1
        }

        // Parse trigger conditions
        if (waveJson.contains("trigger")) {
            const auto& trigger = waveJson["trigger"];

            if (trigger.contains("scrollDistance")) {
                wave.trigger.scrollDistance = trigger["scrollDistance"].get<float>();
            }

            if (trigger.contains("timeDelay")) {
                wave.trigger.timeDelay = trigger["timeDelay"].get<float>();
            }
        }

        // Parse spawn data
        if (!waveJson.contains("spawns") || !waveJson["spawns"].is_array()) {
            std::cerr << "Warning: Wave " << i << " has no spawns array, skipping..." << std::endl;
            continue;
        }

        const auto& spawnsArray = waveJson["spawns"];
        if (spawnsArray.size() > WAVE_MAX_ENTITIES_PER_WAVE) {
            std::cerr << "Warning: Wave " << i << " exceeds max entities ("
                      << WAVE_MAX_ENTITIES_PER_WAVE << "), truncating..." << std::endl;
        }

        for (size_t j = 0; j < spawnsArray.size() && j < WAVE_MAX_ENTITIES_PER_WAVE; ++j) {
            const auto& spawnJson = spawnsArray[j];
            WaveSpawnData spawnData;

            // Parse entity type (required)
            if (spawnJson.contains("type")) {
                std::string typeStr = spawnJson["type"].get<std::string>();
                spawnData.entityType = parseEntityType(typeStr);
            }

            // Parse enemy subtype (for enemies)
            if (spawnData.entityType == EntitySpawnType::ENEMY && spawnJson.contains("enemyType")) {
                std::string enemyTypeStr = spawnJson["enemyType"].get<std::string>();
                spawnData.enemyType = parseEnemyType(enemyTypeStr);
            }

            // Parse position
            if (spawnJson.contains("positionX")) {
                spawnData.positionX = spawnJson["positionX"].get<float>();
            } else {
                spawnData.positionX = WAVE_SPAWN_AHEAD_DISTANCE;
            }

            if (spawnJson.contains("positionY")) {
                spawnData.positionY = spawnJson["positionY"].get<float>();
            } else {
                spawnData.positionY = (WAVE_SPAWN_MIN_Y + WAVE_SPAWN_MAX_Y) / 2.0f; // Center
            }

            // Clamp Y position
            if (spawnData.positionY < WAVE_SPAWN_MIN_Y) {
                spawnData.positionY = WAVE_SPAWN_MIN_Y;
            } else if (spawnData.positionY > WAVE_SPAWN_MAX_Y) {
                spawnData.positionY = WAVE_SPAWN_MAX_Y;
            }

            // Parse count
            if (spawnJson.contains("count")) {
                spawnData.count = spawnJson["count"].get<int>();
                if (spawnData.count < 1) spawnData.count = 1;
            }

            // Parse spawn pattern
            if (spawnJson.contains("pattern")) {
                std::string patternStr = spawnJson["pattern"].get<std::string>();
                spawnData.pattern = parseSpawnPattern(patternStr);
            }

            // Parse spacing (for patterns)
            if (spawnJson.contains("spacing")) {
                spawnData.spacing = spawnJson["spacing"].get<float>();
            } else {
                // Use default spacing based on pattern
                if (spawnData.pattern == SpawnPattern::LINE ||
                    spawnData.pattern == SpawnPattern::FORMATION) {
                    spawnData.spacing = WAVE_FORMATION_SPACING_Y;
                } else if (spawnData.pattern == SpawnPattern::GRID) {
                    spawnData.spacing = WAVE_FORMATION_SPACING_X;
                }
            }

            wave.spawnData.push_back(spawnData);
        }

        config.waves.push_back(wave);
    }

    if (config.waves.empty()) {
        throw std::runtime_error("No valid waves found in configuration file");
    }

    return config;
}

bool validateWaveConfig(const WaveConfiguration& config) {
    if (config.waves.empty()) {
        std::cerr << "Validation failed: No waves in configuration" << std::endl;
        return false;
    }

    if (config.defaultSpawnInterval < WAVE_MIN_SPAWN_INTERVAL ||
        config.defaultSpawnInterval > WAVE_MAX_SPAWN_INTERVAL) {
        std::cerr << "Validation failed: Invalid spawn interval" << std::endl;
        return false;
    }

    for (size_t i = 0; i < config.waves.size(); ++i) {
        const auto& wave = config.waves[i];

        if (wave.spawnData.empty()) {
            std::cerr << "Validation warning: Wave " << i << " has no spawn data" << std::endl;
        }

        for (size_t j = 0; j < wave.spawnData.size(); ++j) {
            const auto& spawn = wave.spawnData[j];

            if (spawn.count < 1) {
                std::cerr << "Validation failed: Wave " << i << " spawn " << j
                          << " has invalid count" << std::endl;
                return false;
            }

            if (spawn.positionY < WAVE_SPAWN_MIN_Y || spawn.positionY > WAVE_SPAWN_MAX_Y) {
                std::cerr << "Validation warning: Wave " << i << " spawn " << j
                          << " has Y position outside bounds" << std::endl;
            }
        }
    }

    return true;
}

} // namespace WaveLoader
