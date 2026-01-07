/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** WaveManager - Manages wave spawning from JSON configuration
*/

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "protocol/PacketTypes.hpp"
#include "interfaces/IWaveListener.hpp"

namespace rtype::server {

struct SpawnConfig {
    std::string type;           // "enemy", "wall", or "powerup"
    std::string enemy_type;     // "basic", "fast", "tank", "boss" (for enemies)
    std::string bonus_type;     // "health", "shield", "speed" (for powerups)
    float position_x;
    float position_y;
    uint32_t count;
    std::string pattern;        // "single", "line", "formation"
    float spacing;
};

struct WaveTrigger {
    float scroll_distance;
    float time_delay;
};

struct Wave {
    uint32_t wave_number;
    WaveTrigger trigger;
    std::vector<SpawnConfig> spawns;
    bool completed;
    bool triggered;

    Wave() : wave_number(0), completed(false), triggered(false) {}
};

struct WaveConfig {
    float default_spawn_interval;
    bool loop_waves;
    std::vector<Wave> waves;
};

/**
 * @brief Manages wave spawning from JSON configuration
 *
 * Simple class that:
 * - Loads wave config from JSON
 * - Checks triggers based on scroll distance or time
 * - Notifies listener when waves start/complete
 * - Notifies listener when entities should spawn
 */
class WaveManager {
public:
    WaveManager();
    ~WaveManager() = default;

    // === Configuration ===

    /**
     * @brief Set the listener for wave events
     */
    void set_listener(IWaveListener* listener) { listener_ = listener; }

    /**
     * @brief Load wave configuration from JSON file
     */
    bool load_from_file(const std::string& filepath);

    // === Update ===

    /**
     * @brief Update wave system (check triggers, spawn enemies)
     * Call this every server tick
     */
    void update(float delta_time, float current_scroll);

    /**
     * @brief Reset to initial state
     */
    void reset();

    // === Queries ===

    uint32_t get_total_waves() const { return config_.waves.size(); }
    uint32_t get_current_wave() const { return current_wave_index_ + 1; }
    bool all_waves_complete() const;

    /**
     * @brief Select map file based on map_id
     * @param map_id 1=Nebula Outpost, 2=Asteroid Belt, 3=Bydo Mothership
     */
    static std::string get_map_file(uint16_t map_id);

private:
    WaveConfig config_;
    uint32_t current_wave_index_;
    float accumulated_time_;
    IWaveListener* listener_ = nullptr;

    void check_wave_triggers(float current_scroll);
    void trigger_wave(Wave& wave);
    void spawn_from_config(const SpawnConfig& spawn);
};

}
