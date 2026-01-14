#include "systems/MovementTargetSystem.hpp"

namespace bagario::systems {

void MovementTargetSystem::init(Registry& registry) {
}

void MovementTargetSystem::update(Registry& registry, float dt) {
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& masses = registry.get_components<components::Mass>();
    auto& targets = registry.get_components<components::MovementTarget>();
    auto& split_vels = registry.get_components<components::SplitVelocity>();

    for (size_t i = 0; i < masses.size(); ++i) {
        Entity entity = masses.get_entity_at(i);
        if (!positions.has_entity(entity) || !velocities.has_entity(entity))
            continue;
        auto& pos = positions[entity];
        auto& vel = velocities[entity];
        const auto& mass = masses.get_data_at(i);
        float speed = config::mass_to_speed(mass.value);
        if (targets.has_entity(entity)) {
            const auto& target = targets[entity];
            float dx = target.target_x - pos.x;
            float dy = target.target_y - pos.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 1.0f) {
                vel.x = (dx / dist) * speed;
                vel.y = (dy / dist) * speed;
            } else {
                vel.x = 0.0f;
                vel.y = 0.0f;
            }
        }
        if (split_vels.has_entity(entity)) {
            auto& split_vel = split_vels[entity];
            vel.x += split_vel.vx;
            vel.y += split_vel.vy;
            float decay = split_vel.decay_rate * dt;
            float split_speed = std::sqrt(split_vel.vx * split_vel.vx + split_vel.vy * split_vel.vy);
            if (split_speed > decay) {
                float factor = (split_speed - decay) / split_speed;
                split_vel.vx *= factor;
                split_vel.vy *= factor;
            } else
                registry.remove_component<components::SplitVelocity>(entity);
        }
    }
}

void MovementTargetSystem::shutdown() {
}

}
