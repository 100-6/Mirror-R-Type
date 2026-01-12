/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** WaveManager implementation
*/

#include "WaveManager.hpp"
#include <fstream>
#include <iostream>
#include <cmath>

namespace rtype::server {

WaveManager::WaveManager()
    : current_wave_index_(0)
    , accumulated_time_(0.0f)
{
}

bool WaveManager::load_from_file(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[WaveManager] Failed to open wave config: " << filepath << "\n";
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;

        config_.default_spawn_interval = j.value("defaultSpawnInterval", 2.0f);
        config_.loop_waves = j.value("loopWaves", false);

        if (j.contains("waves") && j["waves"].is_array()) {
            for (const auto& wave_json : j["waves"]) {
                Wave wave;
                wave.wave_number = wave_json.value("waveNumber", 0);

                if (wave_json.contains("trigger")) {
                    const auto& trigger = wave_json["trigger"];
                    wave.trigger.scroll_distance = trigger.value("scrollDistance", 0.0f);
                    wave.trigger.time_delay = trigger.value("timeDelay", 0.0f);
                }

                if (wave_json.contains("spawns") && wave_json["spawns"].is_array()) {
                    for (const auto& spawn_json : wave_json["spawns"]) {
                        SpawnConfig spawn;
                        spawn.type = spawn_json.value("type", "enemy");
                        spawn.enemy_type = spawn_json.value("enemyType", "basic");
                        spawn.bonus_type = spawn_json.value("bonusType", "health");
                        spawn.position_x = spawn_json.value("positionX", 1920.0f);
                        spawn.position_y = spawn_json.value("positionY", 300.0f);
                        spawn.count = spawn_json.value("count", 1);
                        spawn.pattern = spawn_json.value("pattern", "single");
                        spawn.spacing = spawn_json.value("spacing", 0.0f);
                        wave.spawns.push_back(spawn);
                    }
                }
                config_.waves.push_back(wave);
            }
        }

        std::cout << "[WaveManager] Loaded " << config_.waves.size() << " waves from " << filepath << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[WaveManager] JSON parse error: " << e.what() << "\n";
        return false;
    }
}

void WaveManager::load_from_phases(const std::vector<Wave>& all_waves)
{
    config_.waves = all_waves;
    config_.default_spawn_interval = 2.0f;
    config_.loop_waves = false;

    current_wave_index_ = 0;
    accumulated_time_ = 0.0f;

    std::cout << "[WaveManager] Loaded " << all_waves.size() << " waves from level config\n";
}

void WaveManager::update(float delta_time, float current_scroll)
{
    accumulated_time_ += delta_time;
    check_wave_triggers(current_scroll);
}

void WaveManager::reset()
{
    current_wave_index_ = 0;
    accumulated_time_ = 0.0f;

    for (auto& wave : config_.waves) {
        wave.completed = false;
        wave.triggered = false;
    }
}

bool WaveManager::all_waves_complete() const
{
    // If there are no waves at all, consider them complete (for instant boss levels)
    if (config_.waves.empty()) {
        return true;
    }

    // Check if all waves are completed
    for (const auto& wave : config_.waves) {
        if (!wave.completed)
            return false;
    }
    return true;
}

std::string WaveManager::get_map_file(uint16_t map_id)
{
    // Map ID to file mapping:
    // 1 = Nebula Outpost
    switch (map_id) {
        case 1:
            return "assets/waves_nebula_outpost.json";

        default:
            std::cout << "[WaveManager] Unknown map_id " << map_id << ", using Nebula Outpost\n";
            return "assets/waves_nebula_outpost.json";
    }
}

