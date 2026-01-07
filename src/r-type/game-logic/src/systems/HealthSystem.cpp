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
#include "ecs/events/GameEvents.hpp"
#include <iostream>
#include <random>
#include <cmath>

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
                std::cout << "[HealthSystem] Entity " << event.target << " HP reached 0, marking for destruction" << std::endl;

                bool isPlayer = controllables.has_entity(event.target);
                bool isEnemy = enemies.has_entity(event.target);

                registry.get_event_bus().publish(ecs::EntityDeathEvent{event.target, isPlayer});

                if (isEnemy) {
                    registry.get_event_bus().publish(ecs::EnemyKilledEvent{event.target, 100});
                    auto& positions = registry.get_components<Position>();
                    if (positions.has_entity(event.target)) {
                        const Position& pos = positions[event.target];

                        static std::mt19937 rng(std::random_device{}());
                        constexpr float PI = 3.1415926535f;
                        std::uniform_int_distribution<int> count_dist(3, 6);
                        std::uniform_real_distribution<float> radius_dist(18.0f, 60.0f);
                        std::uniform_real_distribution<float> angle_dist(0.0f, 2.0f * PI);
                        std::uniform_real_distribution<float> scale_dist(0.6f, 1.0f);

                        int explosions = count_dist(rng);
                        for (int i = 0; i < explosions; ++i) {
                            float radius = radius_dist(rng);
                            float angle = angle_dist(rng);
                            float offsetX = std::cos(angle) * radius;
                            float offsetY = std::sin(angle) * radius;
                            float scale = scale_dist(rng);
                            registry.get_event_bus().publish(ecs::ExplosionEvent{
                                event.target,
                                pos.x + offsetX,
                                pos.y + offsetY,
                                scale
                            });
                        }
                    }
                }

                registry.add_component(event.target, ToDestroy{});
                std::cout << "[HealthSystem] ToDestroy component added to entity " << event.target << std::endl;

                if (isPlayer)
                    std::cout << "GAME OVER! Player died!" << std::endl;
                else if (isEnemy)
                    std::cout << "Enemy " << event.target << " destroyed!" << std::endl;
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
