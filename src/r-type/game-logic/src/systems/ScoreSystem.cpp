/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ScoreSystem
*/

#include "systems/ScoreSystem.hpp"
#include "components/GameComponents.hpp"
#include "ecs/Registry.hpp"
#include "ecs/events/InputEvents.hpp"
#include <iostream>

void ScoreSystem::init(Registry& registry)
{
    std::cout << "ScoreSystem: Initialisation" << std::endl;

    auto& eventBus = registry.get_event_bus();

    // S'abonner à l'événement EnemyKilledEvent
    enemyKilledSubId_ = eventBus.subscribe<ecs::EnemyKilledEvent>(
        [&registry](const ecs::EnemyKilledEvent& event) {
            auto& scores = registry.get_components<Score>();

            for (size_t i = 0; i < scores.size(); i++) {
                Entity entity = scores.get_entity_at(i);
                Score& score = scores[entity];
                int old_score = score.value;
                score.value += event.scoreValue;
                std::cout << "Enemy killed! Score: " << old_score << " -> " << score.value << std::endl;
            }
        }
    );
}

void ScoreSystem::shutdown()
{
    std::cout << "ScoreSystem: Arrêt" << std::endl;
}

void ScoreSystem::update(Registry& registry, float dt)
{
    (void)registry;
    (void)dt;
    // Le score est mis à jour via les événements, pas dans update
}
