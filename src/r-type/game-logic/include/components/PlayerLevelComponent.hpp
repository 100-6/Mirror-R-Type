/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** PlayerLevelComponent - Per-player level progression system
*/

#pragma once

#include <cstdint>
#include "GameComponents.hpp"
#include "ShipComponents.hpp"

namespace rtype::game {

/**
 * @brief Level thresholds - score required to reach each level
 *
 * Level 1: 0 points     - SCOUT + BASIC
 * Level 2: 2000 points  - FIGHTER + SPREAD
 * Level 3: 5000 points  - CRUISER + BURST
 * Level 4: 10000 points - BOMBER + LASER
 * Level 5: 20000 points - CARRIER + CHARGE
 */
constexpr uint32_t LEVEL_THRESHOLDS[] = {
    0,        // Level 1 - SCOUT (starting level)
    2000,     // Level 2 - FIGHTER
    5000,     // Level 3 - CRUISER
    10000,    // Level 4 - BOMBER
    20000     // Level 5 - CARRIER (max level)
};
constexpr size_t MAX_LEVEL = 5;

/**
 * @brief Maps level (1-5) to ship type (0-4)
 *
 * Level 1 = SCOUT (0)
 * Level 2 = FIGHTER (1)
 * Level 3 = CRUISER (2)
 * Level 4 = BOMBER (3)
 * Level 5 = CARRIER (4)
 *
 * @param level The player level (1-5)
 * @return uint8_t The ship type (0-4)
 */
inline uint8_t get_ship_type_for_level(uint8_t level) {
    if (level == 0 || level > MAX_LEVEL)
        return 0;  // Default to SCOUT
    return level - 1;
}

/**
 * @brief Maps level (1-5) to weapon type
 *
 * Level 1 = BASIC
 * Level 2 = SPREAD
 * Level 3 = BURST
 * Level 4 = LASER
 * Level 5 = CHARGE
 *
 * @param level The player level (1-5)
 * @return WeaponType The corresponding weapon type
 */
inline WeaponType get_weapon_type_for_level(uint8_t level) {
    switch (level) {
        case 1: return WeaponType::BASIC;
        case 2: return WeaponType::SPREAD;
        case 3: return WeaponType::BURST;
        case 4: return WeaponType::LASER;
        case 5: return WeaponType::CHARGE;
        default: return WeaponType::BASIC;
    }
}

/**
 * @brief Get level for a given score
 *
 * Iterates through thresholds to find the highest level the score qualifies for.
 *
 * @param score The player's total score
 * @return uint8_t The corresponding level (1-5)
 */
inline uint8_t get_level_for_score(uint32_t score) {
    for (size_t i = MAX_LEVEL; i > 0; --i) {
        if (score >= LEVEL_THRESHOLDS[i - 1]) {
            return static_cast<uint8_t>(i);
        }
    }
    return 1;
}

/**
 * @brief Get score threshold for next level
 *
 * @param current_level The current level (1-5)
 * @return uint32_t The score needed for next level, or UINT32_MAX if max level
 */
inline uint32_t get_next_level_threshold(uint8_t current_level) {
    if (current_level >= MAX_LEVEL)
        return UINT32_MAX;
    return LEVEL_THRESHOLDS[current_level];
}

/**
 * @brief Compute skin_id from level and color
 *
 * The skin_id encodes both ship type (from level) and color:
 * skin_id = color * 5 + ship_type
 *
 * @param level The player level (1-5)
 * @param color_id The color selection (0=GREEN, 1=RED, 2=BLUE)
 * @return uint8_t The computed skin_id (0-14)
 */
inline uint8_t compute_skin_id(uint8_t level, uint8_t color_id) {
    uint8_t ship_type = get_ship_type_for_level(level);
    return color_id * 5 + ship_type;
}

/**
 * @brief Extract color_id from skin_id
 *
 * @param skin_id The skin identifier (0-14)
 * @return uint8_t The color (0=GREEN, 1=RED, 2=BLUE)
 */
inline uint8_t get_color_from_skin_id(uint8_t skin_id) {
    return skin_id / 5;
}

/**
 * @brief Component to track per-player level progression
 *
 * This component is attached to player entities and tracks their
 * individual progression through the level system.
 */
struct PlayerLevel {
    uint8_t current_level = 1;           // Current level (1-5)
    uint8_t color_id = 0;                // Player color selection (0=GREEN, 1=RED, 2=BLUE)
    bool level_up_pending = false;       // Flag for visual feedback
    float level_up_timer = 0.0f;         // Timer for level-up effect (seconds)

    /**
     * @brief Get the ship type for the current level
     */
    ShipType get_ship_type() const {
        return static_cast<ShipType>(get_ship_type_for_level(current_level));
    }

    /**
     * @brief Get the weapon type for the current level
     */
    WeaponType get_weapon_type() const {
        return get_weapon_type_for_level(current_level);
    }

    /**
     * @brief Get the computed skin_id for network sync
     */
    uint8_t get_skin_id() const {
        return compute_skin_id(current_level, color_id);
    }
};

} // namespace rtype::game
