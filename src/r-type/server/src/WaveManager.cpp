#include "WaveManager.hpp"
#include <fstream>
#include <iostream>
#include <cmath>

namespace rtype::server {

WaveManager::WaveManager()
    : current_wave_index_(0)
    , accumulated_time_(0.0f) {
}

bool WaveManager::load_from_file(const std::string& filepath) {
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

void WaveManager::update(float delta_time, float current_scroll) {
    accumulated_time_ += delta_time;
    check_wave_triggers(current_scroll);
}

void WaveManager::reset() {
    current_wave_index_ = 0;
    accumulated_time_ = 0.0f;

    for (auto& wave : config_.waves) {
        wave.completed = false;
        wave.triggered = false;
    }
}

bool WaveManager::all_waves_complete() const {
    for (const auto& wave : config_.waves) {
        if (!wave.completed)
            return false;
    }
    return !config_.waves.empty();
}

std::string WaveManager::get_map_file(protocol::GameMode mode, protocol::Difficulty difficulty) {
    std::string mode_str;
    switch (mode) {
        case protocol::GameMode::DUO: mode_str = "duo"; break;
        case protocol::GameMode::TRIO: mode_str = "trio"; break;
        case protocol::GameMode::SQUAD: mode_str = "squad"; break;
        default: mode_str = "squad"; break;
    }
    std::string diff_str;
    switch (difficulty) {
        case protocol::Difficulty::EASY: diff_str = "easy"; break;
        case protocol::Difficulty::NORMAL: diff_str = "normal"; break;
        case protocol::Difficulty::HARD: diff_str = "hard"; break;
        default: diff_str = "normal"; break;
    }
    // For now, use the simple wave file as fallback
    // In production, you would have files like: waves_duo_easy.json, waves_squad_hard.json, etc.
    return "src/r-type/assets/waves_simple.json";
}

void WaveManager::check_wave_triggers(float current_scroll) {
    for (size_t i = current_wave_index_; i < config_.waves.size(); ++i) {
        Wave& wave = config_.waves[i];

        if (wave.triggered || wave.completed)
            continue;
        bool scroll_triggered = (current_scroll >= wave.trigger.scroll_distance);
        bool time_triggered = (accumulated_time_ >= wave.trigger.time_delay);
        if (scroll_triggered && time_triggered) {
            trigger_wave(wave);
            wave.triggered = true;
            current_wave_index_ = i;
        }
    }
}

void WaveManager::trigger_wave(Wave& wave) {
    std::cout << "[WaveManager] Triggering wave " << wave.wave_number << "\n";

    if (wave_start_callback_)
        wave_start_callback_(wave.wave_number, "Wave " + std::to_string(wave.wave_number));
    for (const auto& spawn : wave.spawns)
        spawn_from_config(spawn);
    // Mark wave as completed immediately for now
    // In a real implementation, we need to track enemy kills
    wave.completed = true;
    if (wave_complete_callback_)
        wave_complete_callback_(wave.wave_number);
}

void WaveManager::spawn_from_config(const SpawnConfig& spawn) {
    if (!spawn_enemy_callback_)
        return;
    if (spawn.pattern == "single")
        spawn_enemy_callback_(spawn.enemy_type, spawn.position_x, spawn.position_y);
    else if (spawn.pattern == "line") {
        for (uint32_t i = 0; i < spawn.count; ++i) {
            float y = spawn.position_y + (i * spawn.spacing);
            spawn_enemy_callback_(spawn.enemy_type, spawn.position_x, y);
        }
    }
    else if (spawn.pattern == "formation") {
        uint32_t cols = static_cast<uint32_t>(std::sqrt(spawn.count));
        uint32_t rows = (spawn.count + cols - 1) / cols;
        for (uint32_t r = 0; r < rows; ++r) {
            for (uint32_t c = 0; c < cols; ++c) {
                if (r * cols + c >= spawn.count) break;

                float x = spawn.position_x + (c * spawn.spacing);
                float y = spawn.position_y + (r * spawn.spacing);
                spawn_enemy_callback_(spawn.enemy_type, x, y);
            }
        }
    }
}

}
