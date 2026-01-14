#pragma once

#include <cstdint>
#include <cmath>

namespace bagario::config {

// =============================================================================
// Network Configuration
// =============================================================================
constexpr uint8_t PROTOCOL_VERSION = 0x01;
constexpr uint16_t DEFAULT_TCP_PORT = 5002;
constexpr uint16_t DEFAULT_UDP_PORT = 5003;
constexpr uint32_t TICK_RATE = 60;
constexpr uint32_t TICK_INTERVAL_MS = 16;
constexpr uint32_t SNAPSHOT_RATE = 20;
constexpr uint32_t SNAPSHOT_INTERVAL_MS = 50;

// =============================================================================
// Map Configuration
// =============================================================================
constexpr float MAP_WIDTH = 5000.0f;
constexpr float MAP_HEIGHT = 5000.0f;

// =============================================================================
// Player Configuration
// =============================================================================
constexpr uint8_t MAX_PLAYERS = 50;
constexpr float STARTING_MASS = 500.0f;
constexpr float MIN_MASS = 10.0f;
constexpr uint8_t MAX_CELLS_PER_PLAYER = 16;

// =============================================================================
// Cell Physics
// =============================================================================
constexpr float BASE_SPEED = 200.0f;
constexpr float MASS_SPEED_FACTOR = 1.0f;
constexpr float MASS_DECAY_RATE = 0.001f;
constexpr float MASS_DECAY_THRESHOLD = 100.0f;

// =============================================================================
// Splitting
// =============================================================================
constexpr float MIN_SPLIT_MASS = 35.0f;
constexpr float SPLIT_SPEED_BOOST = 500.0f;
constexpr float SPLIT_LOSS_FACTOR = 0.5f;
constexpr float MERGE_TIME_BASE = 30.0f;
constexpr float MERGE_TIME_PER_MASS = 0.02f;

// =============================================================================
// Eating Mechanics
// =============================================================================
constexpr float EAT_OVERLAP_RATIO = 0.4f;
constexpr float EAT_MASS_RATIO = 1.25f;

// =============================================================================
// Food Configuration
// =============================================================================
constexpr float FOOD_MASS = 1.0f;
constexpr int MAX_FOOD = 1000;
constexpr int FOOD_SPAWN_RATE = 10;
constexpr float FOOD_SPAWN_RADIUS = 10.0f;

// =============================================================================
// Virus Configuration
// =============================================================================
constexpr float VIRUS_MASS = 100.0f;
constexpr int MAX_VIRUSES = 30;
constexpr float VIRUS_SPLIT_MASS = 130.0f;
constexpr int VIRUS_SPLIT_COUNT = 8;

// =============================================================================
// Ejected Mass Configuration
// =============================================================================
constexpr float EJECT_MASS_COST = 16.0f;
constexpr float EJECT_MASS_VALUE = 12.0f;
constexpr float EJECT_SPEED = 600.0f;
constexpr float EJECT_DECAY_TIME = 20.0f;
constexpr float MIN_EJECT_MASS = 32.0f;

// =============================================================================
// Leaderboard
// =============================================================================
constexpr int LEADERBOARD_SIZE = 10;
constexpr float LEADERBOARD_UPDATE_RATE = 2.0f;

// =============================================================================
// Utility Functions
// =============================================================================

/**
 * @brief Calculate cell radius from mass
 * Using formula: radius = 10 * sqrt(mass / PI)
 */
inline float mass_to_radius(float mass) {
    constexpr float PI = 3.14159265358979323846f;
    return 10.0f * std::sqrt(mass / PI);
}

/**
 * @brief Calculate mass from cell radius
 */
inline float radius_to_mass(float radius) {
    constexpr float PI = 3.14159265358979323846f;
    return PI * (radius / 10.0f) * (radius / 10.0f);
}

/**
 * @brief Calculate cell speed from mass
 * Bigger cells move slower
 */
inline float mass_to_speed(float mass) {
    return BASE_SPEED / std::sqrt(mass * MASS_SPEED_FACTOR);
}

/**
 * @brief Check if cell A can eat cell B
 */
inline bool can_eat(float mass_a, float mass_b) {
    return mass_a > mass_b * EAT_MASS_RATIO;
}

/**
 * @brief Calculate merge time for a cell based on mass
 */
inline float get_merge_time(float mass) {
    return MERGE_TIME_BASE + (mass * MERGE_TIME_PER_MASS);
}

}
