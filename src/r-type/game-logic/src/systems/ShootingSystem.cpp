/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ShootingSystem
*/

#include "systems/ShootingSystem.hpp"
#include "components/CombatHelpers.hpp"
#include "ecs/events/InputEvents.hpp"
#include "ecs/events/GameEvents.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

// Define M_PI if not available (MSVC)
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

void ShootingSystem::init(Registry& registry)
{
    std::cout << "ShootingSystem: Initialisation." << std::endl;

    auto& eventBus = registry.get_event_bus();

    // Trigger Pressed
    eventBus.subscribe<ecs::PlayerStartFireEvent>([this, &registry](const ecs::PlayerStartFireEvent& event) {
        auto& weapons = registry.get_components<Weapon>();
        if (!weapons.has_entity(event.player)) {
            std::cout << "[SHOOT] PlayerStartFireEvent: entity " << event.player << " has no Weapon!\n";
            return;
        }

        auto& weapon = weapons[event.player];
        weapon.trigger_held = true;
        std::cout << "[SHOOT] PlayerStartFireEvent received for entity " << event.player << ", trigger_held now true\n";

        // Pour les armes automatiques, on tire immédiatement si le cooldown est prêt
        if (weapon.type != WeaponType::CHARGE) {
            int projectiles; int damage; float spread, speed, firerate, burst_delay;
            get_weapon_stats(weapon.type, projectiles, spread, damage, speed, firerate, burst_delay);

            if (weapon.time_since_last_fire >= firerate) {
                auto& positions = registry.get_components<Position>();
                auto& sprites = registry.get_components<Sprite>();
                auto& colliders = registry.get_components<Collider>();

                if (positions.has_entity(event.player)) {
                     float playerWidth = sprites.has_entity(event.player) ? sprites[event.player].width :
                                         (colliders.has_entity(event.player) ? colliders[event.player].width : 0.0f);
                     float playerHeight = sprites.has_entity(event.player) ? sprites[event.player].height :
                                          (colliders.has_entity(event.player) ? colliders[event.player].height : 0.0f);
                     createProjectiles(registry, event.player, weapon, positions[event.player], playerWidth, playerHeight);
                }
            }
        }
    });

    // Trigger Released
    eventBus.subscribe<ecs::PlayerStopFireEvent>([this, &registry](const ecs::PlayerStopFireEvent& event) {
        auto& weapons = registry.get_components<Weapon>();
        if (!weapons.has_entity(event.player)) {
            std::cout << "[SHOOT] PlayerStopFireEvent: entity " << event.player << " has no Weapon!\n";
            return;
        }

        auto& weapon = weapons[event.player];
        weapon.trigger_held = false;
        std::cout << "[SHOOT] PlayerStopFireEvent received for entity " << event.player << ", trigger_held now false\n";

        // Pour le tir chargé, on tire au relâchement
        if (weapon.type == WeaponType::CHARGE) {
            auto& positions = registry.get_components<Position>();
            auto& sprites = registry.get_components<Sprite>();
            auto& colliders = registry.get_components<Collider>();

            if (positions.has_entity(event.player)) {
                float playerWidth = sprites.has_entity(event.player) ? sprites[event.player].width :
                                    (colliders.has_entity(event.player) ? colliders[event.player].width : 0.0f);
                float playerHeight = sprites.has_entity(event.player) ? sprites[event.player].height :
                                     (colliders.has_entity(event.player) ? colliders[event.player].height : 0.0f);
                createProjectiles(registry, event.player, weapon, positions[event.player], playerWidth, playerHeight);
            }

            // Note: L'effet visuel sera caché automatiquement par l'update
            weapon.is_charging = false;
            weapon.current_charge_duration = 0.0f;
            weapon.time_since_last_fire = 0.0f;
        }
    });
}

void ShootingSystem::shutdown()
{
    std::cout << "ShootingSystem: Arrêt." << std::endl;
}

