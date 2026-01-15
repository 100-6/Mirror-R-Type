/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** BossSystem - Manages boss phases, attacks, and movement
*/

#ifndef BOSS_SYSTEM_HPP_
#define BOSS_SYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/LevelComponents.hpp"
#include "components/GameComponents.hpp"
#include <cstdint>

namespace game {

/**
 * @brief System that manages boss behavior (phases, attacks, movement)
 *
 * The BossSystem is responsible for:
 * - Monitoring boss health to trigger phase transitions (66%, 33%)
 * - Executing attack patterns based on BossPhase component
 * - Updating boss movement patterns (sine wave, figure-8, chase)
 * - Spawning boss projectiles with correct velocities/damage
 * - Handling phase-specific behavior (speed multipliers, pattern changes)
 *
 * This system works on server-side only (authoritative).
 */
class BossSystem : public ISystem {
public:
    BossSystem();
    virtual ~BossSystem() = default;

    /**
     * @brief Initialize the system
     * @param registry ECS registry
     */
    void init(Registry& registry) override;

    /**
     * @brief Shutdown the system
     */
    void shutdown() override;

    /**
     * @brief Update all boss entities
     * @param registry ECS registry
     * @param dt Delta time in seconds
     */
    void update(Registry& registry, float dt) override;

private:
    // === Phase Management ===

    /**
     * @brief Calculate which phase boss should be in based on health
     * @param health_pct Health percentage (0.0-1.0)
     * @param phase Boss phase component
     * @return Phase number (0, 1, or 2)
     */
    uint8_t calculate_phase_from_health(float health_pct, const BossPhase& phase);

    /**
     * @brief Transition boss to new phase
     */
    void transition_boss_phase(Registry& registry, Entity boss_entity,
                                BossPhase& phase, uint8_t new_phase);

    // === Attack Pattern Execution ===

    /**
     * @brief Execute current attack pattern
     */
    void execute_boss_attack(Registry& registry, Entity boss_entity, BossPhase& phase);

    /**
     * @brief Spawn 360° spray pattern
     */
    void spawn_360_spray(Registry& registry, const Position& boss_pos,
                         const BossAttackConfig& attack);

    /**
     * @brief Spawn aimed burst at nearest player
     */
    void spawn_aimed_burst(Registry& registry, const Position& boss_pos,
                           const BossAttackConfig& attack);

    /**
     * @brief Spawn spiral pattern
     */
    void spawn_spiral(Registry& registry, const Position& boss_pos,
                      const BossAttackConfig& attack, float phase_timer);

    /**
     * @brief Spawn laser sweep
     */
    void spawn_laser_sweep(Registry& registry, const Position& boss_pos,
                           const BossAttackConfig& attack, float phase_timer);

    /**
     * @brief Spawn aimed triple shot with spread
     */
    void spawn_aimed_triple(Registry& registry, const Position& boss_pos,
                            const BossAttackConfig& attack);

    /**
     * @brief Spawn random barrage
     */
    void spawn_random_barrage(Registry& registry, const Position& boss_pos,
                              const BossAttackConfig& attack);

    // === Movement Pattern Updates ===

    /**
     * @brief Update boss movement based on current pattern
     */
    void update_boss_movement(Registry& registry, Entity boss_entity,
                              BossPhase& phase, float dt);

    // === Helper Functions ===

    /**
     * @brief Find nearest player entity to a position
     */
    Entity find_nearest_player(Registry& registry, const Position& from_pos);

    /**
     * @brief Spawn a single boss projectile
     */
    void spawn_boss_projectile(Registry& registry, float x, float y,
                                float vx, float vy, uint32_t damage);

    /**
     * @brief Generate random float in range [min, max]
     */
    float random_float(float min, float max);

    /**
     * @brief Generate random angle in radians [0, 2π]
     */
    float random_angle();
};

} // namespace game

#endif // BOSS_SYSTEM_HPP_
