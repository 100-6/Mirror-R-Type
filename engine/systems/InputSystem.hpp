/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Input System - Polls input and updates InputComponent
*/

#pragma once

#include "temp/TempRegistry.hpp"
#include "../include/core/event/EventBus.hpp"
#include "../plugin_manager/include/IInputPlugin.hpp"
#include <memory>

namespace rtype {

/**
 * @brief System that handles input polling
 * Updates InputComponent based on IInputPlugin state
 */
class InputSystem {
public:
    /**
     * @brief Constructor
     * @param registry Entity registry
     * @param event_bus Event bus for communication
     * @param input_plugin Input plugin (optional, can be nullptr for headless testing)
     */
    InputSystem(TempRegistry& registry, core::EventBus& event_bus, IInputPlugin* input_plugin = nullptr)
        : registry_(registry)
        , event_bus_(event_bus)
        , input_plugin_(input_plugin) {}

    /**
     * @brief Update all entities with InputComponent
     * @param delta_time Time since last frame (unused for now)
     */
    void update(float delta_time = 0.0f) {
        (void)delta_time; // Unused for now

        // If no input plugin, skip
        if (!input_plugin_) {
            return;
        }

        // Update input plugin state
        input_plugin_->update();

        // Poll input for all entities with InputComponent
        auto entities = registry_.get_entities_with<InputComponent>();
        
        for (EntityId entity : entities) {
            auto input_opt = registry_.get_component<InputComponent>(entity);
            if (!input_opt.has_value()) {
                continue;
            }

            auto* input = input_opt.value();

            // Poll movement keys (WASD + Arrow keys for cross-platform support)
            input->move_up = input_plugin_->is_key_pressed(Key::Z) || 
                            input_plugin_->is_key_pressed(Key::Up);
            
            input->move_down = input_plugin_->is_key_pressed(Key::S) || 
                              input_plugin_->is_key_pressed(Key::Down);
            
            input->move_left = input_plugin_->is_key_pressed(Key::Q) || 
                              input_plugin_->is_key_pressed(Key::Left);
            
            input->move_right = input_plugin_->is_key_pressed(Key::D) || 
                               input_plugin_->is_key_pressed(Key::Right);

            // Poll shoot key (Space)
            input->shoot = input_plugin_->is_key_pressed(Key::Space);
        }
    }

    /**
     * @brief Set the input plugin
     * @param plugin Input plugin pointer
     */
    void set_input_plugin(IInputPlugin* plugin) {
        input_plugin_ = plugin;
    }

private:
    TempRegistry& registry_;
    core::EventBus& event_bus_;
    IInputPlugin* input_plugin_;
};

} // namespace rtype
