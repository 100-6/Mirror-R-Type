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

struct BonusDropConfig {
    bool enabled = false;
    std::string bonus_type = "health";  // "health", "shield", "speed", "bonus_weapon"
    float drop_chance = 1.0f;
};

struct SpawnConfig {
    std::string type;           // "enemy", "wall", or "powerup"
    std::string enemy_type;     // "basic", "fast", "tank", "boss" (for enemies)
    std::string bonus_type;     // "health", "shield", "speed" (for powerups)
    float position_x;
    float position_y;
    uint32_t count;
    std::string pattern;        // "single", "line", "formation"
    float spacing;
    BonusDropConfig bonus_drop; // Optional bonus drop on death (for enemies)
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
    float time_since_triggered;
    uint32_t triggered_generation;  // Generation when wave was triggered

    Wave() : wave_number(0), completed(false), triggered(false),
             time_since_triggered(0.0f), triggered_generation(0) {}
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
     * @brief Load waves from level phases (for level system)
     * @param phases Vector of phases containing waves
     */
    void load_from_phases(const std::vector<Wave>& all_waves);

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

    /**
     * @brief Reset to a specific wave number
     * @param wave_number Target wave number to reset to
     */
    void reset_to_wave(uint32_t wave_number);

    /**
     * @brief Immediately spawn a specific wave, bypassing triggers
     * @param wave_number Wave number to spawn
     * @return true if wave found and spawned
     */
    bool spawn_wave(uint32_t wave_number);

    /**
     * @brief Get the scroll distance where a wave starts
     * @param wave_number Wave number to look up
     * @return float Scroll distance, or 0.0f if not found
     */
    float get_wave_start_scroll(uint32_t wave_number) const;

    // === Queries ===

    uint32_t get_total_waves() const { return config_.waves.size(); }
    uint32_t get_current_wave() const { return current_wave_index_ + 1; }
    bool all_waves_complete() const;

    /**
     * @brief Get all waves (for broadcasting after reset)
     * @return const reference to waves vector
     */
    const std::vector<Wave>& get_waves() const { return config_.waves; }

    /**
     * @brief Select map file based on map_id
     * @param map_id 1=Nebula Outpost, 2=Asteroid Belt, 3=Bydo Mothership
     */
    static std::string get_map_file(uint16_t map_id);

private:
    WaveConfig config_;
    uint32_t current_wave_index_;
    float accumulated_time_;
    uint32_t wave_generation_;  // Increments on each reset
    IWaveListener* listener_ = nullptr;

    void check_wave_triggers(float current_scroll);
    void check_wave_completion(float delta_time);
    void trigger_wave(Wave& wave);
    void spawn_from_config(const SpawnConfig& spawn);
};

}
