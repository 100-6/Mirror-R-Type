/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MovementSystem
*/

#ifndef MOVEMENTSYSTEM_HPP_
#define MOVEMENTSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Components.hpp"
#include "ecs/Registry.hpp"
#include "core/event/EventBus.hpp"

// class Registry;

class MovementSystem : public ISystem {
    private:
        core::EventBus::SubscriptionId moveSubId_;

    public:
        virtual ~MovementSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;
};

#endif /* !MOVEMENTSYSTEM_HPP_ */
