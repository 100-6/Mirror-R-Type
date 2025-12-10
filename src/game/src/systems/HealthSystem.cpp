/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** HealthSystem
*/

#include "systems/HealthSystem.hpp"
#include "components/GameComponents.hpp"
#include "ecs/Registry.hpp"
#include "ecs/events/InputEvents.hpp"
#include <iostream>

void HealthSystem::init(Registry& registry)
{
    std::cout << "HealthSystem: Initialisation" << std::endl;

    auto& eventBus = registry.get_event_bus();

    damageSubId_ = eventBus.subscribe<ecs::DamageEvent>(
        [&registry](const ecs::DamageEvent& event) {
            auto& healths = registry.get_components<Health>();
            auto& controllables = registry.get_components<Controllable>();
            auto& enemies = registry.get_components<Enemy>();

            if (!healths.has_entity(event.target))
                return;

            Health& health = healths[event.target];
            int oldHp = health.current;
            health.current -= event.damageAmount;

            std::cout << "DamageEvent: Entity " << event.target
                      << " took " << event.damageAmount << " damage ("
                      << oldHp << " -> " << health.current << " HP)" << std::endl;

            if (health.current <= 0) {
                health.current = 0;

                bool isPlayer = controllables.has_entity(event.target);
                bool isEnemy = enemies.has_entity(event.target);

                registry.get_event_bus().publish(ecs::EntityDeathEvent{event.target, isPlayer});

                if (isEnemy) {
                    registry.get_event_bus().publish(ecs::EnemyKilledEvent{event.target, 100});
                }

                registry.add_component(event.target, ToDestroy{});

                if (isPlayer) {
                    std::cout << "GAME OVER! Player died!" << std::endl;
                } else if (isEnemy) {
                    std::cout << "Enemy " << event.target << " destroyed!" << std::endl;
                }
            }
        }
    );
}

void HealthSystem::shutdown()
{
    std::cout << "HealthSystem: Arrêt" << std::endl;
}

void HealthSystem::update(Registry& registry, float dt)
{
    (void)registry;
    (void)dt;
}
