/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ColliderDebugSystem implementation
*/

#include "systems/ColliderDebugSystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "plugin_manager/CommonTypes.hpp"

namespace rtype::client {

ColliderDebugSystem::ColliderDebugSystem(engine::IGraphicsPlugin& graphics_plugin)
    : graphics_plugin_(graphics_plugin)
    , enabled_(false) {
}

void ColliderDebugSystem::init(Registry& registry) {
    (void)registry;
}

void ColliderDebugSystem::shutdown() {
}

void ColliderDebugSystem::update(Registry& registry, float dt) {
    (void)dt;

    if (!enabled_)
        return;

    // Check if components are registered
    if (!registry.has_component_registered<Position>() ||
        !registry.has_component_registered<Collider>())
        return;

    auto& positions = registry.get_components<Position>();
    auto& colliders = registry.get_components<Collider>();

    // Draw a green rectangle outline for each entity with Position + Collider
    for (size_t i = 0; i < positions.size(); ++i) {
        Entity entity = positions.get_entity_at(i);

        if (!colliders.has_entity(entity))
            continue;

        const Position& pos = positions[entity];
        const Collider& col = colliders[entity];

        // Create rectangle from center-based position using half-extents
        float half_w = col.width * 0.5f;
        float half_h = col.height * 0.5f;
        engine::Rectangle rect{pos.x - half_w, pos.y - half_h, col.width, col.height};

        // Draw green outline (RGB: 0, 255, 0)
        engine::Color green{0, 255, 0, 255};
        graphics_plugin_.draw_rectangle_outline(rect, green, 2.0f);
    }
}

}
