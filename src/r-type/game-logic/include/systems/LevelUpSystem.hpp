/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LevelUpSystem - Handles per-player level progression
*/

#ifndef LEVELUPSYSTEM_HPP_
#define LEVELUPSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "Entity.hpp"
#include "core/event/EventBus.hpp"
#include <functional>
#include <cstdint>

namespace rtype::game {

/**
 * @brief Callback type for level-up events (for network broadcasting)
 *
 * @param entity Server entity that leveled up
 * @param new_level The new level (1-5)
 * @param new_skin_id The computed skin_id (color * 5 + ship_type)
 */
using LevelUpCallback = std::function<void(
    engine::Entity entity,
    uint8_t new_level,
    uint8_t new_skin_id
)>;

/**
 * @brief System that handles per-player level progression based on score
 *
 * This system monitors player scores and triggers level-up effects when
 * score thresholds are reached. Level-ups change:
 * - Ship type (visual and hitbox)
 * - Weapon type
 *
 * The system is designed to run on the server side and notifies the network
 * layer via callbacks when level-ups occur.
 */
class LevelUpSystem : public ISystem {
public:
    LevelUpSystem() = default;
    ~LevelUpSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

    /**
     * @brief Set callback for level-up events
     *
     * This callback is invoked when a player levels up, allowing
     * the network system to broadcast the change to clients.
     */
    void set_level_up_callback(LevelUpCallback callback) { level_up_callback_ = callback; }

private:
    core::EventBus::SubscriptionId enemy_killed_sub_id_ = 0;
    LevelUpCallback level_up_callback_;

    /**
     * @brief Check and apply level-up for all players
     *
     * Called after score updates to check if any player has reached
     * a new level threshold.
     */
    void check_all_players_level_up(Registry& registry);

    /**
     * @brief Apply level-up effects to a player entity
     *
     * Changes weapon, updates hitbox, and sets visual feedback flags.
     *
     * @param registry The ECS registry
     * @param player_entity The player entity to level up
     * @param new_level The new level to apply
     */
    void apply_level_up(Registry& registry, engine::Entity player_entity, uint8_t new_level);
};

} // namespace rtype::game

#endif /* !LEVELUPSYSTEM_HPP_ */
