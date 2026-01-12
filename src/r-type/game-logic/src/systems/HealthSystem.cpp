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
        [this, &registry](const ecs::DamageEvent& event) {
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

                        // Check if enemy should drop a bonus
                        if (enemies.has_entity(event.target)) {
                            const Enemy& enemy = enemies[event.target];
                            // FORCE BONUS_WEAPON DROP FOR TESTING - toujours drop un bonus weapon
                            registry.get_event_bus().publish(ecs::BonusSpawnEvent{
                                pos.x, pos.y, static_cast<int>(BonusType::BONUS_WEAPON)
                            });
                            std::cout << "[HealthSystem] FORCED BONUS_WEAPON spawn at (" << pos.x << ", " << pos.y << ")" << std::endl;
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

void HealthSystem::spawnBonusAtPosition(Registry& registry, BonusType type, float x, float y)
{
    constexpr float BONUS_RADIUS = 40.0f;  // Rayon plus grand pour être visible

    // Couleur selon le type de bonus
    engine::Color tint;
    std::string typeName;
    switch (type) {
        case BonusType::HEALTH:
            tint = engine::Color::Green;
            typeName = "HP";
            break;
        case BonusType::SHIELD:
            tint = engine::Color::Purple;
            typeName = "Bouclier";
            break;
        case BonusType::SPEED:
            tint = engine::Color::SpeedBlue;
            typeName = "Vitesse";
            break;
        case BonusType::BONUS_WEAPON:
            tint = engine::Color::Yellow;
            typeName = "Arme Bonus";
            break;
    }

    engine::Vector2f texSize = {BONUS_RADIUS * 2, BONUS_RADIUS * 2};

    Entity bonus = registry.spawn_entity();
    registry.add_component(bonus, Position{x, y});
    registry.add_component(bonus, Bonus{type, BONUS_RADIUS});
    registry.add_component(bonus, Collider{BONUS_RADIUS * 2, BONUS_RADIUS * 2});
    registry.add_component(bonus, Scrollable{1.0f, false, true}); // Scroll et détruit hors écran

    Sprite sprite{
        engine::INVALID_HANDLE,  // Texture will be set client-side
        texSize.x,
        texSize.y,
        0.0f,
        tint,
        0.0f,
        0.0f,
        0  // Layer
    };
    sprite.source_rect = {0.0f, 0.0f, 16.0f, 16.0f};
    sprite.origin_x = texSize.x / 2.0f;
    sprite.origin_y = texSize.y / 2.0f;
    registry.add_component(bonus, sprite);

    std::cout << "[HealthSystem] Spawned bonus " << typeName << " at (" << x << ", " << y << ")" << std::endl;
}
