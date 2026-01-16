/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LevelManager - Manages level configuration loading and access
*/

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "components/LevelComponents.hpp"
#include "WaveManager.hpp"

namespace rtype::server {

// ============================================================================
// LEVEL CONFIGURATION STRUCTURES
// ============================================================================

/**
 * @brief Boss configuration for a level
 */
struct BossConfig {
    std::string boss_name;
    float spawn_scroll_distance;
    float spawn_position_x;
    float spawn_position_y;
    std::string enemy_type;                        // "boss"
    uint8_t total_phases;
    std::string script_path;                       // Lua script path for boss behavior
    std::vector<game::BossPhaseConfig> phases;

    BossConfig()
        : boss_name("Boss")
        , spawn_scroll_distance(0.0f)
        , spawn_position_x(1600.0f)
        , spawn_position_y(540.0f)
        , enemy_type("boss")
        , total_phases(3)
        , script_path("boss/boss1_mars_guardian.lua")
    {}
};

/**
 * @brief Phase grouping for waves
 */
struct PhaseConfig {
    uint32_t phase_number;
    std::string phase_name;
    float scroll_start;
    float scroll_end;
    std::string difficulty;               // "easy", "medium", "hard"
    std::vector<Wave> waves;

    PhaseConfig()
        : phase_number(1)
        , phase_name("Phase 1")
        , scroll_start(0.0f)
        , scroll_end(1000.0f)
        , difficulty("easy")
    {}
};

/**
 * @brief Complete level configuration
 */
struct LevelConfig {
    uint8_t level_id;
    std::string level_name;
    std::string level_description;
    uint16_t map_id;
    float base_scroll_speed;
    float total_scroll_distance;
    uint32_t total_chunks;           // New chunk-based duration

    // Checkpoints removed
    std::vector<PhaseConfig> phases;
    BossConfig boss;

    LevelConfig()
        : level_id(1)
        , level_name("Level 1")
        , level_description("")
        , map_id(1)
        , base_scroll_speed(60.0f)
        , total_scroll_distance(8000.0f)
        , total_chunks(20)           // Default 20 chunks
    {}
};

// ============================================================================
// LEVEL MANAGER CLASS
// ============================================================================

/**
 * @brief Manages level configuration loading and provides access to level data
 *
 * The LevelManager is responsible for:
 * - Loading level JSON files
 * - Parsing level structure (phases, waves, boss config, checkpoints)
 * - Providing level data to systems (LevelSystem, BossSystem, CheckpointSystem)
 * - Managing level file paths
 */
class LevelManager {
public:
    LevelManager();
    ~LevelManager() = default;

    // === Configuration Loading ===

    /**
     * @brief Load level configuration from JSON file
     * @param filepath Path to level JSON file
     * @return true if successful, false otherwise
     */
    bool load_from_file(const std::string& filepath);

    /**
     * @brief Load level by level ID (1-3)
     * @param level_id Level number (1, 2, or 3)
     * @return true if successful, false otherwise
     */
    bool load_level(uint8_t level_id);

    // === Level Data Access ===

    const LevelConfig& get_level_config() const { return config_; }

    uint8_t get_level_id() const { return config_.level_id; }
    const std::string& get_level_name() const { return config_.level_name; }
    const std::string& get_level_description() const { return config_.level_description; }
    float get_base_scroll_speed() const { return config_.base_scroll_speed; }
    float get_total_scroll_distance() const { return config_.total_scroll_distance; }
    uint32_t get_total_chunks() const { return config_.total_chunks; }

    // === Checkpoint Access ===

    // Checkpoint Access Removed


    // === Phase Access ===

    const std::vector<PhaseConfig>& get_phases() const { return config_.phases; }
    uint32_t get_phase_count() const { return config_.phases.size(); }
    const PhaseConfig& get_phase(uint32_t index) const { return config_.phases[index]; }

    /**
     * @brief Check if all phases are complete
     */
    bool all_phases_complete(uint32_t current_phase_index) const {
        return current_phase_index >= config_.phases.size();
    }

    // === Wave Access (for specific phase) ===

    uint32_t get_wave_count_in_phase(uint32_t phase_index) const {
        if (phase_index >= config_.phases.size()) return 0;
        return config_.phases[phase_index].waves.size();
    }

    const Wave& get_wave_in_phase(uint32_t phase_index, uint32_t wave_index) const {
        return config_.phases[phase_index].waves[wave_index];
    }

    // === Boss Access ===

    const BossConfig& get_boss_config() const { return config_.boss; }
    float get_boss_spawn_distance() const { return config_.boss.spawn_scroll_distance; }

    // === Static Utilities ===

    /**
     * @brief Load level index configuration (map ID to file)
     * @param filepath Path to index JSON file
     * @return true if successful
     */
    bool load_level_index(const std::string& filepath);

    /**
     * @brief Get level file path based on level ID
     * @param level_id Level number
     * @return Path to level JSON file
     */
    std::string get_level_file(uint8_t level_id);

private:
    LevelConfig config_;
    std::unordered_map<uint8_t, std::string> level_files_;

    // === JSON Parsing Helpers ===

    bool parse_level_metadata(const nlohmann::json& j);
    // parse_checkpoints removed
    bool parse_phases(const nlohmann::json& j);
    bool parse_boss_config(const nlohmann::json& j);

    // parse_checkpoint removed
    PhaseConfig parse_phase(const nlohmann::json& j);
    Wave parse_wave(const nlohmann::json& j);
    SpawnConfig parse_spawn(const nlohmann::json& j);
    game::BossPhaseConfig parse_boss_phase(const nlohmann::json& j);
    game::BossAttackConfig parse_boss_attack(const nlohmann::json& j);
};

} // namespace rtype::server
