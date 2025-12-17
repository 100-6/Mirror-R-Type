/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** WaveConfig - Configuration for wave-based enemy spawning system
*/

#ifndef WAVE_CONFIG_HPP_
#define WAVE_CONFIG_HPP_

// ============= WAVE SYSTEM CONFIGURATION =============

// Wave spawning parameters
#define WAVE_DEFAULT_SPAWN_INTERVAL     2.0f    // Seconds between individual spawns
#define WAVE_DEFAULT_BETWEEN_WAVES      5.0f    // Seconds between waves
#define WAVE_MIN_SPAWN_INTERVAL         0.5f    // Minimum spawn interval
#define WAVE_MAX_SPAWN_INTERVAL         10.0f   // Maximum spawn interval

// Spawn positions (relative to screen)
#define WAVE_SPAWN_OFFSET_X             50.0f   // Pixels beyond right edge
#define WAVE_SPAWN_MIN_Y                50.0f   // Minimum Y position
#define WAVE_SPAWN_MAX_Y                1030.0f // Maximum Y position (1080 - 50)
#define WAVE_SPAWN_WALL_HEIGHT          100.0f  // Default wall height
#define WAVE_SPAWN_WALL_WIDTH           50.0f   // Default wall width

// Scrolling-based spawning
#define WAVE_SCROLL_TRIGGER_DISTANCE    100.0f  // Pixels of scrolling to trigger spawn
#define WAVE_SPAWN_AHEAD_DISTANCE       1920.0f // Spawn distance ahead (screen width)

// Entity type identifiers (for JSON)
#define WAVE_ENTITY_TYPE_ENEMY          "enemy"
#define WAVE_ENTITY_TYPE_WALL           "wall"
#define WAVE_ENTITY_TYPE_OBSTACLE       "obstacle"
#define WAVE_ENTITY_TYPE_POWERUP        "powerup"

// Default values for spawned entities
#define WAVE_DEFAULT_ENEMY_TYPE         0       // EnemyType::BASIC
#define WAVE_DEFAULT_WALL_HEALTH        100
#define WAVE_DEFAULT_OBSTACLE_HEALTH    50

// Wave system limits
#define WAVE_MAX_ACTIVE_WAVES           10      // Maximum concurrent waves
#define WAVE_MAX_ENTITIES_PER_WAVE      50      // Maximum entities per wave
#define WAVE_JSON_MAX_SIZE_MB           10      // Maximum JSON file size (MB)

// Spawn patterns
#define WAVE_PATTERN_SINGLE             "single"
#define WAVE_PATTERN_LINE               "line"
#define WAVE_PATTERN_GRID               "grid"
#define WAVE_PATTERN_RANDOM             "random"
#define WAVE_PATTERN_FORMATION          "formation"

// Formation spacing
#define WAVE_FORMATION_SPACING_X        100.0f  // Horizontal spacing
#define WAVE_FORMATION_SPACING_Y        80.0f   // Vertical spacing

#endif /* !WAVE_CONFIG_HPP_ */
