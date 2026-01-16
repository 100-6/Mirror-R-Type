#include "systems/MapBoundsSystem.hpp"

namespace bagario::systems {

MapBoundsSystem::MapBoundsSystem(float width, float height)
    : m_map_width(width)
    , m_map_height(height) {}

void MapBoundsSystem::init(Registry& registry) {
}

void MapBoundsSystem::update(Registry& registry, float dt) {
    auto& positions = registry.get_components<Position>();
    auto& colliders = registry.get_components<components::CircleCollider>();

    for (size_t i = 0; i < positions.size(); ++i) {
        Entity entity = positions.get_entity_at(i);
        auto& pos = positions.get_data_at(i);
        float radius = 0.0f;
        if (colliders.has_entity(entity))
            radius = colliders[entity].radius;
        pos.x = std::clamp(pos.x, radius, m_map_width - radius);
        pos.y = std::clamp(pos.y, radius, m_map_height - radius);
    }
}

void MapBoundsSystem::shutdown() {
}

void MapBoundsSystem::set_bounds(float width, float height) {
    m_map_width = width;
    m_map_height = height;
}

}
