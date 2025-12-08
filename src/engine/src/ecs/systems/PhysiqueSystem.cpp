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
    auto& projectiles = registry.get_components<Projectile>();
    auto& toDestroy = registry.get_components<ToDestroy>();
    auto& sprites = registry.get_components<Sprite>();

    for (size_t i = 0; i < velocitys.size(); i++)
    {
        Entity entity = velocitys.get_entity_at(i);

        if (!(positions.has_entity(entity)))
            continue;

        auto& pos = positions[entity];
        auto& vel = velocitys[entity];

        pos.x += vel.x * dt;
        pos.y += vel.y * dt;

        // FRICTION - seulement pour les entités contrôlables (pas les projectiles)
        if (controllables.has_entity(entity)) {
            vel.x *= 0.98f;
            vel.y *= 0.98f;

            // Limite Gauche/Droite
            if (pos.x < 0) pos.x = 0;
            if (pos.x > SCREEN_WIDTH) pos.x = SCREEN_WIDTH;

            // Limite Haut/Bas
            if (pos.y < 0) pos.y = 0;
            if (pos.y > SCREEN_HEIGHT) pos.y = SCREEN_HEIGHT;
        }

        // Détruire les projectiles qui sortent de l'écran
        if (projectiles.has_entity(entity)) {
            if (pos.x < -100 || pos.x > SCREEN_WIDTH + 100 ||
                pos.y < -100 || pos.y > SCREEN_HEIGHT + 100) {
                registry.add_component(entity, ToDestroy{});
            }
        }

        // Scrolling background infini (layer < -50 = background)
        if (sprites.has_entity(entity)) {
            auto& sprite = sprites[entity];
            if (sprite.layer < -50) {
                // Si le fond est complètement sorti à gauche, le repositionner à droite
                if (pos.x <= -SCREEN_WIDTH) {
                    pos.x += SCREEN_WIDTH * 2.0f;
                }
            }
        }
    }

}