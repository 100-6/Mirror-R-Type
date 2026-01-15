/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ShipComponents - Ship type definitions and hitbox utilities
*/

#pragma once

#include <cstdint>
#include "GameConfig.hpp"

namespace rtype::game {

/**
 * @brief Ship type enumeration for different ship classes
 * These correspond to the different ship classes available for selection
 */
enum class ShipType : uint8_t {
    SCOUT = 0,      // Éclaireur - Fast, agile scout ship
    FIGHTER = 1,    // Chasseur - Balanced fighter
    CRUISER = 2,    // Croiseur - Heavy cruiser
    BOMBER = 3,     // Bombardier - Medium bomber
    CARRIER = 4     // Porte-avions - Large carrier
};

/**
 * @brief Hitbox size categories for different ship types
 */
enum class HitboxSize {
    SMALL,      // 80x80 - SCOUT
    MEDIUM,     // 104x104 - FIGHTER, BOMBER
    LARGE       // 128x128 - CRUISER, CARRIER
};

/**
 * @brief Structure to hold hitbox dimensions
 */
struct HitboxDimensions {
    float width;
    float height;
};

/**
 * @brief Extract ship type from skin_id
 *
 * The skin_id is encoded as: color * 5 + type (range 0-14)
 * where color is 0-2 (GREEN, RED, BLUE) and type is 0-4 (SCOUT to CARRIER)
 *
 * @param skin_id The skin identifier (0-14)
 * @return ShipType The extracted ship type
 */
inline ShipType get_ship_type_from_skin_id(uint8_t skin_id) {
    return static_cast<ShipType>(skin_id % 5);
}

/**
 * @brief Get the hitbox size category for a given ship type
 *
 * Ship type to hitbox size mapping:
 * - SCOUT → SMALL (80x80)
 * - FIGHTER → MEDIUM (104x104)
 * - BOMBER → MEDIUM (104x104)
 * - CRUISER → LARGE (128x128)
 * - CARRIER → LARGE (128x128)
 *
 * @param type The ship type
 * @return HitboxSize The corresponding hitbox size category
 */
inline HitboxSize get_hitbox_size_for_ship_type(ShipType type) {
    switch (type) {
        case ShipType::SCOUT:
            return HitboxSize::SMALL;
        case ShipType::FIGHTER:
        case ShipType::BOMBER:
            return HitboxSize::MEDIUM;
        case ShipType::CRUISER:
        case ShipType::CARRIER:
            return HitboxSize::LARGE;
        default:
            return HitboxSize::MEDIUM;  // Safe default
    }
}

/**
 * @brief Get the actual hitbox dimensions for a given ship type
 *
 * This function translates a ship type into concrete width/height values
 * using the constants defined in GameConfig.hpp
 *
 * @param type The ship type
 * @return HitboxDimensions The width and height of the hitbox
 */
inline HitboxDimensions get_hitbox_dimensions_for_ship_type(ShipType type) {
    using namespace rtype::shared;

    HitboxSize size = get_hitbox_size_for_ship_type(type);

    switch (size) {
        case HitboxSize::SMALL:
            return {config::PLAYER_HITBOX_SMALL_WIDTH,
                    config::PLAYER_HITBOX_SMALL_HEIGHT};
        case HitboxSize::MEDIUM:
            return {config::PLAYER_HITBOX_MEDIUM_WIDTH,
                    config::PLAYER_HITBOX_MEDIUM_HEIGHT};
        case HitboxSize::LARGE:
            return {config::PLAYER_HITBOX_LARGE_WIDTH,
                    config::PLAYER_HITBOX_LARGE_HEIGHT};
        default:
            return {config::PLAYER_HITBOX_MEDIUM_WIDTH,
                    config::PLAYER_HITBOX_MEDIUM_HEIGHT};  // Safe default
    }
}

/**
 * @brief Get hitbox dimensions directly from a skin_id
 *
 * This is a convenience function that combines ship type extraction
 * and hitbox dimension lookup in a single call.
 *
 * @param skin_id The skin identifier (0-14)
 * @return HitboxDimensions The width and height of the hitbox for this skin
 */
inline HitboxDimensions get_hitbox_dimensions_from_skin_id(uint8_t skin_id) {
    ShipType type = get_ship_type_from_skin_id(skin_id);
    return get_hitbox_dimensions_for_ship_type(type);
}

} // namespace rtype::game
