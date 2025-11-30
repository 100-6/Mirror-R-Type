/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MovementSystem
*/

#include "ecs/systems/MovementSystem.hpp"
#include <iostream>

void MovementSystem::init(Registry& registry)
{
    std::cout << "MovementSystem: Initialisation." << std::endl;
}

void MovementSystem::shutdown()
{
    std::cout << "MovementSystem: Arrêt." << std::endl;
}

void MovementSystem::update(Registry& registry)
{
    auto& inputs = registry.get_components<Input>();
    auto& positions = registry.get_components<Position>();

    for (size_t i = 0; i < inputs.size(); ++i) {
        Entity entity = inputs.get_entity_at(i);

        if (!positions.has_entity(entity))
            continue;

        auto& input = inputs.get_data_at(i);
        auto& position = positions.get_data_by_entity_id(entity);

        if (input.up)
            position.y -= 1;
        else if (input.down)
            position.y += 1;

        if (input.left)
            position.x -= 1;
        else if (input.right)
            position.x += 1;

        input.up = input.down = input.left = input.right = false;
    }
}
