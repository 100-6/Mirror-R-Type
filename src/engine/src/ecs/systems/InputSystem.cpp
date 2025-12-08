/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** InputSystem
*/

#include "ecs/systems/InputSystem.hpp"
#include "ecs/Components.hpp"
#include "ecs/events/InputEvents.hpp"
#include <iostream>

InputSystem::InputSystem(engine::IInputPlugin* plugin)
    : input_plugin(plugin)
{
    if (!input_plugin)
        throw std::runtime_error("InputSystem: plugin cannot be null");
}

void InputSystem::init(Registry& registry)
{
    std::cout << "InputSystem: Initialisation avec " << input_plugin->get_name() << std::endl;
}

void InputSystem::shutdown()
{
    std::cout << "InputSystem: Arrêt" << std::endl;
}

void InputSystem::update(Registry& registry, float dt)
{
    (void)dt;

    auto& eventBus = registry.get_event_bus();
    auto& controllables = registry.get_components<Controllable>();

    for (size_t i = 0; i < controllables.size(); i++) {
        Entity entity = controllables.get_entity_at(i);

        if (!controllables.has_entity(entity))
            continue;

        float dirX = 0.0f;
        float dirY = 0.0f;

        bool up = input_plugin->is_key_pressed(engine::Key::W) ||
                  input_plugin->is_key_pressed(engine::Key::Up);
        bool down = input_plugin->is_key_pressed(engine::Key::S) ||
                    input_plugin->is_key_pressed(engine::Key::Down);
        bool left = input_plugin->is_key_pressed(engine::Key::A) ||
                    input_plugin->is_key_pressed(engine::Key::Left);
        bool right = input_plugin->is_key_pressed(engine::Key::D) ||
                     input_plugin->is_key_pressed(engine::Key::Right);

        if (up) dirY -= 1.0f;
        if (down) dirY += 1.0f;
        if (left) dirX -= 1.0f;
        if (right) dirX += 1.0f;

        eventBus.publish(ecs::PlayerMoveEvent{entity, dirX, dirY});

        if (input_plugin->is_key_pressed(engine::Key::Space))
            eventBus.publish(ecs::PlayerFireEvent{entity});

        if (input_plugin->is_key_just_pressed(engine::Key::LShift) || 
            input_plugin->is_key_just_pressed(engine::Key::RShift))
            eventBus.publish(ecs::PlayerSpecialEvent{entity});
    }

    input_plugin->update();
}
