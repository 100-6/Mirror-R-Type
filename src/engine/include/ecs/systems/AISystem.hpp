/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AISystem
*/

#ifndef AISYSTEM_HPP_
#define AISYSTEM_HPP_

#include "ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include <vector>

struct Wave {
    int basicCount = 0;
    int fastCount = 0;
    int tankCount = 0;
    float spawnInterval = 2.0f; // Seconds between spawns in this wave
};

class AISystem : public ISystem {
    public:
        explicit AISystem(engine::IGraphicsPlugin& graphics);
        ~AISystem() override = default;

        void init(Registry& registry) override;
        void update(Registry& registry, float dt) override;
        void shutdown() override;

    private:
        engine::IGraphicsPlugin& graphics_;

        // Wave Management
        std::vector<Wave> waves_;
        size_t currentWaveIndex_ = 0;
        bool isWaveInProgress_ = false;
        
        float waveTimer_ = 0.0f;       // Timer for breaks between waves
        float spawnTimer_ = 0.0f;      // Timer for spawning enemies
        int enemiesSpawnedInWave_ = 0; // Count of enemies spawned in current wave
        
        float timeBetweenWaves_ = 3.0f;

        // Assets
        engine::TextureHandle basicEnemyTex_ = engine::INVALID_HANDLE;
        engine::TextureHandle fastEnemyTex_ = engine::INVALID_HANDLE;
        engine::TextureHandle tankEnemyTex_ = engine::INVALID_HANDLE;
        engine::TextureHandle bulletTex_ = engine::INVALID_HANDLE;

        // Helpers
        void startNextWave();
        void spawnEnemy(Registry& registry, EnemyType type);
        void updateEnemyBehavior(Registry& registry, float dt);
        Entity findNearestPlayer(Registry& registry, const Position& enemyPos);
};

#endif /* !AISYSTEM_HPP_ */
