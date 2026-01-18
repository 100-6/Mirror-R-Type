/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LevelUpSystem implementation
*/

#include "systems/LevelUpSystem.hpp"
#include "components/GameComponents.hpp"
#include "components/PlayerLevelComponent.hpp"
#include "components/ShipComponents.hpp"
#include "components/CombatHelpers.hpp"
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/events/InputEvents.hpp"
#include <iostream>

namespace rtype::game {

void LevelUpSystem::init(Registry& registry)
{
    std::cout << "LevelUpSystem: Initialized" << std::endl;

    auto& eventBus = registry.get_event_bus();

    // Subscribe to EnemyKilledEvent to check for level-ups after score updates
    enemy_killed_sub_id_ = eventBus.subscribe<ecs::EnemyKilledEvent>(
        [this, &registry](const ecs::EnemyKilledEvent& /* event */) {
            // Check all players for potential level-ups
            // This runs after ScoreSystem has updated the scores
            check_all_players_level_up(registry);
        }
    );
}

void LevelUpSystem::update(Registry& registry, float dt)
{
    // Update level-up visual effect timers
    auto& player_levels = registry.get_components<PlayerLevel>();

    for (size_t i = 0; i < player_levels.size(); ++i) {
        PlayerLevel& pl = player_levels.get_data_at(i);
        if (pl.level_up_pending) {
            pl.level_up_timer -= dt;
            if (pl.level_up_timer <= 0.0f) {
                pl.level_up_pending = false;
                pl.level_up_timer = 0.0f;
            }
        }
    }
}

void LevelUpSystem::shutdown()
{
    std::cout << "LevelUpSystem: Shutdown" << std::endl;
}

void LevelUpSystem::check_all_players_level_up(Registry& registry)
{
    auto& player_levels = registry.get_components<PlayerLevel>();
    auto& scores = registry.get_components<Score>();
    auto& controllables = registry.get_components<Controllable>();

    for (size_t i = 0; i < player_levels.size(); ++i) {
        Entity entity = player_levels.get_entity_at(i);

        // Verify this is a player entity (has Controllable and Score)
        if (!scores.has_entity(entity) || !controllables.has_entity(entity))
            continue;

        PlayerLevel& pl = player_levels[entity];
        const Score& score = scores[entity];

        // Calculate what level the player should be at
        uint8_t new_level = get_level_for_score(static_cast<uint32_t>(score.value));

        // Check if level has increased
        if (new_level > pl.current_level) {
            std::cout << "LevelUpSystem: Player entity " << entity
                      << " leveling up from " << static_cast<int>(pl.current_level)
                      << " to " << static_cast<int>(new_level)
                      << " (score: " << score.value << ")" << std::endl;

            apply_level_up(registry, entity, new_level);
        }
    }
}

void LevelUpSystem::apply_level_up(Registry& registry, Entity player_entity, uint8_t new_level)
{
    auto& player_levels = registry.get_components<PlayerLevel>();
    auto& weapons = registry.get_components<Weapon>();
    auto& colliders = registry.get_components<Collider>();

    if (!player_levels.has_entity(player_entity))
        return;

    PlayerLevel& pl = player_levels[player_entity];
    uint8_t old_level = pl.current_level;

    // Update level
    pl.current_level = new_level;
    pl.level_up_pending = true;
    pl.level_up_timer = 2.0f;  // 2 second visual effect

    // Get new ship type and weapon type
    WeaponType new_weapon_type = get_weapon_type_for_level(new_level);
    uint8_t new_skin_id = compute_skin_id(new_level, pl.color_id);

    // Update weapon
    if (weapons.has_entity(player_entity)) {
        weapons[player_entity] = create_weapon(new_weapon_type, engine::INVALID_HANDLE);
    }

    // Update hitbox based on new ship type
    if (colliders.has_entity(player_entity)) {
        HitboxDimensions hitbox = get_hitbox_dimensions_from_skin_id(new_skin_id);
        colliders[player_entity].width = hitbox.width;
        colliders[player_entity].height = hitbox.height;
    }

    std::cout << "LevelUpSystem: Applied level up - Level " << static_cast<int>(old_level)
              << " -> " << static_cast<int>(new_level)
              << ", Ship type: " << static_cast<int>(get_ship_type_for_level(new_level))
              << ", Weapon: " << static_cast<int>(new_weapon_type)
              << ", Skin ID: " << static_cast<int>(new_skin_id) << std::endl;

    // Notify network layer via callback
    if (level_up_callback_) {
        level_up_callback_(player_entity, new_level, new_skin_id);
    }
}

} // namespace rtype::game
