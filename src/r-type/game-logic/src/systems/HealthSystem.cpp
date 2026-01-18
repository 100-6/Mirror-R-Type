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
                    // Get the killer entity from the projectile owner
                    Entity killer = 0;
                    auto& projectileOwners = registry.get_components<ProjectileOwner>();
                    if (projectileOwners.has_entity(event.source)) {
                        killer = projectileOwners[event.source].owner;
                    }
                    registry.get_event_bus().publish(ecs::EnemyKilledEvent{event.target, 100, killer});
                    auto& positions = registry.get_components<Position>();
                    auto& ais = registry.get_components<AI>();
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

                        // Emit explosion sound event based on enemy type
                        ecs::ExplosionSoundEvent::ExplosionType soundType =
                            ecs::ExplosionSoundEvent::ExplosionType::ENEMY_BASIC;
                        if (ais.has_entity(event.target)) {
                            const AI& ai = ais[event.target];
                            switch (ai.type) {
                                case EnemyType::Boss:
                                    soundType = ecs::ExplosionSoundEvent::ExplosionType::ENEMY_BOSS;
                                    break;
                                case EnemyType::Tank:
                                    soundType = ecs::ExplosionSoundEvent::ExplosionType::ENEMY_TANK;
                                    break;
                                default:
                                    soundType = ecs::ExplosionSoundEvent::ExplosionType::ENEMY_BASIC;
                                    break;
                            }
                        }
                        registry.get_event_bus().publish(ecs::ExplosionSoundEvent{
                            soundType, pos.x, pos.y, 1.0f
                        });

                        // Random chance to drop a bonus (20% chance)
                        std::uniform_real_distribution<float> drop_chance(0.0f, 1.0f);
                        constexpr float BONUS_DROP_RATE = 0.20f;  // 20% chance to drop

                        if (drop_chance(rng) <= BONUS_DROP_RATE) {
                            // Choose random bonus type: only HEALTH and BONUS_WEAPON
                            std::uniform_int_distribution<int> bonus_type_dist(0, 1);
                            int bonus_type_id = bonus_type_dist(rng);
                            BonusType bonus_type;
                            std::string bonus_name;

                            if (bonus_type_id == 0) {
                                bonus_type = BonusType::HEALTH;
                                bonus_name = "HEALTH";
                            } else {
                                bonus_type = BonusType::BONUS_WEAPON;
                                bonus_name = "BONUS_WEAPON";
                            }

                            registry.get_event_bus().publish(ecs::BonusSpawnEvent{
                                pos.x, pos.y, static_cast<int>(bonus_type)
                            });
                            std::cout << "[HealthSystem] Enemy dropped " << bonus_name
                                      << " bonus at (" << pos.x << ", " << pos.y << ")" << std::endl;
                        }
                    }
                }

                registry.add_component(event.target, ToDestroy{});
                std::cout << "[HealthSystem] ToDestroy component added to entity " << event.target << std::endl;

                if (isPlayer) {
                    // Emit player explosion sound
                    auto& positions = registry.get_components<Position>();
                    if (positions.has_entity(event.target)) {
                        const Position& pos = positions[event.target];
                        registry.get_event_bus().publish(ecs::ExplosionSoundEvent{
                            ecs::ExplosionSoundEvent::ExplosionType::PLAYER,
                            pos.x, pos.y, 1.0f
                        });
                    }
                    // Destroy companion turret when player dies
                    registry.get_event_bus().publish(ecs::CompanionDestroyEvent{event.target});
                    std::cout << "GAME OVER! Player died!" << std::endl;
                }
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
