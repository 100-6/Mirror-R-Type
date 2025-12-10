/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** InputSystem
*/

#include "ecs/systems/InputSystem.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/events/InputEvents.hpp"
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

    for (size_t i = 0; i < inputs.size(); i++) {
        Entity entity = inputs.get_entity_at(i);

        if (!inputs.has_entity(entity))
            continue;

        auto& input = inputs[entity];

        // Capturer l'état des touches
        input.up = input_plugin.is_key_pressed(engine::Key::W) ||
                   input_plugin.is_key_pressed(engine::Key::Up);
        input.down = input_plugin.is_key_pressed(engine::Key::S) ||
                     input_plugin.is_key_pressed(engine::Key::Down);
        input.left = input_plugin.is_key_pressed(engine::Key::A) ||
                     input_plugin.is_key_pressed(engine::Key::Left);
        input.right = input_plugin.is_key_pressed(engine::Key::D) ||
                      input_plugin.is_key_pressed(engine::Key::Right);
        input.fire = input_plugin.is_key_pressed(engine::Key::Space);
        input.special = input_plugin.is_key_just_pressed(engine::Key::LShift) ||
                        input_plugin.is_key_just_pressed(engine::Key::RShift);

        // Publier les événements pour les autres systèmes
        float dirX = 0.0f;
        float dirY = 0.0f;

        if (input.up) dirY -= 1.0f;
        if (input.down) dirY += 1.0f;
        if (input.left) dirX -= 1.0f;
        if (input.right) dirX += 1.0f;

        eventBus.publish(ecs::PlayerMoveEvent{entity, dirX, dirY});

        if (input.fire)
            eventBus.publish(ecs::PlayerFireEvent{entity});

        if (input.special)
            eventBus.publish(ecs::PlayerSpecialEvent{entity});
    }

    input_plugin.update();
}
