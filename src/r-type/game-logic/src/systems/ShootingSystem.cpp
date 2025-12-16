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

void ShootingSystem::init(Registry& registry)
{
    std::cout << "ShootingSystem: Initialisation." << std::endl;

    auto& eventBus = registry.get_event_bus();
    fireSubId_ = eventBus.subscribe<ecs::PlayerFireEvent>([this, &registry](const ecs::PlayerFireEvent& event) {
        auto& positions = registry.get_components<Position>();
        auto& weapons = registry.get_components<Weapon>();
        auto& sprites = registry.get_components<Sprite>();

        if (!positions.has_entity(event.player))
            return;

        if (!weapons.has_entity(event.player))
            return;

        auto& weapon = weapons[event.player];

        // Récupérer les stats de l'arme
        int projectiles; float spread, speed, firerate, burst_delay;
        get_weapon_stats(weapon.type, projectiles, spread, speed, firerate, burst_delay);

        // Vérifier le cooldown
        if (weapon.time_since_last_fire < firerate)
            return;

        // Pour les rafales, gérer le compteur
        if (weapon.type == WeaponType::BURST && weapon.burst_count > 0) {
            if (weapon.time_since_last_fire < burst_delay)
                return;
        }

        const Position& playerPos = positions[event.player];

        float playerHeight = 0.0f;
        if (sprites.has_entity(event.player))
            playerHeight = sprites[event.player].height;

        // Fonction générique pour tous les types d'armes
        createProjectiles(registry, event.player, weapon, playerPos, playerHeight);
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

        // Tir automatique pour les ennemis uniquement
        if (enemies.has_entity(entity) && positions.has_entity(entity)) {
            int projectiles; float spread, speed, firerate, burst_delay;
            get_weapon_stats(weapon.type, projectiles, spread, speed, firerate, burst_delay);

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
                    engine::Color{255, 100, 100, 255},
                    0.0f,
                    0.0f,
                    0
                });
                registry.add_component(projectile, Damage{10});
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

void ShootingSystem::createProjectiles(Registry& registry, Entity shooter, Weapon& weapon, const Position& shooterPos, float shooterHeight)
{
    int projectiles;
    float spread, speed, firerate, burst_delay;

    get_weapon_stats(weapon.type, projectiles, spread, speed, firerate, burst_delay);

    // Récupérer la largeur du shooter pour positionner le projectile à l'extrémité droite
    auto& sprites = registry.get_components<Sprite>();
    float shooterWidth = 0.0f;
    if (sprites.has_entity(shooter)) {
        shooterWidth = sprites[shooter].width;
    }

    int projectile_count = projectiles;
    float startAngle = 0.0f;
    float angleStep = 0.0f;

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
        float bulletOffsetX = shooterWidth;
        float bulletOffsetY = (shooterHeight / 2.0f) - (weapon.projectile_sprite.height / 2.0f);

        registry.add_component(projectile, Position{
            shooterPos.x + bulletOffsetX,
            shooterPos.y + bulletOffsetY
        });

        float vx = speed * std::cos(radians);
        float vy = speed * std::sin(radians);

        registry.add_component(projectile, Velocity{vx, vy});
        registry.add_component(projectile, Collider{weapon.projectile_sprite.width, weapon.projectile_sprite.height});

        registry.add_component(projectile, Sprite{
            weapon.projectile_sprite.texture,
            weapon.projectile_sprite.width,
            weapon.projectile_sprite.height,
            weapon.projectile_sprite.rotation,
            weapon.projectile_sprite.tint,
            weapon.projectile_sprite.origin_x,
            weapon.projectile_sprite.origin_y,
            weapon.projectile_sprite.layer
        });

        registry.add_component(projectile, Projectile{angle, 5.0f, 0.0f, ProjectileFaction::Player});
        registry.add_component(projectile, NoFriction{});

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
