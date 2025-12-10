/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** PlayerInputSystem - R-Type specific input interpretation
*/

#include "systems/PlayerInputSystem.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/events/InputEvents.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include <iostream>
#include <unordered_map>

// État interne du système pour tracker les touches pressées
static std::unordered_map<Entity, std::unordered_map<engine::Key, bool>> keyStates;

void PlayerInputSystem::init(Registry& registry)
{
    auto& eventBus = registry.get_event_bus();

    // S'abonner aux événements bruts d'input
    eventBus.subscribe<ecs::RawKeyPressedEvent>([](const ecs::RawKeyPressedEvent& event) {
        keyStates[event.entity][event.key] = true;
    });

    eventBus.subscribe<ecs::RawKeyReleasedEvent>([](const ecs::RawKeyReleasedEvent& event) {
        keyStates[event.entity][event.key] = false;
    });

    std::cout << "PlayerInputSystem: Initialisation" << std::endl;
}

void PlayerInputSystem::shutdown()
{
    keyStates.clear();
    std::cout << "PlayerInputSystem: Arrêt" << std::endl;
}

void PlayerInputSystem::update(Registry& registry, float dt)
{
    (void)dt;

    auto& eventBus = registry.get_event_bus();
    auto& inputs = registry.get_components<Input>();

    // Interpréter les états des touches pour le gameplay R-Type
    for (size_t i = 0; i < inputs.size(); i++) {
        Entity entity = inputs.get_entity_at(i);

        if (!inputs.has_entity(entity))
            continue;

        auto& entityKeys = keyStates[entity];

        float dirX = 0.0f;
        float dirY = 0.0f;

        if (entityKeys[engine::Key::W] || entityKeys[engine::Key::Up])
            dirY -= 1.0f;
        if (entityKeys[engine::Key::S] || entityKeys[engine::Key::Down])
            dirY += 1.0f;
        if (entityKeys[engine::Key::A] || entityKeys[engine::Key::Left])
            dirX -= 1.0f;
        if (entityKeys[engine::Key::D] || entityKeys[engine::Key::Right])
            dirX += 1.0f;

        eventBus.publish(ecs::PlayerMoveEvent{entity, dirX, dirY});

        if (entityKeys[engine::Key::Space])
            eventBus.publish(ecs::PlayerFireEvent{entity});

        if (entityKeys[engine::Key::LShift] || entityKeys[engine::Key::RShift])
            eventBus.publish(ecs::PlayerSpecialEvent{entity});
    }
}
