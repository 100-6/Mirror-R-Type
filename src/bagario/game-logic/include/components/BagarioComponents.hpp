#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace bagario::components {

/**
 * @brief Mass component - determines cell size and speed
 *
 * Radius formula: radius = 10 * sqrt(mass / PI)
 * Speed formula: speed = BASE_SPEED / sqrt(mass)
 */
struct Mass {
    float value = 10.0f;
};

/**
 * @brief Tag component for player-controlled cells
 */
struct PlayerCell {
    uint32_t player_id = 0;
    uint32_t color = 0xFFFFFFFF;
    std::string name;
};

/**
 * @brief Tag component for food pellets
 */
struct Food {
    float nutrition = 1.0f;
    uint32_t color = 0xFFFFFFFF;  // Random color for Agar.io-style food
};

/**
 * @brief Component for viruses (green spiky cells)
 * Viruses split large cells that touch them
 * Can be fed with ejected mass to shoot new viruses
 */
struct Virus {
    int fed_count = 0;  // Number of ejected masses absorbed
    float absorption_scale = 1.0f;  // Visual scale multiplier (grows when absorbing mass)
    float absorption_timer = 0.0f;  // Timer for absorption animation
    bool is_moving = false;  // True for shot viruses that need velocity decay
};

/**
 * @brief Tag component for ejected mass
 */
struct EjectedMass {
    float decay_timer = 20.0f;  // Seconds until despawn
    uint32_t original_owner = 0;
};

/**
 * @brief Ownership component - links cells to their player
 * Used for multi-cell scenarios (after splitting)
 */
struct CellOwner {
    uint32_t owner_id = 0;
};

/**
 * @brief Movement target - where the cell is trying to go
 * Used for mouse-following movement
 */
struct MovementTarget {
    float target_x = 0.0f;
    float target_y = 0.0f;
};

/**
 * @brief Merge timer - prevents cells from merging immediately after split
 */
struct MergeTimer {
    float time_remaining = 30.0f;  // Seconds until merge allowed
    bool can_merge = false;
};

/**
 * @brief Split velocity - temporary boost after splitting
 */
struct SplitVelocity {
    float vx = 0.0f;
    float vy = 0.0f;
    float decay_rate = 5.0f;  // How fast the boost decays
};

/**
 * @brief Circle collider for Bagario (replaces rectangular Collider)
 * Uses radius instead of width/height
 */
struct CircleCollider {
    float radius = 10.0f;
};

/**
 * @brief Network ID for entity synchronization
 */
struct NetworkId {
    uint32_t id = 0;
};

/**
 * @brief Player score tracking
 */
struct Score {
    uint32_t value = 0;
    float highest_mass = 0.0f;
    uint32_t cells_eaten = 0;
    uint32_t players_eaten = 0;
};

/**
 * @brief Player info for connected players (server-side)
 */
struct PlayerInfo {
    uint32_t client_id = 0;
    uint32_t player_id = 0;
    std::string name;
    uint32_t color = 0xFFFFFFFF;
    bool is_alive = true;
    std::chrono::steady_clock::time_point last_input_time;
};

}
