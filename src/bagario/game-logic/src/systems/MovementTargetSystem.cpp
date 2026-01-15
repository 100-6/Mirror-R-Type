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
    auto& ejected_masses = registry.get_components<components::EjectedMass>();

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

    // Handle ejected mass decay and friction
    std::vector<Entity> to_destroy;
    for (size_t i = 0; i < ejected_masses.size(); ++i) {
        Entity entity = ejected_masses.get_entity_at(i);
        auto& ejected = ejected_masses.get_data_at(i);

        // Update decay timer
        ejected.decay_timer -= dt;
        if (ejected.decay_timer <= 0.0f) {
            to_destroy.push_back(entity);
            continue;
        }

        // Apply friction to slow down ejected mass
        if (velocities.has_entity(entity)) {
            auto& vel = velocities[entity];
            float friction = 3.0f * dt;  // Friction coefficient
            float current_speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
            if (current_speed > friction) {
                float factor = (current_speed - friction * current_speed) / current_speed;
                vel.x *= factor;
                vel.y *= factor;
            } else {
                vel.x = 0.0f;
                vel.y = 0.0f;
            }
        }
    }

    // Mark entities for destruction
    for (Entity entity : to_destroy) {
        registry.add_component<ToDestroy>(entity, ToDestroy{});
    }
}

void MovementTargetSystem::shutdown() {
}

}
