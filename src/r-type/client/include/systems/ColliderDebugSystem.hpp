/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ColliderDebugSystem - visualize colliders with green boxes
*/

#pragma once

#include "ecs/systems/ISystem.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"

class Registry;

namespace rtype::client {

/**
 * @brief System that renders debug visualization for colliders
 *
 * This system draws green rectangles around all entities with colliders
 * to help visualize collision boundaries during development.
 */
class ColliderDebugSystem : public ISystem {
public:
    explicit ColliderDebugSystem(engine::IGraphicsPlugin& graphics_plugin);
    ~ColliderDebugSystem() override = default;

    void init(Registry& registry) override;
    void shutdown() override;
    void update(Registry& registry, float dt) override;

    // Enable or disable debug rendering
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }

private:
    engine::IGraphicsPlugin& graphics_plugin_;
    bool enabled_;
};

}