void ShootingSystem::update(Registry& registry, float dt)
{
    // Mettre à jour le cooldown de toutes les armes
    auto& weapons = registry.get_components<Weapon>();
    auto& enemies = registry.get_components<Enemy>();
    auto& positions = registry.get_components<Position>();
    auto& sprites = registry.get_components<Sprite>();
    auto& attacheds = registry.get_components<Attached>();

    for (size_t i = 0; i < weapons.size(); i++) {
        Entity entity = weapons.get_entity_at(i);

        if (!weapons.has_entity(entity))
            continue;

        auto& weapon = weapons[entity];
        weapon.time_since_last_fire += dt;

        // Logique Joueur
        if (!enemies.has_entity(entity)) {
            static int trigger_check_counter = 0;
            if (++trigger_check_counter % 60 == 0 && weapon.trigger_held) {
                std::cout << "[SHOOT] Entity " << entity << " has trigger_held=true, type=" << (int)weapon.type << ", cooldown=" << weapon.time_since_last_fire << "\n";
            }

            if (weapon.trigger_held) {
                if (weapon.type == WeaponType::CHARGE) {
                    weapon.is_charging = true;
                    weapon.current_charge_duration += dt;
                    
                    // Création paresseuse de l'effet visuel
                    if (weapon.chargeEffectEntity == (size_t)-1 || !sprites.has_entity(weapon.chargeEffectEntity)) {
                         Entity effect = registry.spawn_entity();
                         weapon.chargeEffectEntity = effect;
                         
                         float pW = sprites.has_entity(entity) ? sprites[entity].width : 0.0f;
                         float pH = sprites.has_entity(entity) ? sprites[entity].height : 0.0f;
                         float startSize = WEAPON_CHARGE_WIDTH_MIN / 2.0f;
                         
                         registry.add_component(effect, Position{0, 0});
                         registry.add_component(effect, Attached{entity, pW + 5.0f, (pH - startSize) / 2.0f});
                         registry.add_component(effect, Sprite{
                            weapon.projectile_sprite.texture,
                            0.0f, 0.0f, 0.0f, engine::Color{0, 150, 255, 150}, 0.0f, 0.0f, 2
                         });
                         // Only set tint/texture if graphics are available or textures loaded
                         if (!graphics_ && weapon.projectile_sprite.texture == engine::INVALID_HANDLE) {
                             // Minimal server logic for effect? Server might not need this effect entity at all.
                         }
                    }

                    // Animer l'effet de charge
                    if (sprites.has_entity(weapon.chargeEffectEntity)) {
                        auto& effectSprite = sprites[weapon.chargeEffectEntity];
                        
                        float t = std::clamp((weapon.current_charge_duration) / (WEAPON_CHARGE_TIME_MAX), 0.0f, 1.0f);
                        
                        // Pulsing size
                        float baseSize = WEAPON_CHARGE_WIDTH_MIN / 1.5f;
                        float maxSize = WEAPON_CHARGE_WIDTH_MAX;
                        float currentSize = baseSize + (maxSize - baseSize) * t;
                        
                        // Ajouter une pulsation rapide
                        float pulse = std::sin(weapon.current_charge_duration * 15.0f) * 5.0f;
                        
                        effectSprite.width = currentSize + pulse;
                        effectSprite.height = currentSize + pulse;
                        
                        // Recalculer l'offset pour rester centré malgré le changement de taille
                        if (attacheds.has_entity(weapon.chargeEffectEntity)) {
                             float pW = sprites.has_entity(entity) ? sprites[entity].width : 0.0f;
                             float pH = sprites.has_entity(entity) ? sprites[entity].height : 0.0f;
                             attacheds[weapon.chargeEffectEntity].offsetY = (pH - effectSprite.height) / 2.0f;
                             attacheds[weapon.chargeEffectEntity].offsetX = pW + 5.0f;
                        }
                        
                        // Rotate
                        effectSprite.tint.r = static_cast<unsigned char>(t * 255);
                        effectSprite.tint.g = static_cast<unsigned char>(150 + (t * 105)); // 150 -> 255
                        effectSprite.tint.b = 255;
                        effectSprite.tint.a = static_cast<unsigned char>(150 + (t * 105)); // Fade in opacity
                    }

                } else {
                    // Tir automatique (continue)
                    int projectiles; int damage; float spread, speed, firerate, burst_delay;
                    get_weapon_stats(weapon.type, projectiles, spread, damage, speed, firerate, burst_delay);

                    if (weapon.time_since_last_fire >= firerate) {
                        if (positions.has_entity(entity)) {
                            std::cout << "[SHOOT] Creating projectiles for entity " << entity << "\n";
                            auto& colliders = registry.get_components<Collider>();
                            float w = sprites.has_entity(entity) ? sprites[entity].width :
                                      (colliders.has_entity(entity) ? colliders[entity].width : 0.0f);
                            float h = sprites.has_entity(entity) ? sprites[entity].height :
                                      (colliders.has_entity(entity) ? colliders[entity].height : 0.0f);
                            createProjectiles(registry, entity, weapon, positions[entity], w, h);
                        }
                    }
                }
            } else {
                // Si on ne charge pas, cacher l'effet visuel
                if (weapon.type == WeaponType::CHARGE && weapon.chargeEffectEntity != (size_t)-1) {
                    if (sprites.has_entity(weapon.chargeEffectEntity)) {
                        sprites[weapon.chargeEffectEntity].tint.a = 0;
                    }
                }
            }
            
            // Gestion de la rafale (BURST) même si trigger relâché
            if (weapon.type == WeaponType::BURST && weapon.burst_count > 0) {
                 int projectiles; int damage; float spread, speed, firerate, burst_delay;
                 get_weapon_stats(weapon.type, projectiles, spread, damage, speed, firerate, burst_delay);
                 
                 if (weapon.time_since_last_fire >= burst_delay) {
                     if (positions.has_entity(entity)) {
                        auto& colliders = registry.get_components<Collider>();
                        float w = sprites.has_entity(entity) ? sprites[entity].width :
                                  (colliders.has_entity(entity) ? colliders[entity].width : 0.0f);
                        float h = sprites.has_entity(entity) ? sprites[entity].height :
                                  (colliders.has_entity(entity) ? colliders[entity].height : 0.0f);
                        createProjectiles(registry, entity, weapon, positions[entity], w, h);
                     }
                 }
            }
        }

        // Tir automatique pour les ennemis uniquement
        if (enemies.has_entity(entity) && positions.has_entity(entity)) {
            int projectiles; int damage; float spread, speed, firerate, burst_delay;
            get_weapon_stats(weapon.type, projectiles, spread, damage, speed, firerate, burst_delay);

            if (weapon.time_since_last_fire >= firerate) {
                weapon.time_since_last_fire = 0.0f;

                const Position& enemyPos = positions[entity];
                float enemyHeight = sprites.has_entity(entity) ? sprites[entity].height : 0.0f;

                Entity projectile = registry.spawn_entity();

                float bulletOffsetX = -weapon.projectile_sprite.width - 10.0f;
                float bulletOffsetY = (enemyHeight / 2.0f) - (weapon.projectile_sprite.height / 2.0f);

                registry.add_component(projectile, Position{
                    enemyPos.x + bulletOffsetX,
                    enemyPos.y + bulletOffsetY
                });

                registry.add_component(projectile, Velocity{-speed, 0.0f});
                registry.add_component(projectile, Collider{weapon.projectile_sprite.width, weapon.projectile_sprite.height});
                registry.add_component(projectile, Sprite{
                    weapon.projectile_sprite.texture,
                    weapon.projectile_sprite.width,
                    weapon.projectile_sprite.height,
                    180.0f,
                    engine::Color{ENEMY_PROJECTILE_COLOR_R, ENEMY_PROJECTILE_COLOR_G, ENEMY_PROJECTILE_COLOR_B, ENEMY_PROJECTILE_COLOR_A},
                    0.0f,
                    0.0f,
                    0
                });
                registry.add_component(projectile, Damage{damage});
                registry.add_component(projectile, Projectile{180.0f, 5.0f, 0.0f, ProjectileFaction::Enemy});
                registry.add_component(projectile, NoFriction{});
            }
        }
    }

    // Mettre à jour le temps de vie des projectiles
    auto& projectiles = registry.get_components<Projectile>();

    for (size_t i = 0; i < projectiles.size(); i++) {
        Entity entity = projectiles.get_entity_at(i);

        if (!projectiles.has_entity(entity))
            continue;

        auto& projectile = projectiles[entity];
        projectile.time_alive += dt;

        if (projectile.time_alive >= projectile.lifetime)
            registry.add_component(entity, ToDestroy{});
    }
}

