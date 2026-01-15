/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** HitEffectSystem
*/

#include "systems/HitEffectSystem.hpp"
#include "components/GameComponents.hpp"
#include "ecs/events/InputEvents.hpp"
#include <iostream>

void HitEffectSystem::init(Registry& registry)
{
    std::cout << "HitEffectSystem: Initialisation" << std::endl;

    auto& eventBus = registry.get_event_bus();

    damageSubId_ = eventBus.subscribe<ecs::DamageEvent>(
        [&registry](const ecs::DamageEvent& event) {
            auto& sprites = registry.get_components<Sprite>();

            if (!sprites.has_entity(event.target))
                return;

            Sprite& sprite = sprites[event.target];
            auto& flashes = registry.get_components<HitFlash>();

            constexpr float FLASH_DURATION = 0.25f;  // Longer for smoother fade

            if (flashes.has_entity(event.target)) {
                // Reset flash timer
                flashes[event.target].time_remaining = FLASH_DURATION;
                flashes[event.target].total_duration = FLASH_DURATION;
            } else {
                // Create new flash with brightness boost
                engine::Color original_color = sprite.tint;

                registry.add_component(event.target, HitFlash{
                    FLASH_DURATION,
                    FLASH_DURATION,
                    original_color,
                    255  // Not used anymore, kept for compatibility
                });

                // Apply initial brightness boost (additive white effect)
                constexpr float MAX_BRIGHTNESS_ADD = 180.0f;
                auto clamp = [](float v) -> uint8_t {
                    return static_cast<uint8_t>(v > 255.0f ? 255 : v);
                };

                sprite.tint = engine::Color{
                    clamp(original_color.r + MAX_BRIGHTNESS_ADD),
                    clamp(original_color.g + MAX_BRIGHTNESS_ADD),
                    clamp(original_color.b + MAX_BRIGHTNESS_ADD),
                    original_color.a
                };
            }
        }
    );
}

void HitEffectSystem::shutdown()
{
    std::cout << "HitEffectSystem: Shutdown" << std::endl;
}

void HitEffectSystem::update(Registry& registry, float dt)
{
    auto& flashes = registry.get_components<HitFlash>();
    auto& sprites = registry.get_components<Sprite>();

    for (size_t i = 0; i < flashes.size(); ++i) {
        Entity entity = flashes.get_entity_at(i);

        if (flashes.has_entity(entity)) {
            HitFlash& flash = flashes[entity];
            flash.time_remaining -= dt;

            if (flash.time_remaining <= 0.0f) {
                // Flash complete - restore original color
                if (sprites.has_entity(entity))
                    sprites[entity].tint = flash.original_color;
                registry.remove_component<HitFlash>(entity);
            } else if (sprites.has_entity(entity)) {
                // Brighten the sprite with an additive white overlay effect
                // progress: 1.0 (start) = very bright, 0.0 (end) = original color
                float progress = flash.time_remaining / flash.total_duration;

                const engine::Color& original = flash.original_color;

                // Add white on top of the original color (additive blend simulation)
                constexpr float MAX_BRIGHTNESS_ADD = 180.0f;
                float brightness_add = MAX_BRIGHTNESS_ADD * progress;

                auto clamp = [](float v) -> uint8_t {
                    return static_cast<uint8_t>(v > 255.0f ? 255 : v);
                };

                uint8_t r = clamp(original.r + brightness_add);
                uint8_t g = clamp(original.g + brightness_add);
                uint8_t b = clamp(original.b + brightness_add);

                // Keep original alpha - don't change transparency!
                sprites[entity].tint = engine::Color{r, g, b, original.a};
            }
        }
    }
}
