/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LaserRenderSystem - Renders laser beams using draw_line
*/

#ifndef LASERRENDERSYSTEM_HPP_
#define LASERRENDERSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "components/GameComponents.hpp"
#include "ecs/Registry.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"

class LaserRenderSystem : public ISystem {
    private:
        engine::IGraphicsPlugin* graphics_;
        float animation_time_ = 0.0f;  // Timer for animation effects

    public:
        LaserRenderSystem(engine::IGraphicsPlugin* graphics = nullptr) : graphics_(graphics) {}
        virtual ~LaserRenderSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;
};

#endif /* !LASERRENDERSYSTEM_HPP_ */
