/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** HealthSystem
*/

#ifndef HEALTHSYSTEM_HPP_
#define HEALTHSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "core/event/EventBus.hpp"
#include "components/GameComponents.hpp"

class HealthSystem : public ISystem {
public:
    HealthSystem() = default;
    ~HealthSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

private:
    core::EventBus::SubscriptionId damageSubId_;

    /**
     * @brief Spawn a bonus at a specific position
     */
    void spawnBonusAtPosition(Registry& registry, BonusType type, float x, float y);
};

#endif /* !HEALTHSYSTEM_HPP_ */
