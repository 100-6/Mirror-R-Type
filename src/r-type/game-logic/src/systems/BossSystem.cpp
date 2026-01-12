/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** BossSystem implementation
*/

#include "systems/BossSystem.hpp"
#include "ecs/CoreComponents.hpp"
#include <iostream>
#include <cmath>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace game {

BossSystem::BossSystem()
{
}

void BossSystem::init(Registry& registry)
{
    // Nothing to initialize
}

void BossSystem::shutdown()
{
    // Nothing to cleanup
}

void BossSystem::update(Registry& registry, float dt)
{
    auto& boss_phases = registry.get_components<BossPhase>();
    auto& healths = registry.get_components<Health>();
    auto& enemies = registry.get_components<Enemy>();

    for (size_t i = 0; i < boss_phases.size(); ++i) {
        Entity boss_entity = boss_phases.get_entity_at(i);
        BossPhase& phase = boss_phases.get_data_at(i);

        // Verify this is actually a boss
        if (!enemies.has_entity(boss_entity)) continue;
        const Enemy& enemy = enemies[boss_entity];
        if (!enemy.is_boss) continue;

        // Check if boss has health component
        if (!healths.has_entity(boss_entity)) continue;
        const Health& health = healths[boss_entity];

        // Calculate health percentage
        float health_pct = static_cast<float>(health.current) / static_cast<float>(health.max);

        // Check for phase transition
        uint8_t new_phase = calculate_phase_from_health(health_pct, phase);
        if (new_phase != phase.current_phase) {
            transition_boss_phase(registry, boss_entity, phase, new_phase);
        }

        // Update phase timer
        phase.phase_timer += dt;
        phase.attack_cooldown -= dt;

        // Execute attack pattern if cooldown expired
        if (phase.attack_cooldown <= 0.0f) {
            execute_boss_attack(registry, boss_entity, phase);
        }

        // Update movement pattern
        update_boss_movement(registry, boss_entity, phase, dt);
    }
}

// ============================================================================
// PHASE MANAGEMENT
// ============================================================================

uint8_t BossSystem::calculate_phase_from_health(float health_pct, const BossPhase& phase)
{
    if (health_pct > 0.66f) {
        return 0;  // Phase 1
    } else if (health_pct > 0.33f) {
        return 1;  // Phase 2
    } else {
        return 2;  // Phase 3
    }
}

void BossSystem::transition_boss_phase(Registry& registry, Entity boss_entity,
                                        BossPhase& phase, uint8_t new_phase)
{
    std::cout << "[BossSystem] Boss transitioning to phase " << static_cast<int>(new_phase + 1) << "\n";

    phase.current_phase = new_phase;
    phase.phase_timer = 0.0f;
    phase.attack_pattern_index = 0;

    // Update movement pattern and speed from phase config
    if (new_phase < phase.phase_configs.size()) {
        const BossPhaseConfig& config = phase.phase_configs[new_phase];
        phase.movement_pattern = config.movement_pattern;
        phase.movement_speed_multiplier = config.movement_speed_multiplier;
    }

    // TODO: Notify clients of phase transition (visual effect, warning message)
}

// ============================================================================
// ATTACK PATTERN EXECUTION
// ============================================================================

void BossSystem::execute_boss_attack(Registry& registry, Entity boss_entity, BossPhase& phase)
{
    auto& positions = registry.get_components<Position>();
    if (!positions.has_entity(boss_entity)) return;

    const Position& boss_pos = positions[boss_entity];

    // Get current attack config
    if (phase.current_phase >= phase.phase_configs.size()) return;
    const BossPhaseConfig& phase_config = phase.phase_configs[phase.current_phase];

    if (phase.attack_pattern_index >= phase_config.attack_patterns.size()) return;
    const BossAttackConfig& attack = phase_config.attack_patterns[phase.attack_pattern_index];

    // Execute attack based on pattern type
    switch (attack.pattern) {
        case BossAttackPattern::SPRAY_360:
            spawn_360_spray(registry, boss_pos, attack);
            break;
        case BossAttackPattern::AIMED_BURST:
            spawn_aimed_burst(registry, boss_pos, attack);
            break;
        case BossAttackPattern::SPIRAL:
            spawn_spiral(registry, boss_pos, attack, phase.phase_timer);
            break;
        case BossAttackPattern::LASER_SWEEP:
            spawn_laser_sweep(registry, boss_pos, attack, phase.phase_timer);
            break;
        case BossAttackPattern::AIMED_TRIPLE:
            spawn_aimed_triple(registry, boss_pos, attack);
            break;
        case BossAttackPattern::RANDOM_BARRAGE:
            spawn_random_barrage(registry, boss_pos, attack);
            break;
    }

    // Reset cooldown
    phase.attack_cooldown = attack.cooldown;

    // Cycle to next attack pattern
    phase.attack_pattern_index = (phase.attack_pattern_index + 1) % phase_config.attack_patterns.size();
}

