/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ShootingSystem
*/

#include "ecs/systems/ShootingSystem.hpp"
#include "ecs/events/InputEvents.hpp"
#include "ecs/events/GameEvents.hpp" // Include for ShotFiredEvent
#include <iostream>

ShootingSystem::ShootingSystem(engine::TextureHandle bulletTexture, float bulletWidth, float bulletHeight)
    : bulletTexture_(bulletTexture), bulletWidth_(bulletWidth), bulletHeight_(bulletHeight)
{
}

void ShootingSystem::init(Registry& registry)
{
    std::cout << "ShootingSystem: Initialisation." << std::endl;

    auto& eventBus = registry.get_event_bus();
    fireSubId_ = eventBus.subscribe<ecs::PlayerFireEvent>([this, &registry](const ecs::PlayerFireEvent& event) {
        auto& positions = registry.get_components<Position>();
        auto& fireRates = registry.get_components<FireRate>();
        auto& sprites = registry.get_components<Sprite>();

        if (!positions.has_entity(event.player))
            return;

        if (!fireRates.has_entity(event.player))
            return;

        auto& fireRate = fireRates[event.player];

        if (fireRate.time_since_last_fire < fireRate.cooldown)
            return;  // Pas encore le moment de tirer


        fireRate.time_since_last_fire = 0.0f;

        const Position& playerPos = positions[event.player];

        float playerHeight = 0.0f;
        if (sprites.has_entity(event.player))
            playerHeight = sprites[event.player].height;

        Entity projectile = registry.spawn_entity();

        float bulletOffsetX = 50.0f;  // Décalage à droite du joueur
        float bulletOffsetY = (playerHeight / 2.0f) - (bulletHeight_ / 2.0f);  // Centré verticalement

        registry.add_component(projectile, Position{
            playerPos.x + bulletOffsetX,
            playerPos.y + bulletOffsetY
        });

        float bulletSpeed = 400.0f;
        registry.add_component(projectile, Velocity{bulletSpeed, 0.0f});

        registry.add_component(projectile, Collider{bulletWidth_, bulletHeight_});

        registry.add_component(projectile, Sprite{
            bulletTexture_,
            bulletWidth_,
            bulletHeight_,
            0.0f,
            engine::Color::White,
            0.0f,
            0.0f,
            0
        });

        registry.add_component(projectile, Projectile{});

        // Publish ShotFiredEvent
        registry.get_event_bus().publish(ecs::ShotFiredEvent{event.player, projectile});
    });
}

void ShootingSystem::shutdown()
{
    std::cout << "ShootingSystem: Arrêt." << std::endl;
}

void ShootingSystem::update(Registry& registry, float dt)
{
    // Mettre à jour le cooldown de toutes les entités avec FireRate
    auto& fireRates = registry.get_components<FireRate>();

    for (size_t i = 0; i < fireRates.size(); i++) {
        Entity entity = fireRates.get_entity_at(i);

        if (!fireRates.has_entity(entity))
            continue;

        auto& fireRate = fireRates[entity];
        fireRate.time_since_last_fire += dt;
    }
}
