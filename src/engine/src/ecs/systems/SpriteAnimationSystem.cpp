/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** SpriteAnimationSystem implementation
*/

#include "ecs/systems/SpriteAnimationSystem.hpp"
#include "ecs/CoreComponents.hpp"
#include <iostream>

void SpriteAnimationSystem::init(Registry& registry) {
    registry.register_component<SpriteAnimation>();
    std::cout << "SpriteAnimationSystem: Initialized" << std::endl;
}

void SpriteAnimationSystem::update(Registry& registry, float dt) {
    auto& animations = registry.get_components<SpriteAnimation>();
    auto& sprites = registry.get_components<Sprite>();

    for (size_t i = 0; i < animations.size(); ++i) {
        Entity entity = animations.get_entity_at(i);

        if (!animations.has_entity(entity) || !sprites.has_entity(entity))
            continue;

        SpriteAnimation& anim = animations[entity];
        Sprite& sprite = sprites[entity];

        // Skip if not playing or no frames
        if (!anim.playing || anim.frames.empty())
            continue;

        // Update elapsed time
        anim.elapsedTime += dt;

        // Check if it's time to change frame
        if (anim.elapsedTime >= anim.frameTime) {
            anim.elapsedTime -= anim.frameTime;

            // Advance to next frame
            anim.currentFrame++;

            // Handle loop or stop
            if (anim.currentFrame >= anim.frames.size()) {
                if (anim.loop) {
                    anim.currentFrame = 0;
                } else {
                    anim.currentFrame = anim.frames.size() - 1;
                    anim.playing = false;
                }
            }

            // Update sprite texture
            sprite.texture = anim.frames[anim.currentFrame];
        }
    }
}

void SpriteAnimationSystem::shutdown() {
    std::cout << "SpriteAnimationSystem: Shutdown" << std::endl;
}
