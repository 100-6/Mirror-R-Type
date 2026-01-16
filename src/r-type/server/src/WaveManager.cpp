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
    , wave_generation_(0)
{
}

void WaveManager::load_from_phases(const std::vector<Wave>& all_waves)
{
    config_.waves = all_waves;
    config_.default_spawn_interval = 2.0f;
    config_.loop_waves = false;

    current_wave_index_ = 0;
    accumulated_time_ = 0.0f;
}

void WaveManager::update(float delta_time, float current_scroll)
{
    accumulated_time_ += delta_time;
    check_wave_triggers(current_scroll);
    check_wave_completion(delta_time);
}

void WaveManager::reset()
{
    current_wave_index_ = 0;
    accumulated_time_ = 0.0f;
    wave_generation_++;  // Invalidate all pending completions

    for (auto& wave : config_.waves) {
        wave.completed = false;
        wave.triggered = false;
        wave.time_since_triggered = 0.0f;
        wave.triggered_generation = 0;  // Reset generation marker
    }
}

void WaveManager::check_wave_completion(float delta_time)
{
    const float WAVE_COMPLETION_TIMEOUT = 30.0f;

    for (auto& wave : config_.waves) {
        if (wave.triggered && !wave.completed) {
            // CRITICAL: Only process waves from current generation
            if (wave.triggered_generation != wave_generation_) {
                std::cout << "[WaveManager] ⚠️ Skipping stale wave " << wave.wave_number
                          << " (gen " << wave.triggered_generation
                          << " vs current " << wave_generation_ << ")\n";
                continue;
            }

            wave.time_since_triggered += delta_time;

            if (wave.time_since_triggered >= WAVE_COMPLETION_TIMEOUT) {
                wave.completed = true;

                if (listener_)
                    listener_->on_wave_completed(wave);
            }
        }
    }
}

void WaveManager::reset_to_wave(uint32_t wave_number)
{
    // Find wave index for the given wave number
    size_t target_index = 0;
    bool found = false;

    for (size_t i = 0; i < config_.waves.size(); ++i) {
        if (config_.waves[i].wave_number == wave_number) {
            target_index = i;
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "[WaveManager] Warning: wave " << wave_number
                  << " not found, resetting to wave 0\n";
        reset();
        return;
    }

    current_wave_index_ = target_index;
    wave_generation_++;  // Invalidate old wave completions

    // Set accumulated time to target wave's delay so time-based triggers work immediately
    if (target_index < config_.waves.size()) {
        accumulated_time_ = config_.waves[target_index].trigger.time_delay;
    } else {
        accumulated_time_ = 0.0f;
    }

    // Mark waves BEFORE target as completed
    for (size_t i = 0; i < target_index; ++i) {
        config_.waves[i].completed = true;
        config_.waves[i].triggered = true;
        config_.waves[i].triggered_generation = wave_generation_;  // Mark with current generation
    }

    // Reset waves AT or AFTER target
    for (size_t i = target_index; i < config_.waves.size(); ++i) {
        config_.waves[i].completed = false;
        config_.waves[i].triggered = false;
        config_.waves[i].time_since_triggered = 0.0f;
        config_.waves[i].triggered_generation = 0;  // Will be set when triggered
    }

    // std::cout << "[WaveManager] Reset to wave " << wave_number
    //           << " (index " << target_index << "), generation=" << wave_generation_ << "\n";

    // Manually spawn the target wave immediately
    spawn_wave(wave_number);
}

bool WaveManager::spawn_wave(uint32_t wave_number)
{
    for (size_t i = 0; i < config_.waves.size(); ++i) {
        if (config_.waves[i].wave_number == wave_number) {
            Wave& wave = config_.waves[i];
            
            // Mark as triggered and set generation
            wave.triggered = true;
            wave.triggered_generation = wave_generation_;
            wave.completed = false; // Ensure it's active
            wave.time_since_triggered = 0.0f;
            
            // Update index tracking
            current_wave_index_ = i;
            
            std::cout << "[WaveManager] 🎮 MANUAL SPAWN: Wave " << wave_number 
                      << " (gen=" << wave_generation_ << ")\n";
                      
            trigger_wave(wave);
            return true;
        }
    }
    
    std::cerr << "[WaveManager] Failed to manual spawn wave " << wave_number << " (not found)\n";
    return false;
}

float WaveManager::get_wave_start_scroll(uint32_t wave_number) const
{
    for (const auto& wave : config_.waves) {
        if (wave.wave_number == wave_number) {
            return wave.trigger.scroll_distance;
        }
    }
    return 0.0f;
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
            // std::cout << "[WaveManager] Unknown map_id " << map_id << ", using Nebula Outpost\n";
            return "assets/waves_nebula_outpost.json";
    }
}

