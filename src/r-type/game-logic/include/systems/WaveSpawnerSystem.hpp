/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** WaveSpawnerSystem - Spawns entities based on scrolling progression and JSON config
*/

#ifndef WAVE_SPAWNER_SYSTEM_HPP_
#define WAVE_SPAWNER_SYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "components/WaveConfig.hpp"
#include "systems/WaveConfigLoader.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include <vector>
#include <memory>

class WaveSpawnerSystem : public ISystem {
public:
    explicit WaveSpawnerSystem(engine::IGraphicsPlugin& graphics);
    ~WaveSpawnerSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

    /**
     * @brief Load wave configuration from JSON file
     * @param filepath Path to the JSON configuration file
     * @return true if loaded successfully, false otherwise
     */
    bool loadWaveConfiguration(const std::string& filepath);

    /**
     * @brief Reset the wave system to start from beginning
     */
    void reset();

private:
    engine::IGraphicsPlugin& graphics_;

    // Wave configuration
    WaveLoader::WaveConfiguration config_;
    bool configLoaded_ = false;
    size_t currentWaveIndex_ = 0;
    float timeSinceWaveTrigger_ = 0.0f;
    bool waitingForTimeDelay_ = false;

    // Scrolling tracking
    float lastScrollPosition_ = 0.0f;
    float totalScrollDistance_ = 0.0f;

    // Asset handles
    engine::TextureHandle basicEnemyTex_ = engine::INVALID_HANDLE;
    engine::TextureHandle fastEnemyTex_ = engine::INVALID_HANDLE;
    engine::TextureHandle tankEnemyTex_ = engine::INVALID_HANDLE;
    engine::TextureHandle bossEnemyTex_ = engine::INVALID_HANDLE;
    engine::TextureHandle wallTex_ = engine::INVALID_HANDLE;
    engine::TextureHandle obstacleTex_ = engine::INVALID_HANDLE;
    engine::TextureHandle bulletTex_ = engine::INVALID_HANDLE;

    // Helper methods

    /**
     * @brief Update scroll distance tracking
     */
    void updateScrollTracking(Registry& registry, float dt);

    /**
     * @brief Check if any waves should be triggered
     */
    void checkWaveTriggers(Registry& registry, float dt);

    /**
     * @brief Spawn entities for a specific wave
     */
    void spawnWave(Registry& registry, const WaveLoader::Wave& wave);

    /**
     * @brief Spawn a single entity based on spawn data
     */
    void spawnEntity(Registry& registry, const WaveSpawnData& spawnData);

    /**
     * @brief Spawn an enemy entity
     */
    Entity spawnEnemy(Registry& registry, EnemyType type, float x, float y);

    /**
     * @brief Spawn a wall entity
     */
    Entity spawnWall(Registry& registry, float x, float y);

    /**
     * @brief Spawn an obstacle entity
     */
    Entity spawnObstacle(Registry& registry, float x, float y);

    /**
     * @brief Apply spawn pattern to create multiple entities
     */
    void applySpawnPattern(Registry& registry, const WaveSpawnData& spawnData);

    /**
     * @brief Get texture handle for enemy type
     */
    engine::TextureHandle getEnemyTexture(EnemyType type) const;

    /**
     * @brief Load all required textures
     */
    void loadTextures();
};

#endif /* !WAVE_SPAWNER_SYSTEM_HPP_ */