void ShootingSystem::createProjectiles(Registry& registry, Entity shooter, Weapon& weapon, const Position& shooterPos, float shooterWidth, float shooterHeight)
{
    int projectiles; int damage;
    float spread, speed, firerate, burst_delay;

    get_weapon_stats(weapon.type, projectiles, spread, damage, speed, firerate, burst_delay);

    int projectile_count = projectiles;
    float startAngle = 0.0f;
    float angleStep = 0.0f;

    // Calcul des stats dynamiques (pour le tir chargé)
    float actual_width = weapon.projectile_sprite.width;
    float actual_height = weapon.projectile_sprite.height;
    int actual_damage = damage;
    engine::Color actual_color = weapon.projectile_sprite.tint;

    if (weapon.type == WeaponType::CHARGE) {
        float t = std::clamp((weapon.current_charge_duration - WEAPON_CHARGE_TIME_MIN) / (WEAPON_CHARGE_TIME_MAX - WEAPON_CHARGE_TIME_MIN), 0.0f, 1.0f);
        
        if (weapon.current_charge_duration < WEAPON_CHARGE_TIME_MIN) {
             // Non chargé (tir de base)
             actual_damage = WEAPON_CHARGE_DAMAGE_MIN;
             actual_width = WEAPON_CHARGE_WIDTH_MIN;
             actual_height = WEAPON_CHARGE_HEIGHT_MIN;
        } else {
             // Chargé
             actual_damage = static_cast<int>(WEAPON_CHARGE_DAMAGE_MIN + t * (WEAPON_CHARGE_DAMAGE_MAX - WEAPON_CHARGE_DAMAGE_MIN));
             actual_width = WEAPON_CHARGE_WIDTH_MIN + t * (WEAPON_CHARGE_WIDTH_MAX - WEAPON_CHARGE_WIDTH_MIN);
             actual_height = WEAPON_CHARGE_HEIGHT_MIN + t * (WEAPON_CHARGE_HEIGHT_MAX - WEAPON_CHARGE_HEIGHT_MIN);
             
             // Transition de couleur (Bleu -> Blanc/Cyan Brillant)
             // Reste bleu (0, 150, 255) mais devient plus clair ou change
             actual_color.g = static_cast<unsigned char>(150 + (t * 105)); // 150 -> 255
             actual_color.r = static_cast<unsigned char>(t * 200);         // 0 -> 200
        }
    } else {
        // Fallback dimensions if no sprite/graphics
        if (!graphics_) {
            actual_width = 20.0f; // Default projectile width
            actual_height = 10.0f; // Default projectile height
        }
    }

    // Pour SPREAD: calculer les angles d'éventail
    if (weapon.type == WeaponType::SPREAD && projectile_count > 1) {
        angleStep = spread / (projectile_count - 1);
        startAngle = -spread / 2.0f;
    }

    // Pour BURST: ne tirer qu'un projectile à la fois
    if (weapon.type == WeaponType::BURST)
        projectile_count = 1;

    for (int i = 0; i < projectile_count; i++) {
        float angle = startAngle + (angleStep * i);
        float radians = angle * (M_PI / 180.0f);

        Entity projectile = registry.spawn_entity();

        // Positionner le projectile à l'extrémité droite du vaisseau
        float bulletOffsetX = shooterWidth + 5.0f;
        float bulletOffsetY = (shooterHeight / 2.0f) - (actual_height / 2.0f);

        registry.add_component(projectile, Position{
            shooterPos.x + bulletOffsetX,
            shooterPos.y + bulletOffsetY
        });

        float vx = speed * std::cos(radians);
        float vy = speed * std::sin(radians);

        registry.add_component(projectile, Velocity{vx, vy});
        registry.add_component(projectile, Collider{actual_width, actual_height});

        registry.add_component(projectile, Sprite{
            weapon.projectile_sprite.texture,
            actual_width,
            actual_height,
            weapon.projectile_sprite.rotation,
            actual_color,
            weapon.projectile_sprite.origin_x,
            weapon.projectile_sprite.origin_y,
            weapon.projectile_sprite.layer
        });

        registry.add_component(projectile, Damage{actual_damage});
        registry.add_component(projectile, Projectile{angle, 5.0f, 0.0f, ProjectileFaction::Player});
        registry.add_component(projectile, NoFriction{});

        // Event for ServerNetworkSystem to pick up - MUST be published AFTER all components are added
        std::cout << "[SHOOT] Publishing ShotFiredEvent: shooter=" << shooter << " projectile=" << projectile << "\n";
        registry.get_event_bus().publish(ecs::ShotFiredEvent{shooter, projectile});

        // Deprecated direct bus call if we use new event above, keeping it for now but ensuring payload is correct
        // registry.get_event_bus().publish(ecs::ShotFiredEvent{shooter, projectile});
    }

    // Gérer la rafale (BURST)
    if (weapon.type == WeaponType::BURST) {
        weapon.burst_count++;

        if (weapon.burst_count < projectiles)
            // Continuer la rafale avec un délai court
            weapon.time_since_last_fire = 0.0f;
        else {
            // Rafale terminée, reset
            weapon.burst_count = 0;
            weapon.time_since_last_fire = 0.0f;
        }
    } else
        // Pour les autres types, reset simplement le cooldown
        weapon.time_since_last_fire = 0.0f;
}
