/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** PlayerInputSystem - R-Type specific input interpretation
*/

#ifndef PLAYERINPUTSYSTEM_HPP_
#define PLAYERINPUTSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"

/**
 * @brief R-Type specific system that interprets raw Input component states
 * into game actions (movement, firing, special abilities)
 *
 * This system reads the generic Input component (populated by InputSystem)
 * and publishes R-Type specific events like PlayerMoveEvent, PlayerFireEvent, etc.
 */
class PlayerInputSystem : public ISystem {
    public:
        PlayerInputSystem() = default;
        virtual ~PlayerInputSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;
};

#endif /* !PLAYERINPUTSYSTEM_HPP_ */
