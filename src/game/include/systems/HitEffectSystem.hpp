/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** HitEffectSystem
*/

#ifndef HITEFFECTSYSTEM_HPP_
#define HITEFFECTSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"

class HitEffectSystem : public ISystem {
public:
    HitEffectSystem() = default;
    ~HitEffectSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

private:
    size_t damageSubId_ = 0;
};

#endif /* !HITEFFECTSYSTEM_HPP_ */
