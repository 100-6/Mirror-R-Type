/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ShootingSystem - Gère la création de projectiles en réponse aux événements de tir
*/

#ifndef SHOOTINGSYSTEM_HPP_
#define SHOOTINGSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Components.hpp"
#include "ecs/Registry.hpp"
#include "core/event/EventBus.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"

class ShootingSystem : public ISystem {
    private:
        core::EventBus::SubscriptionId fireSubId_;
        engine::TextureHandle bulletTexture_;
        float bulletWidth_;
        float bulletHeight_;

    public:
        ShootingSystem(engine::TextureHandle bulletTexture, float bulletWidth, float bulletHeight);
        virtual ~ShootingSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;
};

#endif /* !SHOOTINGSYSTEM_HPP_ */
