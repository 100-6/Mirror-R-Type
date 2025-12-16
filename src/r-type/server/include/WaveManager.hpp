#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>

#include "protocol/PacketTypes.hpp"

namespace rtype::server {

struct SpawnConfig {
    std::string type;           // "enemy" or "bonus"
    std::string enemy_type;     // "basic", "fast", "tank", "boss"
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
 */
class WaveManager {
public:
    using SpawnEnemyCallback = std::function<void(const std::string& enemy_type, float x, float y)>;
    using WaveStartCallback = std::function<void(uint32_t wave_number, const std::string& wave_name)>;
    using WaveCompleteCallback = std::function<void(uint32_t wave_number)>;

    WaveManager();
    ~WaveManager() = default;

    /**
     * @brief Load wave configuration from JSON file
     */
    bool load_from_file(const std::string& filepath);

    /**
     * @brief Update wave system (check triggers, spawn enemies)
     */
    void update(float delta_time, float current_scroll);

    /**
     * @brief Reset wave manager to initial state
     */
    void reset();

    /**
     * @brief Get total number of waves
     */
    uint32_t get_total_waves() const { return config_.waves.size(); }

    /**
     * @brief Get current wave number
     */
    uint32_t get_current_wave() const { return current_wave_index_ + 1; }

    /**
     * @brief Check if all waves are complete
     */
    bool all_waves_complete() const;

    /**
     * @brief Set callback for enemy spawns
     */
    void set_spawn_enemy_callback(SpawnEnemyCallback callback) {
        spawn_enemy_callback_ = callback;
    }

    /**
     * @brief Set callback for wave start
     */
    void set_wave_start_callback(WaveStartCallback callback) {
        wave_start_callback_ = callback;
    }

    /**
     * @brief Set callback for wave complete
     */
    void set_wave_complete_callback(WaveCompleteCallback callback) {
        wave_complete_callback_ = callback;
    }

    /**
     * @brief Select map file based on game mode and difficulty
     */
    static std::string get_map_file(protocol::GameMode mode, protocol::Difficulty difficulty);

private:
    WaveConfig config_;
    uint32_t current_wave_index_;
    float accumulated_time_;

    SpawnEnemyCallback spawn_enemy_callback_;
    WaveStartCallback wave_start_callback_;
    WaveCompleteCallback wave_complete_callback_;

    void check_wave_triggers(float current_scroll);
    void trigger_wave(Wave& wave);
    void spawn_from_config(const SpawnConfig& spawn);
};

}
