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

    // S'abonner aux dégâts pour déclencher l'effet visuel
    damageSubId_ = eventBus.subscribe<ecs::DamageEvent>(
        [&registry](const ecs::DamageEvent& event) {
            auto& sprites = registry.get_components<Sprite>();
            
            // Si l'entité n'a pas de sprite, pas d'effet visuel
            if (!sprites.has_entity(event.target))
                return;

            Sprite& sprite = sprites[event.target];
            auto& flashes = registry.get_components<HitFlash>();

            if (flashes.has_entity(event.target)) {
                // Déjà en train de flasher, on reset le timer
                flashes[event.target].time_remaining = 0.1f;
            } else {
                // Commence à flasher : sauvegarde couleur et applique rouge
                registry.add_component(event.target, HitFlash{0.1f, sprite.tint});
                sprite.tint = engine::Color{255, 0, 0, 255}; // Rouge
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
                // Fin de l'effet
                if (sprites.has_entity(entity)) {
                    sprites[entity].tint = flash.original_color;
                }
                registry.remove_component<HitFlash>(entity);
            }
        }
    }
}
