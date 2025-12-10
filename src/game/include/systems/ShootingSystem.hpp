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

class ShootingSystem : public ISystem {
    private:
        core::EventBus::SubscriptionId fireSubId_;

        void createBasicProjectile(Registry& registry, Entity shooter, const Weapon& weapon, const Position& shooterPos, float shooterHeight);
        void createSpreadProjectiles(Registry& registry, Entity shooter, const Weapon& weapon, const Position& shooterPos, float shooterHeight);
        void createBurstProjectiles(Registry& registry, Entity shooter, Weapon& weapon, const Position& shooterPos, float shooterHeight);

    public:
        ShootingSystem() = default;
        virtual ~ShootingSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;
};

#endif /* !SHOOTINGSYSTEM_HPP_ */
