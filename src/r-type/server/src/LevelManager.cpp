/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LevelManager implementation
*/

#include "LevelManager.hpp"
#include <fstream>
#include <iostream>

namespace rtype::server {

LevelManager::LevelManager()
{
}

bool LevelManager::load_from_file(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[LevelManager] Failed to open level config: " << filepath << "\n";
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (!parse_level_metadata(j)) return false;
    if (!parse_phases(j)) return false;
        if (!parse_boss_config(j)) return false;

        // std::cout << "[LevelManager] Loaded level " << static_cast<int>(config_.level_id)
        //           << ": " << config_.level_name << "\n";
        // std::cout << "[LevelManager]   - " << config_.phases.size() << " phases\n";
        // std::cout << "[LevelManager]   - " << config_.checkpoints.size() << " checkpoints\n";
        // std::cout << "[LevelManager]   - Boss: " << config_.boss.boss_name << "\n";

        return true;
    } catch (const std::exception& e) {
        // std::cerr << "[LevelManager] JSON parse error: " << e.what() << "\n";
        return false;
    }
}

bool LevelManager::load_level(uint8_t level_id)
{
    // Clear existing config basics
    config_ = LevelConfig();
    
    // Allow debug levels (0, 99) in addition to regular levels (1-3)
    std::string filepath = get_level_file(level_id);
    return load_from_file(filepath);
}

