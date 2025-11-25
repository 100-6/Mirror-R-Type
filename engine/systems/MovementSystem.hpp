/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Movement System - Converts input to velocity
*/

#pragma once

#include "temp/TempRegistry.hpp"
#include "../include/core/event/EventBus.hpp"
#include <cmath>

namespace rtype {

/**
 * @brief System that converts input to velocity
 * Handles diagonal movement normalization
 */
class MovementSystem {
public:
    /**
     * @brief Constructor
     * @param registry Entity registry
     * @param event_bus Event bus for communication
     */
    MovementSystem(TempRegistry& registry, core::EventBus& event_bus)
        : registry_(registry)
        , event_bus_(event_bus) {}

    /**
     * @brief Update all entities with InputComponent and VelocityComponent
     * @param delta_time Time since last frame (unused for now)
     */
    void update(float delta_time = 0.0f) {
        (void)delta_time; // Unused for now

        // Process all entities with both Input and Velocity components
        auto entities = registry_.get_entities_with<InputComponent, VelocityComponent>();
        
        for (EntityId entity : entities) {
            auto input_opt = registry_.get_component<InputComponent>(entity);
            auto velocity_opt = registry_.get_component<VelocityComponent>(entity);

            if (!input_opt.has_value() || !velocity_opt.has_value()) {
                continue;
            }

            auto* input = input_opt.value();
            auto* velocity = velocity_opt.value();

            // Calculate desired velocity based on input
            float vx = 0.0f;
            float vy = 0.0f;

            if (input->move_right) vx += 1.0f;
            if (input->move_left) vx -= 1.0f;
            if (input->move_down) vy += 1.0f;
            if (input->move_up) vy -= 1.0f;

            // Normalize diagonal movement to prevent faster diagonal speed
            float magnitude = std::sqrt(vx * vx + vy * vy);
            if (magnitude > 0.0f) {
                vx = (vx / magnitude) * velocity->max_speed;
                vy = (vy / magnitude) * velocity->max_speed;
            }

            // Apply velocity
            velocity->velocity.x = vx;
            velocity->velocity.y = vy;
        }
    }

private:
    TempRegistry& registry_;
    core::EventBus& event_bus_;
};

} // namespace rtype
