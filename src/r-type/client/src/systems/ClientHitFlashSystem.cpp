/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ClientHitFlashSystem - Client-side hit flash effect
*/

#include "systems/ClientHitFlashSystem.hpp"
#include "components/GameComponents.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/events/GameEvents.hpp"
#include "ecs/events/InputEvents.hpp"
#include <iostream>

namespace rtype::client {

void ClientHitFlashSystem::init(Registry& registry)
{
    std::cout << "ClientHitFlashSystem: Initialisation" << std::endl;
    previous_hp_.clear();
}

void ClientHitFlashSystem::shutdown()
{
    std::cout << "ClientHitFlashSystem: Shutdown" << std::endl;
    previous_hp_.clear();
}

void ClientHitFlashSystem::update(Registry& registry, float dt)
{
    auto& healths = registry.get_components<Health>();
    auto& sprites = registry.get_components<Sprite>();
    auto& overlays = registry.get_components<FlashOverlay>();
    auto& localPlayers = registry.get_components<LocalPlayer>();

    // Check for HP decreases and create new flash overlays
    for (size_t i = 0; i < healths.size(); ++i) {
        Entity entity = healths.get_entity_at(i);

        if (!healths.has_entity(entity) || !sprites.has_entity(entity))
            continue;

        const Health& health = healths[entity];
        int current_hp = health.current;

        // Check if we have previous HP recorded
        auto it = previous_hp_.find(entity);
        if (it != previous_hp_.end()) {
            int prev_hp = it->second;

            // HP decreased?
            if (current_hp < prev_hp) {
                bool is_local_player = localPlayers.has_entity(entity);

                // Apply flash effect only if still alive
                if (current_hp > 0) {
                    constexpr float FLASH_DURATION = 0.25f;

                    if (overlays.has_entity(entity)) {
                        // Reset existing overlay
                        overlays[entity].time_remaining = FLASH_DURATION;
                        overlays[entity].total_duration = FLASH_DURATION;
                    } else {
                        // Create new flash overlay - RenderSystem will draw white rectangle
                        registry.add_component(entity, FlashOverlay{
                            FLASH_DURATION,
                            FLASH_DURATION,
                            200.0f  // Max alpha
                        });
                    }

                    // Emit hit events to trigger sounds
                    if (is_local_player) {
                        registry.get_event_bus().publish(ecs::PlayerHitEvent{entity, Entity{0}});
                    } else {
                        registry.get_event_bus().publish(ecs::EnemyHitEvent{entity, Entity{0}});
                    }
                }

                // Emit EnemyKilledEvent when non-player entity dies (HP drops to 0 or below)
                if (current_hp <= 0 && !is_local_player) {
                    registry.get_event_bus().publish(ecs::EnemyKilledEvent{entity, 100});
                }
            }
        }

        // Update tracked HP
        previous_hp_[entity] = current_hp;
    }

    // Update existing overlays (just update timer, RenderSystem handles the visual effect)
    for (size_t i = 0; i < overlays.size(); ++i) {
        Entity entity = overlays.get_entity_at(i);

        if (!overlays.has_entity(entity))
            continue;

        FlashOverlay& overlay = overlays[entity];
        overlay.time_remaining -= dt;

        if (overlay.time_remaining <= 0.0f) {
            // Flash complete - remove component
            registry.remove_component<FlashOverlay>(entity);
        }
    }

    // Clean up tracked HP for destroyed entities
    std::vector<Entity> to_remove;
    for (const auto& [entity, hp] : previous_hp_) {
        if (!healths.has_entity(entity)) {
            to_remove.push_back(entity);
        }
    }
    for (Entity entity : to_remove) {
        previous_hp_.erase(entity);
    }
}

} // namespace rtype::client
