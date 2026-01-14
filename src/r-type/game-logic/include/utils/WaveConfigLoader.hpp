/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** WaveConfigLoader - Loads wave configurations from JSON files
*/

#ifndef WAVE_CONFIG_LOADER_HPP_
    #define WAVE_CONFIG_LOADER_HPP_

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "components/GameComponents.hpp"
#include "components/WaveConfig.hpp"

using json = nlohmann::json;

namespace WaveLoader {

/**
 * @brief Represents a single wave with spawn data and trigger conditions
 */
struct Wave {
    int waveNumber = 0;  // Explicit wave number from JSON
    std::vector<WaveSpawnData> spawnData;
    WaveTrigger trigger;
};

/**
 * @brief Configuration for the entire wave system loaded from JSON
 */
struct WaveConfiguration {
    std::vector<Wave> waves;
    float defaultSpawnInterval = WAVE_DEFAULT_SPAWN_INTERVAL;
    bool loopWaves = false;  // Loop back to first wave when complete
};

/**
 * @brief Parse EnemyType from string
 * @param typeStr String representation of enemy type
 * @return Corresponding EnemyType enum value
 */
inline EnemyType parseEnemyType(const std::string& typeStr) {
    if (typeStr == "basic") return EnemyType::Basic;
    if (typeStr == "fast") return EnemyType::Fast;
    if (typeStr == "tank") return EnemyType::Tank;
    if (typeStr == "boss") return EnemyType::Boss;
    return EnemyType::Basic; // Default
}

/**
 * @brief Parse EntitySpawnType from string
 * @param typeStr String representation of entity type
 * @return Corresponding EntitySpawnType enum value
 */
inline EntitySpawnType parseEntityType(const std::string& typeStr) {
    if (typeStr == WAVE_ENTITY_TYPE_ENEMY) return EntitySpawnType::ENEMY;
    if (typeStr == WAVE_ENTITY_TYPE_WALL) return EntitySpawnType::WALL;
    if (typeStr == WAVE_ENTITY_TYPE_OBSTACLE) return EntitySpawnType::OBSTACLE;
    if (typeStr == WAVE_ENTITY_TYPE_POWERUP) return EntitySpawnType::POWERUP;
    return EntitySpawnType::ENEMY; // Default
}

/**
 * @brief Parse SpawnPattern from string
 * @param patternStr String representation of spawn pattern
 * @return Corresponding SpawnPattern enum value
 */
inline SpawnPattern parseSpawnPattern(const std::string& patternStr) {
    if (patternStr == WAVE_PATTERN_SINGLE) return SpawnPattern::SINGLE;
    if (patternStr == WAVE_PATTERN_LINE) return SpawnPattern::LINE;
    if (patternStr == WAVE_PATTERN_GRID) return SpawnPattern::GRID;
    if (patternStr == WAVE_PATTERN_RANDOM) return SpawnPattern::RANDOM;
    if (patternStr == WAVE_PATTERN_FORMATION) return SpawnPattern::FORMATION;
    return SpawnPattern::SINGLE; // Default
}

/**
 * @brief Parse BonusType from string
 * @param typeStr String representation of bonus type
 * @return Corresponding BonusType enum value
 */
inline BonusType parseBonusType(const std::string& typeStr) {
    if (typeStr == "health") return BonusType::HEALTH;
    if (typeStr == "shield") return BonusType::SHIELD;
    if (typeStr == "speed") return BonusType::SPEED;
    if (typeStr == "bonus_weapon") return BonusType::BONUS_WEAPON;
    return BonusType::HEALTH; // Default
}

/**
 * @brief Load wave configuration from JSON file
 * @param filepath Path to the JSON configuration file
 * @return WaveConfiguration structure containing all waves
 * @throws std::runtime_error if file cannot be opened or parsed
 */
WaveConfiguration loadWaveConfig(const std::string& filepath);

/**
 * @brief Validate wave configuration
 * @param config Configuration to validate
 * @return true if valid, false otherwise
 */
bool validateWaveConfig(const WaveConfiguration& config);

} // namespace WaveLoader

#endif /* !WAVE_CONFIG_LOADER_HPP_ */
