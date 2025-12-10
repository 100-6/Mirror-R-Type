/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** InputSystem
*/

#include "ecs/systems/InputSystem.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/events/InputEvents.hpp"
#include "plugin_manager/KeyConfig.hpp"
#include <iostream>

InputSystem::InputSystem(engine::IInputPlugin& plugin)
    : input_plugin(plugin)
{
}

void InputSystem::init(Registry& registry)
{
    std::cout << "InputSystem: Initialisation avec " << input_plugin.get_name() << std::endl;
}

void InputSystem::shutdown()
{
    std::cout << "InputSystem: Arrêt" << std::endl;
}

void InputSystem::update(Registry& registry, float dt)
{
    (void)dt;

    auto& eventBus = registry.get_event_bus();
    auto& inputs = registry.get_components<Input>();

    static std::unordered_map<Entity, std::unordered_map<engine::Key, bool>> previousStates;

    for (size_t i = 0; i < inputs.size(); i++) {
        Entity entity = inputs.get_entity_at(i);

        if (!inputs.has_entity(entity))
            continue;

        auto& prevState = previousStates[entity];

        for (const auto& key : engine::KeyConfig::ALL_KEYS) {
            bool isPressed = input_plugin.is_key_pressed(key);
            bool wasPressed = prevState[key];

            if (isPressed && !wasPressed)
                eventBus.publish(ecs::RawKeyPressedEvent{entity, key});
            else if (!isPressed && wasPressed)
                eventBus.publish(ecs::RawKeyReleasedEvent{entity, key});

            prevState[key] = isPressed;
        }
    }

    input_plugin.update();
}
