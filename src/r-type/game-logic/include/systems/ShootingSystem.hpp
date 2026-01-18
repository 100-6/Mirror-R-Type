/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ShootingSystem - Gère la création de projectiles en réponse aux événements de tir
*/

#ifndef SHOOTINGSYSTEM_HPP_
#define SHOOTINGSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "components/GameComponents.hpp"
#include "ecs/Registry.hpp"
#include "core/event/EventBus.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include <optional>

class ShootingSystem : public ISystem {
    private:
        engine::IGraphicsPlugin* graphics_;
        core::EventBus::SubscriptionId fireSubId_;

        void createProjectiles(Registry& registry, Entity shooter, Weapon& weapon, const Position& shooterPos, float shooterWidth, float shooterHeight);
        void updateLaserBeam(Registry& registry, Entity shooter, const Position& shooterPos, float shooterWidth, float shooterHeight, float dt);
        void performLaserRaycast(Registry& registry, float startX, float startY, float range, Entity shooter, LaserBeam& beam);

    public:
        ShootingSystem(engine::IGraphicsPlugin* graphics = nullptr) : graphics_(graphics) {}
        virtual ~ShootingSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;
};

#endif /* !SHOOTINGSYSTEM_HPP_ */