void BossSystem::spawn_360_spray(Registry& registry, const Position& boss_pos,
                                  const BossAttackConfig& attack)
{
    float angle_step = (2.0f * M_PI) / attack.projectile_count;

    for (uint32_t i = 0; i < attack.projectile_count; ++i) {
        float angle = i * angle_step;
        float vx = std::cos(angle) * attack.projectile_speed;
        float vy = std::sin(angle) * attack.projectile_speed;

        spawn_boss_projectile(registry, boss_pos.x, boss_pos.y, vx, vy, attack.projectile_damage);
    }
}

void BossSystem::spawn_aimed_burst(Registry& registry, const Position& boss_pos,
                                    const BossAttackConfig& attack)
{
    Entity nearest_player = find_nearest_player(registry, boss_pos);
    if (nearest_player == engine::INVALID_HANDLE) {
        // No player found, shoot left by default
        for (uint32_t i = 0; i < attack.projectile_count; ++i) {
            spawn_boss_projectile(registry, boss_pos.x, boss_pos.y,
                                  -attack.projectile_speed, 0.0f, attack.projectile_damage);
        }
        return;
    }

    auto& positions = registry.get_components<Position>();
    const Position& player_pos = positions[nearest_player];

    // Calculate direction to player
    float dx = player_pos.x - boss_pos.x;
    float dy = player_pos.y - boss_pos.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 0.001f) dist = 1.0f;  // Avoid division by zero

    // Normalize direction
    float dir_x = dx / dist;
    float dir_y = dy / dist;

    // Spawn burst with spread
    float spread_rad = attack.spread_angle * M_PI / 180.0f;
    float base_angle = std::atan2(dir_y, dir_x);

    for (uint32_t i = 0; i < attack.projectile_count; ++i) {
        float offset = (static_cast<float>(i) - attack.projectile_count / 2.0f) * spread_rad;
        float angle = base_angle + offset;
        float vx = std::cos(angle) * attack.projectile_speed;
        float vy = std::sin(angle) * attack.projectile_speed;

        spawn_boss_projectile(registry, boss_pos.x, boss_pos.y, vx, vy, attack.projectile_damage);
    }
}

void BossSystem::spawn_spiral(Registry& registry, const Position& boss_pos,
                               const BossAttackConfig& attack, float phase_timer)
{
    float rotation_speed_rad = attack.rotation_speed * M_PI / 180.0f;
    float base_angle = phase_timer * rotation_speed_rad;

    float angle_step = (2.0f * M_PI) / attack.projectile_count;

    for (uint32_t i = 0; i < attack.projectile_count; ++i) {
        float angle = base_angle + (i * angle_step);
        float vx = std::cos(angle) * attack.projectile_speed;
        float vy = std::sin(angle) * attack.projectile_speed;

        spawn_boss_projectile(registry, boss_pos.x, boss_pos.y, vx, vy, attack.projectile_damage);
    }
}

void BossSystem::spawn_laser_sweep(Registry& registry, const Position& boss_pos,
                                    const BossAttackConfig& attack, float phase_timer)
{
    // Laser sweep: spawn rapid projectiles in a sweeping arc
    float sweep_angle = std::sin(phase_timer * 2.0f) * M_PI / 2.0f;  // -90° to +90° sweep

    for (uint32_t i = 0; i < attack.projectile_count; ++i) {
        float angle_offset = (static_cast<float>(i) / attack.projectile_count - 0.5f) * 0.1f;
        float angle = sweep_angle + angle_offset;
        float vx = std::cos(angle) * attack.projectile_speed;
        float vy = std::sin(angle) * attack.projectile_speed;

        spawn_boss_projectile(registry, boss_pos.x, boss_pos.y, vx, vy, attack.projectile_damage);
    }
}

void BossSystem::spawn_aimed_triple(Registry& registry, const Position& boss_pos,
                                     const BossAttackConfig& attack)
{
    // Same as aimed burst, but specifically for 3 projectiles
    spawn_aimed_burst(registry, boss_pos, attack);
}

void BossSystem::spawn_random_barrage(Registry& registry, const Position& boss_pos,
                                       const BossAttackConfig& attack)
{
    for (uint32_t i = 0; i < attack.projectile_count; ++i) {
        float angle = random_angle();
        float vx = std::cos(angle) * attack.projectile_speed;
        float vy = std::sin(angle) * attack.projectile_speed;

        spawn_boss_projectile(registry, boss_pos.x, boss_pos.y, vx, vy, attack.projectile_damage);
    }
}

// ============================================================================
// MOVEMENT PATTERN UPDATES
// ============================================================================

