/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** PhysiqueSystem
*/

#include "ecs/systems/PhysiqueSystem.hpp"
#include <iostream>

void PhysiqueSystem::init(Registry& registry)
{
    std::cout << "PhysiqueSystem: Initialisation." << std::endl;
}

void PhysiqueSystem::shutdown()
{
    std::cout << "PhysiqueSystem: Arrêt." << std::endl;
}

void PhysiqueSystem::update(Registry& registry, float dt)
{
    auto& positions = registry.get_components<Position>();
    auto& velocitys = registry.get_components<Velocity>();
    auto& controllables = registry.get_components<Controllable>();

    for (size_t i = 0; i < velocitys.size(); i++)
    {
        Entity entity = velocitys.get_entity_at(i);

        if (!(positions.has_entity(entity)))
            continue;

        auto& pos = positions[entity];
        auto& vel = velocitys[entity];

        pos.x += vel.x * dt;
        pos.y += vel.y * dt;

        //FRICTION (je suis pas sur sah)
        vel.x *= 0.98f;
        vel.y *= 0.98f;

        if (controllables.has_entity(entity)) {
            // Limite Gauche/Droite
            if (pos.x < 0) pos.x = 0;
            if (pos.x > SCREEN_WIDTH) pos.x = SCREEN_WIDTH;

            // Limite Haut/Bas
            if (pos.y < 0) pos.y = 0;
            if (pos.y > SCREEN_HEIGHT) pos.y = SCREEN_HEIGHT;
        }
    }

}