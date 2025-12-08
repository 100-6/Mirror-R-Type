/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ScoreSystem
*/

#ifndef SCORESYSTEM_HPP_
#define SCORESYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "plugin_manager/IInputPlugin.hpp"

class ScoreSystem : public ISystem {
public:
    explicit ScoreSystem(engine::IInputPlugin& plugin);
    ~ScoreSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

private:
    engine::IInputPlugin& input_plugin;
};

#endif /* !SCORESYSTEM_HPP_ */
