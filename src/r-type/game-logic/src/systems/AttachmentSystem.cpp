/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AttachmentSystem
*/

#include "systems/AttachmentSystem.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/Registry.hpp"
#include <cmath>

void AttachmentSystem::init(Registry& registry)
{
    (void)registry;
}

void AttachmentSystem::update(Registry& registry, float dt)
{
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

            // Calculer la position cible
            float targetX = parentPos.x + attached.offsetX;
            float targetY = parentPos.y + attached.offsetY;

            // Si smoothFactor > 0, on interpole avec latence (effet de suivi)
            // Sinon, on positionne directement (comportement original)
            if (attached.smoothFactor > 0.0f) {
                // Interpolation exponentielle (lerp) pour un suivi fluide
                float lerpFactor = 1.0f - std::exp(-attached.smoothFactor * dt);
                pos.x += (targetX - pos.x) * lerpFactor;
                pos.y += (targetY - pos.y) * lerpFactor;
            } else {
                // Positionnement direct (pas de latence)
                pos.x = targetX;
                pos.y = targetY;
            }
        }
    }
}
