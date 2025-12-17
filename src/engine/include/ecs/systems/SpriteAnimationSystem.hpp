/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** SpriteAnimationSystem - Handles sprite frame animations
*/

#ifndef SPRITE_ANIMATION_SYSTEM_HPP
#define SPRITE_ANIMATION_SYSTEM_HPP

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"

/**
 * @brief System for updating sprite animations
 *
 * Updates the current frame of animated sprites based on elapsed time.
 * Changes the texture of the Sprite component when frame changes.
 */
class SpriteAnimationSystem : public ISystem {
public:
    SpriteAnimationSystem() = default;
    ~SpriteAnimationSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;
};

#endif // SPRITE_ANIMATION_SYSTEM_HPP