void BossSystem::update_boss_movement(Registry& registry, Entity boss_entity,
                                       BossPhase& phase, float dt)
{
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();

    if (!positions.has_entity(boss_entity) || !velocities.has_entity(boss_entity)) {
        return;
    }

    Position& pos = positions[boss_entity];
    Velocity& vel = velocities[boss_entity];

    float speed_mult = phase.movement_speed_multiplier;
    phase.movement_timer += dt;

    switch (phase.movement_pattern) {
        case BossMovementPattern::STATIONARY:
            vel.x = 0.0f;
            vel.y = 0.0f;
            break;

        case BossMovementPattern::HORIZONTAL_SINE:
            vel.x = 0.0f;
            vel.y = std::sin(phase.movement_timer * 2.0f) * 100.0f * speed_mult;
            break;

        case BossMovementPattern::VERTICAL_SINE:
            vel.x = std::sin(phase.movement_timer * 2.0f) * 50.0f * speed_mult;
            vel.y = 0.0f;
            break;

        case BossMovementPattern::FIGURE_EIGHT:
            // Lissajous curve for figure-8 pattern
            vel.x = std::sin(phase.movement_timer * 1.5f) * 80.0f * speed_mult;
            vel.y = std::sin(phase.movement_timer * 3.0f) * 120.0f * speed_mult;
            break;

        case BossMovementPattern::CIRCLE:
            // Circular motion
            vel.x = std::cos(phase.movement_timer * 2.0f) * 100.0f * speed_mult;
            vel.y = std::sin(phase.movement_timer * 2.0f) * 100.0f * speed_mult;
            break;

        case BossMovementPattern::AGGRESSIVE_CHASE:
        {
            // Move toward nearest player
            Entity nearest_player = find_nearest_player(registry, pos);
            if (nearest_player != engine::INVALID_HANDLE) {
                const Position& player_pos = positions[nearest_player];
                float dx = player_pos.x - pos.x;
                float dy = player_pos.y - pos.y;
                float dist = std::sqrt(dx * dx + dy * dy);

                if (dist > 0.001f) {
                    vel.x = (dx / dist) * 60.0f * speed_mult;
                    vel.y = (dy / dist) * 60.0f * speed_mult;
                }
            }
            break;
        }
    }

    // Clamp boss position to stay within screen bounds
    // Boss should stay between x=800 and x=1800, y=100 and y=980
    const float MIN_X = 800.0f;
    const float MAX_X = 1800.0f;
    const float MIN_Y = 100.0f;
    const float MAX_Y = 980.0f;

    if (pos.x < MIN_X) {
        pos.x = MIN_X;
        vel.x = 0.0f;  // Stop moving left
    } else if (pos.x > MAX_X) {
        pos.x = MAX_X;
        vel.x = 0.0f;  // Stop moving right
    }

    if (pos.y < MIN_Y) {
        pos.y = MIN_Y;
        vel.y = 0.0f;  // Stop moving up
    } else if (pos.y > MAX_Y) {
        pos.y = MAX_Y;
        vel.y = 0.0f;  // Stop moving down
    }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

Entity BossSystem::find_nearest_player(Registry& registry, const Position& from_pos)
{
    // Use Controllable component to find players (works on both client and server)
    auto& controllables = registry.get_components<Controllable>();
    auto& positions = registry.get_components<Position>();

    Entity nearest = engine::INVALID_HANDLE;
    float min_dist_sq = std::numeric_limits<float>::max();

    for (size_t i = 0; i < controllables.size(); ++i) {
        Entity player_entity = controllables.get_entity_at(i);
        if (!positions.has_entity(player_entity)) continue;

        const Position& player_pos = positions[player_entity];
        float dx = player_pos.x - from_pos.x;
        float dy = player_pos.y - from_pos.y;
        float dist_sq = dx * dx + dy * dy;

        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            nearest = player_entity;
        }
    }

    return nearest;
}

void BossSystem::spawn_boss_projectile(Registry& registry, float x, float y,
                                        float vx, float vy, uint32_t damage)
{
    Entity projectile = registry.spawn_entity();

    // Position
    registry.add_component(projectile, Position{x, y});

    // Velocity
    registry.add_component(projectile, Velocity{vx, vy});

    // Projectile component (enemy faction)
    Projectile proj;
    proj.faction = ProjectileFaction::Enemy;
    proj.lifetime = 10.0f;  // 10 seconds lifetime
    proj.time_alive = 0.0f;
    registry.add_component(projectile, proj);

    // Damage
    Damage dmg;
    dmg.value = damage;
    registry.add_component(projectile, dmg);

    // Collider (small projectile)
    registry.add_component(projectile, Collider{10.0f, 10.0f});

    // Sprite (TODO: load proper boss projectile sprite)
    Sprite sprite;
    sprite.width = 10.0f;
    sprite.height = 10.0f;
    sprite.layer = 3;
    registry.add_component(projectile, sprite);
}

float BossSystem::random_float(float min, float max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

float BossSystem::random_angle()
{
    return random_float(0.0f, 2.0f * M_PI);
}

} // namespace game
