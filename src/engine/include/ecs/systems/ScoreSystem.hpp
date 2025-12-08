/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ScoreSystem
*/

#ifndef SCORESYSTEM_HPP_
#define SCORESYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "core/event/EventBus.hpp"

class ScoreSystem : public ISystem {
public:
    ScoreSystem() = default;
    ~ScoreSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

private:
    core::EventBus::SubscriptionId enemyKilledSubId_;
};

#endif /* !SCORESYSTEM_HPP_ */
