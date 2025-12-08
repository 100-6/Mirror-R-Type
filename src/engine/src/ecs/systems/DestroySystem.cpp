/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** DestroySystem
*/

#include "ecs/systems/DestroySystem.hpp"
#include <vector>

void DestroySystem::init(Registry& registry)
{
    std::cout << "DestroySystem: Initialisation." << std::endl;
}

void DestroySystem::shutdown()
{
    std::cout << "DestroySystem: Arrêt." << std::endl;
}

void DestroySystem::update(Registry& registry, float dt)
{
    (void)dt;

    auto& to_destroy = registry.get_components<ToDestroy>();
    std::vector<Entity> entities_to_kill;

    // Collecte toutes les entités marquées pour destruction
    for (size_t i = 0; i < to_destroy.size(); i++) {
        Entity entity = to_destroy.get_entity_at(i);
        entities_to_kill.push_back(entity);
    }

    // Détruit toutes les entités collectées
    for (Entity entity : entities_to_kill)
        registry.kill_entity(entity);
}
