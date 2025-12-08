/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ScoreSystem
*/

#include "ecs/systems/ScoreSystem.hpp"
#include "ecs/Components.hpp"
#include "ecs/Registry.hpp"
#include <iostream>
#include <stdexcept>

ScoreSystem::ScoreSystem(engine::IInputPlugin* plugin)
    : input_plugin(plugin), k_was_pressed(false)
{
    if (!input_plugin)
        throw std::runtime_error("ScoreSystem: plugin cannot be null");
}

void ScoreSystem::init(Registry& registry)
{
    std::cout << "ScoreSystem: Initialisation" << std::endl;
}

void ScoreSystem::shutdown()
{
    std::cout << "ScoreSystem: Arrêt" << std::endl;
}

void ScoreSystem::update(Registry& registry, float dt)
{
    (void)dt;

    bool k_is_pressed = input_plugin->is_key_pressed(engine::Key::K);

    // Debug: afficher si K est détecté
    if (k_is_pressed) {
        std::cout << "K pressed! was_pressed=" << k_was_pressed << std::endl;
    }

    // Detect rising edge (key just pressed)
    if (k_is_pressed && !k_was_pressed) {
        auto& scores = registry.get_components<Score>();
        std::cout << "Adding score! Found " << scores.size() << " entities with Score" << std::endl;

        for (size_t i = 0; i < scores.size(); i++) {
            Entity entity = scores.get_entity_at(i);
            Score& score = scores[entity];
            int old_score = score.value;
            score.value += 100;
            std::cout << "Score: " << old_score << " -> " << score.value << std::endl;
        }
    }

    k_was_pressed = k_is_pressed;
}
