/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MovementSystem
*/

#include "ecs/systems/MovementSystem.hpp"
#include "ecs/events/InputEvents.hpp"
#include <iostream>
#include <cmath>

void MovementSystem::init(Registry& registry)
{
    std::cout << "MovementSystem: Initialisation." << std::endl;

    auto& eventBus = registry.get_event_bus();
    moveSubId_ = eventBus.subscribe<ecs::PlayerMoveEvent>([&registry](const ecs::PlayerMoveEvent& event) {
        auto& velocities = registry.get_components<Velocity>();
        auto& controllables = registry.get_components<Controllable>();

        if (!velocities.has_entity(event.player) || !controllables.has_entity(event.player)) {
            return;
        }

        auto& vel = velocities[event.player];
        const auto& ctrl = controllables[event.player];

        float dirX = event.directionX;
        float dirY = event.directionY;

        float hypothenus = std::sqrt(dirX * dirX + dirY * dirY);

        if (hypothenus > 0.0f) {
            dirX = (dirX / hypothenus) * ctrl.speed;
            dirY = (dirY / hypothenus) * ctrl.speed;

            vel.x = dirX;
            vel.y = dirY;
        } else {
            vel.x = 0.0f;
            vel.y = 0.0f;
        }
    });
}

void MovementSystem::shutdown()
{
    std::cout << "MovementSystem: Arrêt." << std::endl;
}

void MovementSystem::update(Registry& registry, float dt)
{
    (void)dt;
    (void)registry;

    // Le système ne fait plus rien dans update() car tout est géré par l'événement
    // La logique de mouvement est déclenchée par les événements PlayerMoveEvent
}