void WaveManager::check_wave_triggers(float current_scroll)
{
    static bool first_check = true;
    if (first_check) {
        std::cout << "[WaveManager] First check: " << config_.waves.size() << " waves loaded\n";
        first_check = false;
    }

    for (size_t i = current_wave_index_; i < config_.waves.size(); ++i) {
        Wave& wave = config_.waves[i];

        if (wave.triggered || wave.completed)
            continue;

        bool scroll_triggered = (current_scroll >= wave.trigger.scroll_distance);
        bool time_triggered = (accumulated_time_ >= wave.trigger.time_delay);

        static int debug_counter = 0;
        if (++debug_counter % 300 == 0 && i == current_wave_index_) {
            std::cout << "[WaveManager] Wave " << wave.wave_number
                      << " scroll=" << current_scroll << "/" << wave.trigger.scroll_distance
                      << " time=" << accumulated_time_ << "/" << wave.trigger.time_delay << "\n";
        }

        if (scroll_triggered && time_triggered) {
            trigger_wave(wave);
            wave.triggered = true;
            current_wave_index_ = i;
        }
    }
}

void WaveManager::trigger_wave(Wave& wave)
{
    std::cout << "[WaveManager] Triggering wave " << wave.wave_number << "\n";

    if (listener_)
        listener_->on_wave_started(wave);

    for (const auto& spawn : wave.spawns)
        spawn_from_config(spawn);

    // Mark wave as completed immediately for now
    wave.completed = true;

    if (listener_)
        listener_->on_wave_completed(wave);
}

void WaveManager::spawn_from_config(const SpawnConfig& spawn)
{
    if (!listener_) {
        std::cout << "[WaveManager] ERROR: No listener set!\n";
        return;
    }

    if (spawn.type == "enemy") {
        std::cout << "[WaveManager] Spawning enemy: type=" << spawn.enemy_type
                  << " pattern=" << spawn.pattern << " count=" << spawn.count << "\n";

        if (spawn.pattern == "single") {
            listener_->on_spawn_enemy(spawn.enemy_type, spawn.position_x, spawn.position_y);
        } else if (spawn.pattern == "line") {
            for (uint32_t i = 0; i < spawn.count; ++i) {
                float y = spawn.position_y + (i * spawn.spacing);
                listener_->on_spawn_enemy(spawn.enemy_type, spawn.position_x, y);
            }
        } else if (spawn.pattern == "formation") {
            uint32_t cols = static_cast<uint32_t>(std::sqrt(spawn.count));
            uint32_t rows = (spawn.count + cols - 1) / cols;
            for (uint32_t r = 0; r < rows; ++r) {
                for (uint32_t c = 0; c < cols; ++c) {
                    if (r * cols + c >= spawn.count) break;
                    float x = spawn.position_x + (c * spawn.spacing);
                    float y = spawn.position_y + (r * spawn.spacing);
                    listener_->on_spawn_enemy(spawn.enemy_type, x, y);
                }
            }
        }
    }
    else if (spawn.type == "wall") {
        std::cout << "[WaveManager] Spawning wall: pattern=" << spawn.pattern << " count=" << spawn.count << "\n";

        if (spawn.pattern == "single") {
            listener_->on_spawn_wall(spawn.position_x, spawn.position_y);
        } else if (spawn.pattern == "line") {
            for (uint32_t i = 0; i < spawn.count; ++i) {
                float y = spawn.position_y + (i * spawn.spacing);
                listener_->on_spawn_wall(spawn.position_x, y);
            }
        }
    }
    else if (spawn.type == "powerup") {
        std::cout << "[WaveManager] Spawning powerup: type=" << spawn.bonus_type
                  << " pattern=" << spawn.pattern << " count=" << spawn.count << "\n";

        if (spawn.pattern == "single") {
            listener_->on_spawn_powerup(spawn.bonus_type, spawn.position_x, spawn.position_y);
        } else if (spawn.pattern == "line") {
            for (uint32_t i = 0; i < spawn.count; ++i) {
                float y = spawn.position_y + (i * spawn.spacing);
                listener_->on_spawn_powerup(spawn.bonus_type, spawn.position_x, y);
            }
        }
    }
    else {
        std::cout << "[WaveManager] WARNING: Unknown spawn type: " << spawn.type << "\n";
    }
}

}
