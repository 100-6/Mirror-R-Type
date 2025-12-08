/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** InputSystem
*/

#include "ecs/systems/InputSystem.hpp"
#include "ecs/Components.hpp"
#include <iostream>

InputSystem::InputSystem(engine::IInputPlugin& plugin)
    : input_plugin(plugin)
{
    // Pas besoin de vérifier null - les références ne peuvent pas être nulles
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

    // Mettre à jour le plugin (lit les événements du frame actuel)
    input_plugin.update();

    // Récupérer tous les composants Input
    auto& inputs = registry.get_components<Input>();

    // Parcourir toutes les entités avec un composant Input
    for (size_t i = 0; i < inputs.size(); i++) {
        Entity entity = inputs.get_entity_at(i);

        if (!inputs.has_entity(entity)) {
            continue;
        }

        Input& input = inputs.get_data_by_entity_id(entity);

        // Lire les touches via le plugin
        input.up = input_plugin.is_key_pressed(engine::Key::W) ||
                   input_plugin.is_key_pressed(engine::Key::Up);

        input.down = input_plugin.is_key_pressed(engine::Key::S) ||
                     input_plugin.is_key_pressed(engine::Key::Down);

        input.left = input_plugin.is_key_pressed(engine::Key::A) ||
                     input_plugin.is_key_pressed(engine::Key::Left);

        input.right = input_plugin.is_key_pressed(engine::Key::D) ||
                      input_plugin.is_key_pressed(engine::Key::Right);

        input.fire = input_plugin.is_key_pressed(engine::Key::Space);

        input.special = input_plugin.is_key_pressed(engine::Key::LShift) ||
                        input_plugin.is_key_pressed(engine::Key::RShift);
    }
}
