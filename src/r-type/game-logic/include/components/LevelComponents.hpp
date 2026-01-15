#ifndef LEVEL_COMPONENTS_HPP
#define LEVEL_COMPONENTS_HPP

#include <cstdint>
#include <string>
#include <vector>
#include "Entity.hpp"
#include "plugin_manager/CommonTypes.hpp"

namespace game {

// ============================================================================
// LEVEL STATE ENUM
// ============================================================================

/**
 * @brief Represents the current state of a level
 */
enum class LevelState : uint8_t {
    LEVEL_START,      ///< Brief intro/title screen (2s)
    WAVES,            ///< Normal enemy waves progression
    BOSS_TRANSITION,  ///< Pause before boss with warning (3s)
    BOSS_FIGHT,       ///< Boss is active
    LEVEL_COMPLETE,   ///< Victory animation (5s)
    GAME_OVER         ///< All players dead, no lives remaining
};

// ============================================================================
// LEVEL CONTROLLER COMPONENT
// ============================================================================

/**
 * @brief Main component that orchestrates level progression
 *
 * This component drives the level state machine and tracks the current
 * level, phase, and wave progression. Only one should exist per game session.
 */
struct LevelController {
    uint8_t current_level;           ///< Current level number (1-3)
    LevelState state;                ///< Current level state
    float state_timer;               ///< Time spent in current state
    uint32_t current_phase_index;    ///< Current phase (0-2: intro, escalation, final)
    uint32_t current_wave_in_phase;  ///< Wave index within current phase
    bool boss_spawned;               ///< Has the boss been spawned?
    engine::Entity boss_entity;      ///< Handle to the boss entity
    bool all_waves_triggered;        ///< Have all waves been triggered?

    LevelController()
        : current_level(1)
        , state(LevelState::LEVEL_START)
        , state_timer(0.0f)
        , current_phase_index(0)
        , current_wave_in_phase(0)
        , boss_spawned(false)
        , boss_entity(engine::INVALID_HANDLE)
        , all_waves_triggered(false)
    {}
};

// Checkpoint components removed - replaced by dynamic respawn system

// ============================================================================
// BOSS PHASE COMPONENTS
// ============================================================================

/**
 * @brief Boss movement pattern types
 */
enum class BossMovementPattern : uint8_t {
    STATIONARY,        ///< Stays in place
    HORIZONTAL_SINE,   ///< Horizontal sine wave motion
    VERTICAL_SINE,     ///< Vertical sine wave motion
    FIGURE_EIGHT,      ///< Figure-8 (Lissajous curve) pattern
    CIRCLE,            ///< Circular motion
    AGGRESSIVE_CHASE   ///< Moves toward nearest player
};

/**
 * @brief Boss attack pattern types
 */
enum class BossAttackPattern : uint8_t {
    SPRAY_360,         ///< Bullets in all directions (360° spray)
    AIMED_BURST,       ///< Multi-shot burst aimed at player
    LASER_SWEEP,       ///< Sweeping laser (simulated with rapid projectiles)
    SPIRAL,            ///< Rotating spiral bullet pattern
    AIMED_TRIPLE,      ///< Three aimed shots with spread
    RANDOM_BARRAGE     ///< Random bullets in all directions
};

/**
 * @brief Configuration for a single boss attack pattern
 */
struct BossAttackConfig {
    BossAttackPattern pattern;       ///< Attack pattern type
    float cooldown;                  ///< Time between attacks
    uint32_t projectile_count;       ///< Number of projectiles to spawn
    float projectile_speed;          ///< Speed of projectiles
    uint32_t projectile_damage;      ///< Damage per projectile
    float spread_angle;              ///< Spread angle for burst/triple attacks (degrees)
    float rotation_speed;            ///< Rotation speed for spiral attacks (degrees/s)

    BossAttackConfig()
        : pattern(BossAttackPattern::SPRAY_360)
        , cooldown(2.0f)
        , projectile_count(12)
        , projectile_speed(200.0f)
        , projectile_damage(15)
        , spread_angle(15.0f)
        , rotation_speed(45.0f)
    {}
};

/**
 * @brief Configuration for a single boss phase
 */
struct BossPhaseConfig {
    uint8_t phase_number;                          ///< Phase number (1, 2, 3)
    float health_threshold;                        ///< Health % to enter this phase (1.0, 0.66, 0.33)
    BossMovementPattern movement_pattern;          ///< Movement pattern for this phase
    float movement_speed_multiplier;               ///< Speed multiplier (increases each phase)
    std::vector<BossAttackConfig> attack_patterns; ///< Attack patterns for this phase

    BossPhaseConfig()
        : phase_number(1)
        , health_threshold(1.0f)
        , movement_pattern(BossMovementPattern::HORIZONTAL_SINE)
        , movement_speed_multiplier(1.0f)
    {}
};

/**
 * @brief Boss multi-phase behavior component
 *
 * Attached to boss entities to enable phase-based attacks and movement.
 * The BossSystem monitors health and transitions between phases.
 */
struct BossPhase {
    uint8_t current_phase;                         ///< Current phase (0, 1, 2)
    uint8_t total_phases;                          ///< Total number of phases (usually 3)
    std::vector<float> phase_health_thresholds;    ///< Health % thresholds [1.0, 0.66, 0.33]

    // Phase timing
    float phase_timer;                             ///< Time spent in current phase
    float attack_cooldown;                         ///< Time until next attack
    uint8_t attack_pattern_index;                  ///< Which attack pattern to use next

    // Movement state
    BossMovementPattern movement_pattern;          ///< Current movement pattern
    float movement_timer;                          ///< Timer for movement animations
    float movement_speed_multiplier;               ///< Speed multiplier for current phase

    // Attack patterns loaded from JSON
    std::vector<BossPhaseConfig> phase_configs;    ///< Configuration for all phases

    BossPhase()
        : current_phase(0)
        , total_phases(3)
        , phase_timer(0.0f)
        , attack_cooldown(0.0f)
        , attack_pattern_index(0)
        , movement_pattern(BossMovementPattern::HORIZONTAL_SINE)
        , movement_timer(0.0f)
        , movement_speed_multiplier(1.0f)
    {
        phase_health_thresholds = {1.0f, 0.66f, 0.33f};
    }
};

// ============================================================================
// SCROLL STATE COMPONENT
// ============================================================================

/**
 * @brief Stores the current scroll position for the level
 *
 * This component is used by systems like CheckpointSystem to know
 * when checkpoints should be activated.
 */
struct ScrollState {
    float current_scroll;    ///< Current scroll position in pixels

    ScrollState()
        : current_scroll(0.0f)
    {}
};

// ============================================================================
// PLAYER LIVES COMPONENT
// ============================================================================

/**
 * @brief Tracks remaining lives for a player
 *
 * When a player dies, their lives are decremented. If lives > 0, they respawn
 * at the active checkpoint after a delay. If lives = 0, they are eliminated.
 *
 * On respawn, players lose all power-ups (punishing mechanic like R-Type).
 */
struct PlayerLives {
    uint32_t player_id;              ///< Player ID this component belongs to
    uint8_t lives_remaining;         ///< Remaining lives (0-3)
    bool respawn_pending;            ///< Is player waiting to respawn?
    float respawn_timer;             ///< Countdown until respawn (3 seconds)
    // checkpoint_index removed - dynamic respawn used instead

    PlayerLives()
        : player_id(0)
        , lives_remaining(3)
        , respawn_pending(false)
        , respawn_timer(0.0f)
    {}
};

} // namespace game

#endif // LEVEL_COMPONENTS_HPP
