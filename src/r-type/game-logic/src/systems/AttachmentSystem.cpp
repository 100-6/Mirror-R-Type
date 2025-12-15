/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AttachmentSystem
*/

#include "systems/AttachmentSystem.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/Registry.hpp"

void AttachmentSystem::init(Registry& registry)
{
    (void)registry;
}

void AttachmentSystem::update(Registry& registry, float dt)
{
    (void)dt;
    auto& positions = registry.get_components<Position>();
    auto& attacheds = registry.get_components<Attached>();

    for (size_t i = 0; i < attacheds.size(); i++) {
        Entity entity = attacheds.get_entity_at(i);

        if (!attacheds.has_entity(entity) || !positions.has_entity(entity))
            continue;

        auto& attached = attacheds[entity];
        
        // Check if parent exists
        if (positions.has_entity(attached.parentEntity)) {
            const auto& parentPos = positions[attached.parentEntity];
            auto& pos = positions[entity];
            
            pos.x = parentPos.x + attached.offsetX;
            pos.y = parentPos.y + attached.offsetY;
        }
    }
}
