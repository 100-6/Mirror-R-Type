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
constexpr float STARTING_MASS = 10.0f;
constexpr float MIN_MASS = 10.0f;
constexpr uint8_t MAX_CELLS_PER_PLAYER = 16;

// =============================================================================
// Cell Physics (Agar.io formula adapted for our coordinate system)
// =============================================================================
constexpr float SPEED_BASE = 2.2f;              // Agar.io base constant
constexpr float SPEED_MULTIPLIER = 150.0f;      // Scaled up for our map (5000x5000)
constexpr float SPEED_EXPONENT = -0.45f;        // Radius exponent (negative = smaller is faster)
constexpr float PLAYER_SPEED = 1.0f;            // Server-wide speed multiplier
constexpr float MIN_SPEED = 100.0f;             // Minimum speed for huge cells
constexpr float MASS_DECAY_RATE = 0.002f;       // 0.2% per second like real Agar.io
constexpr float MASS_DECAY_THRESHOLD = 100.0f;

// =============================================================================
// Splitting
// =============================================================================
constexpr float MIN_SPLIT_MASS = 35.0f;
constexpr float SPLIT_SPEED_BOOST = 600.0f;     // Initial split velocity
constexpr float SPLIT_DECAY_RATE = 600.0f;      // Fast decay (~1s to stop)
constexpr float SPLIT_LOSS_FACTOR = 0.5f;
constexpr float MERGE_TIME_BASE = 30.0f;        // 30 seconds base merge time (like Agar.io)
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
constexpr int INITIAL_FOOD = 100;          
constexpr int FOOD_SPAWN_BATCH = 20;       
constexpr int FOOD_SPAWN_RATE = 10;        
constexpr float FOOD_SPAWN_INTERVAL = 0.5f;
constexpr float FOOD_SPAWN_RADIUS = 10.0f;

// =============================================================================
// Virus Configuration
// =============================================================================
constexpr float VIRUS_MASS = 100.0f;
constexpr int MAX_VIRUSES = 30;
constexpr int INITIAL_VIRUSES = 10;             // Viruses to spawn at start
constexpr float VIRUS_SPLIT_MASS = 130.0f;      // Cell must be >= this mass to be split by virus
constexpr int VIRUS_SPLIT_COUNT = 8;            // Number of pieces when split by virus
constexpr float VIRUS_FEED_MASS = 7.0f;         // Mass gained per ejected mass eaten
constexpr int VIRUS_POP_THRESHOLD = 7;          // Ejected masses to absorb before popping
constexpr float VIRUS_SHOOT_SPEED = 350.0f;     // Speed of shot virus (reduced for better gameplay)
constexpr float VIRUS_SHOOT_MASS = 100.0f;      // Mass of shot virus
constexpr float VIRUS_FRICTION = 265.0f;        // Deceleration per second (lower = goes further)

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
 * @brief Calculate cell speed from mass (exact Agar.io/MultiOgarII formula)
 * Formula: speed = 2.2 * radius^(-0.45) * 40 * playerSpeed
 * Smaller cells are MUCH faster (like a "pile électrique")
 * Examples with mass 10 (radius ~18): speed ~310
 *          with mass 100 (radius ~56): speed ~140
 *          with mass 1000 (radius ~178): speed ~65
 */
inline float mass_to_speed(float mass) {
    float radius = mass_to_radius(mass);
    float speed = SPEED_BASE * std::pow(radius, SPEED_EXPONENT) * SPEED_MULTIPLIER * PLAYER_SPEED;
    return std::max(speed, MIN_SPEED);
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