void WaveManager::check_wave_triggers(float current_scroll)
{
    // Calculate current chunk and offset (1 chunk = 480px = 30 tiles * 16px)
    float chunkSizePx = 480.0f;
    int currentChunk = static_cast<int>(current_scroll / chunkSizePx);
    float currentOffset = (current_scroll - (currentChunk * chunkSizePx)) / chunkSizePx;

    for (size_t i = current_wave_index_; i < config_.waves.size(); ++i) {
        Wave& wave = config_.waves[i];

        if (wave.triggered || wave.completed)
            continue;

        bool should_trigger = false;

        // Check chunk trigger
        if (currentChunk > wave.trigger.chunk_id) {
            should_trigger = true;
        } else if (currentChunk == wave.trigger.chunk_id) {
            if (currentOffset >= wave.trigger.offset) {
                should_trigger = true;
            }
        }
        
        // Legacy scroll trigger backup (only if chunkId == 0)
        if (wave.trigger.chunk_id == 0 && wave.trigger.scroll_distance > 0.1f) {
            if (current_scroll >= wave.trigger.scroll_distance) {
                should_trigger = true;
            }
        }
        
        bool time_triggered = (accumulated_time_ >= wave.trigger.time_delay);

        if (should_trigger && time_triggered) {
            trigger_wave(wave);
            wave.triggered = true;
            current_wave_index_ = i;
            break;  // Only trigger one wave per update to prevent cascading triggers
        }
    }
}

void WaveManager::trigger_wave(Wave& wave)
{
    std::cout << "[WaveManager] 🌊 Wave " << wave.wave_number
              << " START (gen=" << wave_generation_ << ")\n";

    wave.triggered_generation = wave_generation_;

    // CRITICAL FIX: Mark all previous waves as completed to prevent out-of-order completions
    // When wave 5 starts, waves 1-4 should not auto-complete later and overwrite the current wave
    for (auto& w : config_.waves) {
        if (w.wave_number < wave.wave_number && w.triggered && !w.completed) {
            w.completed = true;
            std::cout << "[WaveManager] ⏭️ Auto-completing old wave " << w.wave_number
                      << " (current wave " << wave.wave_number << " starting)\n";
        }
    }

    if (listener_)
        listener_->on_wave_started(wave);

    for (const auto& spawn : wave.spawns)
        spawn_from_config(spawn);

    wave.time_since_triggered = 0.0f;
}

void WaveManager::spawn_from_config(const SpawnConfig& spawn)
{
    if (!listener_) {
        // std::cout << "[WaveManager] ERROR: No listener set!\n";
        return;
    }

    if (spawn.type == "enemy") {

        if (spawn.pattern == "single") {
            listener_->on_spawn_enemy(spawn.enemy_type, spawn.position_x, spawn.position_y, spawn.bonus_drop);
        } else if (spawn.pattern == "line") {
            for (uint32_t i = 0; i < spawn.count; ++i) {
                float y = spawn.position_y + (i * spawn.spacing);
                listener_->on_spawn_enemy(spawn.enemy_type, spawn.position_x, y, spawn.bonus_drop);
            }
        } else if (spawn.pattern == "formation") {
            uint32_t cols = static_cast<uint32_t>(std::sqrt(spawn.count));
            uint32_t rows = (spawn.count + cols - 1) / cols;
            for (uint32_t r = 0; r < rows; ++r) {
                for (uint32_t c = 0; c < cols; ++c) {
                    if (r * cols + c >= spawn.count) break;
                    float x = spawn.position_x + (c * spawn.spacing);
                    float y = spawn.position_y + (r * spawn.spacing);
                    listener_->on_spawn_enemy(spawn.enemy_type, x, y, spawn.bonus_drop);
                }
            }
        }
    }
    else if (spawn.type == "wall") {

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

        if (spawn.pattern == "single") {
            listener_->on_spawn_powerup(spawn.bonus_type, spawn.position_x, spawn.position_y);
        } else if (spawn.pattern == "line") {
            for (uint32_t i = 0; i < spawn.count; ++i) {
                float y = spawn.position_y + (i * spawn.spacing);
                listener_->on_spawn_powerup(spawn.bonus_type, spawn.position_x, y);
            }
        }
    }
}

}
