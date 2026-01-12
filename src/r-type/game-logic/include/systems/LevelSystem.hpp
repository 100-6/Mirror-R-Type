/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LevelSystem - Orchestrates level progression through state machine
*/

#ifndef LEVEL_SYSTEM_HPP_
#define LEVEL_SYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/LevelComponents.hpp"
#include "components/GameComponents.hpp"

namespace game {

/**
 * @brief System that manages level progression through a state machine
 *
 * The LevelSystem orchestrates the entire level flow:
 * - LEVEL_START → WAVES → BOSS_TRANSITION → BOSS_FIGHT → LEVEL_COMPLETE
 * - Detects phase completion (all waves spawned + all enemies dead)
 * - Triggers boss spawn on BOSS_FIGHT entry
 * - Loads next level on LEVEL_COMPLETE
 * - Handles final victory (level 3 complete)
 *
 * This system works on server-side only (authoritative).
 */
class LevelSystem : public ISystem {
public:
    LevelSystem();
    virtual ~LevelSystem() = default;

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
     * @brief Update level state machine
     * @param registry ECS registry
     * @param dt Delta time in seconds
     */
    void update(Registry& registry, float dt) override;

    // === State Transition Callbacks ===

    /**
     * @brief Called when level transitions to WAVES state
     */
    void on_waves_started(Registry& registry, LevelController& lc);

    /**
     * @brief Called when transitioning to BOSS_TRANSITION
     */
    void on_boss_transition_started(Registry& registry, LevelController& lc);

    /**
     * @brief Called when boss fight starts
     */
    void on_boss_fight_started(Registry& registry, LevelController& lc);

    /**
     * @brief Called when level is completed
     */
    void on_level_completed(Registry& registry, LevelController& lc);

    // === Query Methods ===

    /**
     * @brief Check if all phases in current level are complete
     */
    bool all_phases_complete(Registry& registry, const LevelController& lc);

    /**
     * @brief Check if there are no enemies remaining
     */
    bool no_enemies_remaining(Registry& registry);

    /**
     * @brief Check if boss is defeated
     */
    bool boss_defeated(Registry& registry, const LevelController& lc);

private:
    // === State Transition Helpers ===

    void transition_to_level_start(LevelController& lc);
    void transition_to_waves(LevelController& lc);
    void transition_to_boss_transition(LevelController& lc);
    void transition_to_boss_fight(LevelController& lc);
    void transition_to_level_complete(LevelController& lc);
    void transition_to_game_over(LevelController& lc);

    // === Level Loading ===

    /**
     * @brief Load next level or trigger final victory
     */
    void load_next_level_or_final_victory(Registry& registry, LevelController& lc);

    /**
     * @brief Clear all enemies and projectiles from registry
     */
    void clear_all_enemies_and_projectiles(Registry& registry);
};

} // namespace game

#endif // LEVEL_SYSTEM_HPP_
