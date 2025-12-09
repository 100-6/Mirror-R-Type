/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ShootingSystem
*/

#include "ecs/systems/ShootingSystem.hpp"
#include "ecs/events/InputEvents.hpp"
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

        if (weapon.time_since_last_fire < weapon.fire_rate)
            return;

        weapon.time_since_last_fire = 0.0f;

        const Position& playerPos = positions[event.player];

        float playerHeight = 0.0f;
        if (sprites.has_entity(event.player))
            playerHeight = sprites[event.player].height;

        switch (weapon.type) {
            case WeaponType::BASIC:
                createBasicProjectile(registry, weapon, playerPos, playerHeight);
                break;

            case WeaponType::SPREAD:
                createSpreadProjectiles(registry, weapon, playerPos, playerHeight);
                break;

            case WeaponType::BURST:
                createBurstProjectiles(registry, weapon, playerPos, playerHeight);
                break;

            case WeaponType::LASER:
                // TODO: Implémenter plus tard
                break;
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

    for (size_t i = 0; i < weapons.size(); i++) {
        Entity entity = weapons.get_entity_at(i);

        if (!weapons.has_entity(entity))
            continue;

        auto& weapon = weapons[entity];
        weapon.time_since_last_fire += dt;
    }

    // Mettre à jour le temps de vie des projectiles
    auto& projectiles = registry.get_components<Projectile>();

    for (size_t i = 0; i < projectiles.size(); i++) {
        Entity entity = projectiles.get_entity_at(i);

        if (!projectiles.has_entity(entity))
            continue;

        auto& projectile = projectiles[entity];
        projectile.time_alive += dt;

        // Marquer pour destruction si le temps de vie est dépassé
        if (projectile.time_alive >= projectile.lifetime)
            registry.add_component(entity, ToDestroy{});
    }
}

void ShootingSystem::createBasicProjectile(Registry& registry, const Weapon& weapon, const Position& shooterPos, float shooterHeight)
{
    Entity projectile = registry.spawn_entity();

    float bulletOffsetX = 50.0f;
    float bulletOffsetY = (shooterHeight / 2.0f) - (weapon.projectile_sprite.height / 2.0f);

    registry.add_component(projectile, Position{
        shooterPos.x + bulletOffsetX,
        shooterPos.y + bulletOffsetY
    });

    registry.add_component(projectile, Velocity{weapon.projectile_speed, 0.0f});
    registry.add_component(projectile, Collider{weapon.projectile_sprite.width, weapon.projectile_sprite.height});

    // Copie du sprite en créant un nouveau Sprite
    // Normalement en mémoir c'est OK je suis pas sur a 100% car c'est que 48 bytes
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

    registry.add_component(projectile, Projectile{0.0f, 5.0f, 0.0f});  // Angle 0° = droite
}

void ShootingSystem::createSpreadProjectiles(Registry& registry, const Weapon& weapon, const Position& shooterPos, float shooterHeight)
{
    // Calculer l'écart d'angle entre chaque projectile
    float angleStep = 0.0f;
    float startAngle = 0.0f;

    if (weapon.projectile_count > 1) {
        angleStep = weapon.spread_angle / (weapon.projectile_count - 1);
        startAngle = -weapon.spread_angle / 2.0f;
    }

    for (int i = 0; i < weapon.projectile_count; i++) {
        float angle = startAngle + (angleStep * i);
        float radians = angle * (M_PI / 180.0f);

        Entity projectile = registry.spawn_entity();

        float bulletOffsetX = 50.0f;
        float bulletOffsetY = (shooterHeight / 2.0f) - (weapon.projectile_sprite.height / 2.0f);

        registry.add_component(projectile, Position{
            shooterPos.x + bulletOffsetX,
            shooterPos.y + bulletOffsetY
        });

        // Calculer la vélocité selon l'angle
        float vx = weapon.projectile_speed * std::cos(radians);
        float vy = weapon.projectile_speed * std::sin(radians);

        registry.add_component(projectile, Velocity{vx, vy});
        registry.add_component(projectile, Collider{weapon.projectile_sprite.width, weapon.projectile_sprite.height});

        // Copie du sprite en créant un nouveau Sprite
        // Normalement en mémoir c'est OK je suis pas sur a 100% car c'est que 48 bytes
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

        registry.add_component(projectile, Projectile{angle, 5.0f, 0.0f});
    }
}

void ShootingSystem::createBurstProjectiles(Registry& registry, Weapon& weapon, const Position& shooterPos, float shooterHeight)
{
    // BURST: tire rapidement plusieurs projectiles l'un après l'autre
    // On crée juste 1 projectile mais on réduit temporairement le cooldown

    Entity projectile = registry.spawn_entity();

    float bulletOffsetX = 50.0f;
    float bulletOffsetY = (shooterHeight / 2.0f) - (weapon.projectile_sprite.height / 2.0f);

    registry.add_component(projectile, Position{
        shooterPos.x + bulletOffsetX,
        shooterPos.y + bulletOffsetY
    });
    
    registry.add_component(projectile, Velocity{weapon.projectile_speed, 0.0f});
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
    
    registry.add_component(projectile, Projectile{0.0f, 5.0f, 0.0f});
    
    // Réduire le cooldown pour tirer rapidement (rafale de projectile_count tirs)
    // Après projectile_count tirs, revenir au fire_rate normal
    static int burstCount = 0;
    burstCount++;
    
    if (burstCount < weapon.projectile_count)
        weapon.time_since_last_fire = weapon.fire_rate - 0.05f;  // Tire après 0.05s
    else
        burstCount = 0;  // Reset pour la prochaine rafale
}
