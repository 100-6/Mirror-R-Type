/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AISystem - Handles enemy AI behavior (movement, shooting)
** Enemy spawning is handled by WaveSpawnerSystem
*/

#ifndef AISYSTEM_HPP_
#define AISYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"

class AISystem : public ISystem {
    public:
        explicit AISystem(engine::IGraphicsPlugin& graphics);
        ~AISystem() override = default;

        void init(Registry& registry) override;
        void update(Registry& registry, float dt) override;
        void shutdown() override;

    private:
        engine::IGraphicsPlugin& graphics_;

        // Assets (only bullet texture for AI shooting)
        engine::TextureHandle bulletTex_ = engine::INVALID_HANDLE;

        // Helpers
        void updateEnemyBehavior(Registry& registry, float dt);
        Entity findNearestPlayer(Registry& registry, const Position& enemyPos);
};

#endif /* !AISYSTEM_HPP_ */
