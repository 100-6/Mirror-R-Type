/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Render System - Renders entities using IGraphicsPlugin
*/

#pragma once

#include "temp/TempRegistry.hpp"
#include "../include/core/event/EventBus.hpp"
#include "../plugin_manager/include/IGraphicsPlugin.hpp"
#include <memory>
#include <algorithm>

namespace rtype {

/**
 * @brief System that renders entities using IGraphicsPlugin
 * No direct dependency on SFML/Raylib
 */
class RenderSystem {
public:
    /**
     * @brief Constructor
     * @param registry Entity registry
     * @param event_bus Event bus for communication
     * @param graphics_plugin Graphics plugin (optional for headless testing)
     */
    RenderSystem(TempRegistry& registry, core::EventBus& event_bus, IGraphicsPlugin* graphics_plugin = nullptr)
        : registry_(registry)
        , event_bus_(event_bus)
        , graphics_plugin_(graphics_plugin)
        , clear_color_(Color::Black) {}

    /**
     * @brief Update - Render all entities with SpriteComponent
     * @param delta_time Time since last frame (unused)
     */
    void update(float delta_time = 0.0f) {
        (void)delta_time; // Unused

        if (!graphics_plugin_) {
            return;
        }

        // Clear screen
        graphics_plugin_->clear(clear_color_);

        // Get all entities with Transform and Sprite
        auto entities = registry_.get_entities_with<TransformComponent, SpriteComponent>();
        
        // Sort by z-order for proper layering
        std::vector<std::pair<EntityId, int>> sorted_entities;
        for (EntityId entity : entities) {
            auto sprite_opt = registry_.get_component<SpriteComponent>(entity);
            if (sprite_opt.has_value()) {
                sorted_entities.push_back({entity, sprite_opt.value()->z_order});
            }
        }

        std::sort(sorted_entities.begin(), sorted_entities.end(),
                  [](const auto& a, const auto& b) { return a.second < b.second; });

        // Render each entity
        for (const auto& [entity, z_order] : sorted_entities) {
            auto transform_opt = registry_.get_component<TransformComponent>(entity);
            auto sprite_opt = registry_.get_component<SpriteComponent>(entity);

            if (!transform_opt.has_value() || !sprite_opt.has_value()) {
                continue;
            }

            auto* transform = transform_opt.value();
            auto* sprite = sprite_opt.value();

            // If texture is loaded, draw sprite
            if (sprite->texture != INVALID_HANDLE) {
                Sprite spr;
                spr.texture_handle = sprite->texture;
                spr.size = sprite->size;
                spr.rotation = transform->rotation;
                spr.tint = sprite->tint;
                
                graphics_plugin_->draw_sprite(spr, transform->position);
            }
            // Otherwise, draw a colored rectangle as fallback
            else {
                Rectangle rect;
                rect.x = transform->position.x;
                rect.y = transform->position.y;
                rect.width = sprite->size.x;
                rect.height = sprite->size.y;
                
                graphics_plugin_->draw_rectangle(rect, sprite->tint);
            }
        }

        // Display the frame
        graphics_plugin_->display();
    }

    /**
     * @brief Set the clear color
     * @param color Color to clear the screen with
     */
    void set_clear_color(Color color) {
        clear_color_ = color;
    }

    /**
     * @brief Set the graphics plugin
     * @param plugin Graphics plugin pointer
     */
    void set_graphics_plugin(IGraphicsPlugin* plugin) {
        graphics_plugin_ = plugin;
    }

private:
    TempRegistry& registry_;
    core::EventBus& event_bus_;
    IGraphicsPlugin* graphics_plugin_;
    Color clear_color_;
};

} // namespace rtype
