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

ScoreSystem::ScoreSystem(engine::IInputPlugin& plugin)
    : input_plugin(plugin)
{
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

    if (input_plugin.is_key_just_pressed(engine::Key::K)) {
        auto& scores = registry.get_components<Score>();

        for (size_t i = 0; i < scores.size(); i++) {
            Entity entity = scores.get_entity_at(i);
            Score& score = scores[entity];
            int old_score = score.value;
            score.value += 100;
            std::cout << "Score: " << old_score << " -> " << score.value << std::endl;
        }
    }
}