bool LevelManager::load_level_index(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[LevelManager] Failed to open level index: " << filepath << "\n";
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (j.contains("maps") && j["maps"].is_array()) {
            for (const auto& map : j["maps"]) {
                std::string id_str = map.value("id", "");
                std::string config_path = map.value("wavesConfig", "");
                
                // Extract level number from id string "level_X_..."
                int level_id = 0;
                if (sscanf(id_str.c_str(), "level_%d_", &level_id) == 1) {
                    level_files_[static_cast<uint8_t>(level_id)] = config_path;
                    std::cout << "[LevelManager] Registered level " << level_id << ": " << config_path << "\n";
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[LevelManager] Index parse error: " << e.what() << "\n";
        return false;
    }
}

std::string LevelManager::get_level_file(uint8_t level_id)
{
    // Check map first
    if (level_files_.find(level_id) != level_files_.end()) {
        return level_files_[level_id];
    }

    // Fallback logic
    switch (level_id) {
        case 0:
            return "assets/levels/level_0_test.json";
        case 1:
            return "assets/levels/level_1_mars_assault.json";
        case 2:
            return "assets/levels/level_2_nebula_station.json";
        case 3:
            return "assets/levels/level_3_uranus_station.json";
        case 4:
            return "assets/levels/level_4_jupiter_orbit.json";
        case 99:
            return "assets/levels/level_99_instant_boss.json";
        default:
            return "assets/levels/level_1_mars_assault.json";
    }
}

// ============================================================================
// JSON PARSING HELPERS
// ============================================================================

bool LevelManager::parse_level_metadata(const nlohmann::json& j)
{
    config_.level_id = j.value("level_id", 1);
    config_.level_name = j.value("level_name", "Unnamed Level");
    config_.level_description = j.value("level_description", "");
    config_.map_id = j.value("map_id", 1);
    config_.base_scroll_speed = j.value("base_scroll_speed", 60.0f);
    config_.total_scroll_distance = j.value("total_scroll_distance", 8000.0f);
    config_.total_chunks = j.value("total_chunks", 20);

    return true;
}

bool LevelManager::parse_phases(const nlohmann::json& j)
{
    if (!j.contains("phases") || !j["phases"].is_array()) {
        std::cerr << "[LevelManager] No phases array found\n";
        return false;
    }

    for (const auto& phase_json : j["phases"]) {
        PhaseConfig phase = parse_phase(phase_json);
        config_.phases.push_back(phase);
    }

    return true;
}

bool LevelManager::parse_boss_config(const nlohmann::json& j)
{
    if (!j.contains("boss")) {
        std::cerr << "[LevelManager] No boss config found\n";
        return false;
    }

    const auto& boss_json = j["boss"];
    config_.boss.boss_name = boss_json.value("boss_name", "Boss");
    config_.boss.spawn_scroll_distance = boss_json.value("spawn_scroll_distance", 7500.0f);
    config_.boss.spawn_position_x = boss_json.value("spawn_position_x", 1600.0f);
    config_.boss.spawn_position_y = boss_json.value("spawn_position_y", 540.0f);
    config_.boss.enemy_type = boss_json.value("enemy_type", "boss");
    config_.boss.total_phases = boss_json.value("total_phases", 3);
    config_.boss.script_path = boss_json.value("script_path", "boss/boss1_mars_guardian.lua");

    if (boss_json.contains("phases") && boss_json["phases"].is_array()) {
        for (const auto& phase_json : boss_json["phases"]) {
            game::BossPhaseConfig phase = parse_boss_phase(phase_json);
            config_.boss.phases.push_back(phase);
        }
        // std::cout << "[LevelManager] Parsed " << config_.boss.phases.size() << " boss phases\n";
    } else {
        // std::cerr << "[LevelManager] WARNING: No boss phases found in JSON!\n";
    }

    return true;
}

// parse_checkpoint removed

PhaseConfig LevelManager::parse_phase(const nlohmann::json& j)
{
    PhaseConfig phase;
    phase.phase_number = j.value("phase_number", 1);
    phase.phase_name = j.value("phase_name", "Phase 1");
    phase.scroll_start = j.value("scroll_start", 0.0f);
    phase.scroll_end = j.value("scroll_end", 1000.0f);
    phase.difficulty = j.value("difficulty", "easy");

    if (j.contains("waves") && j["waves"].is_array()) {
        for (const auto& wave_json : j["waves"]) {
            Wave wave = parse_wave(wave_json);
            phase.waves.push_back(wave);
        }
    }

    return phase;
}

Wave LevelManager::parse_wave(const nlohmann::json& j)
{
    Wave wave;
    wave.wave_number = j.value("wave_number", 0);
    wave.completed = false;
    wave.triggered = false;

    if (j.contains("trigger")) {
        const auto& trigger = j["trigger"];
        wave.trigger.scroll_distance = trigger.value("scrollDistance", 0.0f);
        wave.trigger.chunk_id = trigger.value("chunkId", 0);
        wave.trigger.offset = trigger.value("offset", 0.0f);
        wave.trigger.time_delay = trigger.value("timeDelay", 0.0f);
    }

    if (j.contains("spawns") && j["spawns"].is_array()) {
        for (const auto& spawn_json : j["spawns"]) {
            SpawnConfig spawn = parse_spawn(spawn_json);
            wave.spawns.push_back(spawn);
        }
    }

    return wave;
}

SpawnConfig LevelManager::parse_spawn(const nlohmann::json& j)
{
    SpawnConfig spawn;
    spawn.type = j.value("type", "enemy");
    spawn.enemy_type = j.value("enemyType", "basic");
    spawn.bonus_type = j.value("bonusType", "health");
    spawn.position_x = j.value("positionX", 1920.0f);
    spawn.position_y = j.value("positionY", 300.0f);
    spawn.count = j.value("count", 1);
    spawn.pattern = j.value("pattern", "single");
    spawn.spacing = j.value("spacing", 0.0f);

    // Parse bonusDrop for enemies
    if (j.contains("bonusDrop")) {
        const auto& drop_json = j["bonusDrop"];
        spawn.bonus_drop.enabled = true;
        spawn.bonus_drop.bonus_type = drop_json.value("bonusType", "health");
        spawn.bonus_drop.drop_chance = drop_json.value("dropChance", 1.0f);
    }

    return spawn;
}

game::BossPhaseConfig LevelManager::parse_boss_phase(const nlohmann::json& j)
{
    game::BossPhaseConfig phase;
    phase.phase_number = j.value("phase_number", 1);
    phase.health_threshold = j.value("health_threshold", 1.0f);

    // Parse movement pattern
    std::string movement_str = j.value("movement_pattern", "horizontal_sine");
    if (movement_str == "horizontal_sine") {
        phase.movement_pattern = game::BossMovementPattern::HORIZONTAL_SINE;
    } else if (movement_str == "vertical_sine") {
        phase.movement_pattern = game::BossMovementPattern::VERTICAL_SINE;
    } else if (movement_str == "figure_eight") {
        phase.movement_pattern = game::BossMovementPattern::FIGURE_EIGHT;
    } else if (movement_str == "circle") {
        phase.movement_pattern = game::BossMovementPattern::CIRCLE;
    } else if (movement_str == "aggressive_chase") {
        phase.movement_pattern = game::BossMovementPattern::AGGRESSIVE_CHASE;
    } else {
        phase.movement_pattern = game::BossMovementPattern::HORIZONTAL_SINE;
    }

    phase.movement_speed_multiplier = j.value("movement_speed_multiplier", 1.0f);

    if (j.contains("attack_patterns") && j["attack_patterns"].is_array()) {
        for (const auto& attack_json : j["attack_patterns"]) {
            game::BossAttackConfig attack = parse_boss_attack(attack_json);
            phase.attack_patterns.push_back(attack);
        }
    }

    return phase;
}

game::BossAttackConfig LevelManager::parse_boss_attack(const nlohmann::json& j)
{
    game::BossAttackConfig attack;

    // Parse attack pattern
    std::string pattern_str = j.value("pattern", "spray_360");
    if (pattern_str == "spray_360") {
        attack.pattern = game::BossAttackPattern::SPRAY_360;
    } else if (pattern_str == "aimed_burst") {
        attack.pattern = game::BossAttackPattern::AIMED_BURST;
    } else if (pattern_str == "laser_sweep") {
        attack.pattern = game::BossAttackPattern::LASER_SWEEP;
    } else if (pattern_str == "spiral") {
        attack.pattern = game::BossAttackPattern::SPIRAL;
    } else if (pattern_str == "aimed_triple") {
        attack.pattern = game::BossAttackPattern::AIMED_TRIPLE;
    } else if (pattern_str == "random_barrage") {
        attack.pattern = game::BossAttackPattern::RANDOM_BARRAGE;
    } else {
        attack.pattern = game::BossAttackPattern::SPRAY_360;
    }

    attack.cooldown = j.value("cooldown", 2.0f);
    attack.projectile_count = j.value("projectile_count", 12);
    attack.projectile_speed = j.value("projectile_speed", 200.0f);
    attack.projectile_damage = j.value("projectile_damage", 15);
    attack.spread_angle = j.value("spread_angle", 15.0f);
    attack.rotation_speed = j.value("rotation_speed", 45.0f);

    return attack;
}

} // namespace rtype::server
