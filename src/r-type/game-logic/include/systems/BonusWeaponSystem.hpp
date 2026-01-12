/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** BonusWeaponSystem - Gère le tir automatique de l'arme bonus
*/

#ifndef BONUSWEAPONSYSTEM_HPP_
#define BONUSWEAPONSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"

class BonusWeaponSystem : public ISystem {
public:
    BonusWeaponSystem(engine::IGraphicsPlugin* graphics = nullptr);
    ~BonusWeaponSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

private:
    engine::IGraphicsPlugin* graphics_;

    void fireBonusWeapon(Registry& registry, Entity bonusWeaponEntity, const Position& weaponPos);
};

#endif /* !BONUSWEAPONSYSTEM_HPP_ */
