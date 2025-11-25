/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Physics System - Applies velocity to transform
*/

#pragma once

#include "temp/TempRegistry.hpp"
#include "../include/core/event/EventBus.hpp"

namespace rtype {

/**
 * @brief System that applies velocity to transform
 * Handles world boundaries
 */
class PhysicsSystem {
public:
    /**
     * @brief Constructor
     * @param registry Entity registry
     * @param event_bus Event bus for communication
     */
    PhysicsSystem(TempRegistry& registry, core::EventBus& event_bus)
        : registry_(registry)
        , event_bus_(event_bus)
        , world_min_x_(0.0f)
        , world_min_y_(0.0f)
        , world_max_x_(1920.0f)
        , world_max_y_(1080.0f) {}

    /**
     * @brief Update all entities with TransformComponent and VelocityComponent
     * @param delta_time Time since last frame in seconds
     */
    void update(float delta_time) {
        // Process all entities with both Transform and Velocity components
        auto entities = registry_.get_entities_with<TransformComponent, VelocityComponent>();
        
        for (EntityId entity : entities) {
            auto transform_opt = registry_.get_component<TransformComponent>(entity);
            auto velocity_opt = registry_.get_component<VelocityComponent>(entity);

            if (!transform_opt.has_value() || !velocity_opt.has_value()) {
                continue;
            }

            auto* transform = transform_opt.value();
            auto* velocity = velocity_opt.value();

            // Apply velocity to position
            transform->position.x += velocity->velocity.x * delta_time;
            transform->position.y += velocity->velocity.y * delta_time;

            // Clamp to world boundaries
            if (transform->position.x < world_min_x_) {
                transform->position.x = world_min_x_;
                velocity->velocity.x = 0.0f;
            }
            if (transform->position.x > world_max_x_) {
                transform->position.x = world_max_x_;
                velocity->velocity.x = 0.0f;
            }
            if (transform->position.y < world_min_y_) {
                transform->position.y = world_min_y_;
                velocity->velocity.y = 0.0f;
            }
            if (transform->position.y > world_max_y_) {
                transform->position.y = world_max_y_;
                velocity->velocity.y = 0.0f;
            }
        }
    }

    /**
     * @brief Set world boundaries
     * @param min_x Minimum X coordinate
     * @param min_y Minimum Y coordinate
     * @param max_x Maximum X coordinate
     * @param max_y Maximum Y coordinate
     */
    void set_world_bounds(float min_x, float min_y, float max_x, float max_y) {
        world_min_x_ = min_x;
        world_min_y_ = min_y;
        world_max_x_ = max_x;
        world_max_y_ = max_y;
    }

private:
    TempRegistry& registry_;
    core::EventBus& event_bus_;
    float world_min_x_;
    float world_min_y_;
    float world_max_x_;
    float world_max_y_;
};

} // namespace rtype
