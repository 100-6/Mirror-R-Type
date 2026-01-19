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

        // Pour le laser beam, activer le composant LaserBeam
        if (weapon.type == WeaponType::LASER) {
            auto& lasers = registry.get_components<LaserBeam>();
            if (!lasers.has_entity(event.player)) {
                registry.add_component(event.player, LaserBeam{
                    true, // active
                    WEAPON_LASER_RANGE,
                    0.0f, // current_length
                    static_cast<float>(WEAPON_LASER_DAMAGE_PER_TICK),
                    WEAPON_LASER_TICK_RATE,
                    0.0f, // time_since_last_tick
                    WEAPON_LASER_WIDTH,
                    engine::Color{WEAPON_LASER_COLOR_R, WEAPON_LASER_COLOR_G, WEAPON_LASER_COLOR_B, WEAPON_LASER_COLOR_A},
                    engine::Color{WEAPON_LASER_CORE_COLOR_R, WEAPON_LASER_CORE_COLOR_G, WEAPON_LASER_CORE_COLOR_B, WEAPON_LASER_CORE_COLOR_A},
                    0.0f, 0.0f // hit_x, hit_y
                });
            } else {
                lasers[event.player].active = true;
            }
            return;
        }

        // Pour les armes qui tirent des projectiles, on tire immédiatement si le cooldown est prêt
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

        // Pour le laser beam, désactiver le composant
        if (weapon.type == WeaponType::LASER) {
            auto& lasers = registry.get_components<LaserBeam>();
            if (lasers.has_entity(event.player)) {
                lasers[event.player].active = false;
            }
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

    for (size_t i = 0; i < weapons.size(); i++) {
        Entity entity = weapons.get_entity_at(i);

        if (!weapons.has_entity(entity))
            continue;

        auto& weapon = weapons[entity];
        weapon.time_since_last_fire += dt;

        // Logique Joueur
        if (!enemies.has_entity(entity)) {
            if (weapon.trigger_held) {
                if (weapon.type == WeaponType::LASER) {
                    // Le laser beam est géré par updateLaserBeam
                    if (positions.has_entity(entity)) {
                        auto& colliders = registry.get_components<Collider>();
                        float w = sprites.has_entity(entity) ? sprites[entity].width :
                                  (colliders.has_entity(entity) ? colliders[entity].width : 0.0f);
                        float h = sprites.has_entity(entity) ? sprites[entity].height :
                                  (colliders.has_entity(entity) ? colliders[entity].height : 0.0f);
                        updateLaserBeam(registry, entity, positions[entity], w, h, dt);
                    }
                } else {
                    // Tir automatique (projectiles)
                    int projectiles; int damage; float spread, speed, firerate, burst_delay;
                    get_weapon_stats(weapon.type, projectiles, spread, damage, speed, firerate, burst_delay);

                    if (weapon.time_since_last_fire >= firerate) {
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
                registry.add_component(projectile, ProjectileOwner{entity});  // Track which enemy fired this
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

    // Dimensions des projectiles
    float actual_width = weapon.projectile_sprite.width;
    float actual_height = weapon.projectile_sprite.height;
    int actual_damage = damage;
    engine::Color actual_color = weapon.projectile_sprite.tint;

    // Fallback dimensions if no sprite/graphics
    if (!graphics_) {
        actual_width = 20.0f;
        actual_height = 10.0f;
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

        // Positionner le projectile au centre du vaisseau (origine déjà centrée)
        // Le vaisseau est maintenant centré grâce au source_rect, donc pas besoin d'offset complexe
        float bulletOffsetX = shooterWidth / 2.0f + 5.0f;  // Du centre vers la droite
        float bulletOffsetY = 0.0f;  // Au centre verticalement

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
        registry.add_component(projectile, ProjectileOwner{shooter});  // Track who fired this projectile
        registry.add_component(projectile, NoFriction{});

        // Event for ServerNetworkSystem to pick up - MUST be published AFTER all components are added
        registry.get_event_bus().publish(ecs::ShotFiredEvent{shooter, projectile});
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

void ShootingSystem::updateLaserBeam(Registry& registry, Entity shooter, const Position& shooterPos, float shooterWidth, float shooterHeight, float dt)
{
    auto& lasers = registry.get_components<LaserBeam>();
    if (!lasers.has_entity(shooter))
        return;

    LaserBeam& beam = lasers[shooter];
    if (!beam.active)
        return;

    // Calculer la position de départ du rayon (devant le vaisseau)
    float startX = shooterPos.x + shooterWidth / 2.0f + 5.0f;
    float startY = shooterPos.y;

    // Effectuer le raycast pour trouver le point de collision
    performLaserRaycast(registry, startX, startY, beam.range, shooter, beam);

    // Appliquer les dégâts sur tick
    beam.time_since_last_tick += dt;
    if (beam.time_since_last_tick >= beam.tick_rate) {
        beam.time_since_last_tick = 0.0f;

        // Appliquer les dégâts à l'entité touchée (s'il y en a une)
        auto& healths = registry.get_components<Health>();
        auto& enemies = registry.get_components<Enemy>();

        // Parcourir les ennemis pour trouver ceux touchés par le rayon
        auto& positions = registry.get_components<Position>();
        auto& colliders = registry.get_components<Collider>();

        for (size_t i = 0; i < enemies.size(); i++) {
            Entity enemy = enemies.get_entity_at(i);
            if (!positions.has_entity(enemy) || !colliders.has_entity(enemy))
                continue;

            const Position& enemyPos = positions[enemy];
            const Collider& enemyCol = colliders[enemy];

            // Vérifier si le rayon traverse l'ennemi
            float half_h = enemyCol.height * 0.5f;
            float top = enemyPos.y - half_h;
            float bottom = enemyPos.y + half_h;

            // Le rayon horizontal est-il dans les limites verticales de l'ennemi ?
            if (startY >= top && startY <= bottom) {
                float half_w = enemyCol.width * 0.5f;
                float left = enemyPos.x - half_w;

                // L'ennemi est-il entre le début et la fin du rayon ?
                if (left >= startX && left <= beam.hit_x) {
                    // Appliquer les dégâts
                    if (healths.has_entity(enemy)) {
                        healths[enemy].current -= static_cast<int>(beam.damage_per_tick);
                        if (healths[enemy].current <= 0) {
                            registry.add_component(enemy, ToDestroy{});
                            // Publier l'événement pour le score (100 points par défaut)
                            registry.get_event_bus().publish(ecs::EnemyKilledEvent{enemy, 100, shooter});
                        }
                    }
                    // Arrêter au premier ennemi touché
                    break;
                }
            }
        }
    }
}


void ShootingSystem::performLaserRaycast(Registry& registry, float startX, float startY, float range, Entity shooter, LaserBeam& beam)
{
    auto& positions = registry.get_components<Position>();
    auto& colliders = registry.get_components<Collider>();
    auto& enemies = registry.get_components<Enemy>();
    auto& walls = registry.get_components<Wall>();

    // Point final par défaut (pas de collision)
    beam.hit_x = startX + range;
    beam.hit_y = startY;
    beam.current_length = range;

    float closest_hit = range;

    // Vérifier la collision avec les ennemis
    for (size_t i = 0; i < enemies.size(); i++) {
        Entity enemy = enemies.get_entity_at(i);
        if (!positions.has_entity(enemy) || !colliders.has_entity(enemy))
            continue;

        const Position& enemyPos = positions[enemy];
        const Collider& enemyCol = colliders[enemy];

        // Intersection AABB avec une ligne horizontale
        float half_w = enemyCol.width * 0.5f;
        float half_h = enemyCol.height * 0.5f;

        float left = enemyPos.x - half_w;
        float top = enemyPos.y - half_h;
        float bottom = enemyPos.y + half_h;

        // Le rayon Y est-il dans les limites de l'ennemi et l'ennemi est-il devant ?
        if (startY >= top && startY <= bottom && left > startX && left < startX + range) {
            float hit_dist = left - startX;
            if (hit_dist < closest_hit) {
                closest_hit = hit_dist;
            }
        }
    }

    // Vérifier la collision avec les murs
    for (size_t i = 0; i < walls.size(); i++) {
        Entity wall = walls.get_entity_at(i);
        if (!positions.has_entity(wall) || !colliders.has_entity(wall))
            continue;

        const Position& wallPos = positions[wall];
        const Collider& wallCol = colliders[wall];

        float half_w = wallCol.width * 0.5f;
        float half_h = wallCol.height * 0.5f;

        float left = wallPos.x - half_w;
        float top = wallPos.y - half_h;
        float bottom = wallPos.y + half_h;

        if (startY >= top && startY <= bottom && left > startX && left < startX + range) {
            float hit_dist = left - startX;
            if (hit_dist < closest_hit) {
                closest_hit = hit_dist;
            }
        }
    }

    beam.current_length = closest_hit;
    beam.hit_x = startX + closest_hit;
    beam.hit_y = startY;
}
