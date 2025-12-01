/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MovementSystem
*/

#include "ecs/systems/MovementSystem.hpp"
#include <iostream>
#include <cmath>

void MovementSystem::init(Registry& registry)
{
    std::cout << "MovementSystem: Initialisation." << std::endl;
}

void MovementSystem::shutdown()
{
    std::cout << "MovementSystem: Arrêt." << std::endl;
}

void MovementSystem::update(Registry& registry, float dt)
{
    (void)dt;

    auto& inputs = registry.get_components<Input>();
    auto& velocitys = registry.get_components<Velocity>();
    auto& controlables = registry.get_components<Controllable>();

    for (size_t i = 0; i < controlables.size(); ++i) {
        Entity entity = controlables.get_entity_at(i);

        if (!(velocitys.has_entity(entity)) || !(inputs.has_entity(entity)))
            continue;
        
        auto& vel = velocitys[entity];
        const auto& input = inputs[entity];
        const auto& ctrl = controlables[entity];

        float dirX = 0.0f;
        float dirY = 0.0f;

        if (input.up) dirY -= 1.0f;
        if (input.down) dirY += 1.0f;
        if (input.right) dirX += 1.0f;
        if (input.left) dirX -= 1.0f;
   
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
        
    }
}
