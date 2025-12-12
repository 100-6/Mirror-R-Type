/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** BonusSystem - Gère le spawn et la collecte des bonus
*/

#ifndef BONUSSYSTEM_HPP_
#define BONUSSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include <random>

class BonusSystem : public ISystem {
public:
    BonusSystem(engine::IGraphicsPlugin& graphics, int screenWidth, int screenHeight);
    ~BonusSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

private:
    engine::IGraphicsPlugin& graphicsPlugin_;
    int screenWidth_;
    int screenHeight_;

    // Timers pour le spawn des bonus
    float healthSpawnTimer_ = 0.0f;
    float shieldSpawnTimer_ = 0.0f;
    float speedSpawnTimer_ = 0.0f;

    // Intervalles de spawn
    static constexpr float HEALTH_SPAWN_INTERVAL = 45.0f;
    static constexpr float SHIELD_SPAWN_INTERVAL = 30.0f;
    static constexpr float SPEED_SPAWN_INTERVAL = 60.0f;

    // Rayon des bonus
    static constexpr float BONUS_RADIUS = 20.0f;

    // Paramètres du bouclier
    static constexpr float SHIELD_RADIUS_OFFSET = 15.0f;

    // Paramètres du boost de vitesse
    static constexpr float SPEED_BOOST_DURATION = 20.0f;
    static constexpr float SPEED_BOOST_MULTIPLIER = 1.5f;

    // Paramètres de soin
    static constexpr int HEALTH_BONUS_AMOUNT = 20;

    // Texture pour les bonus
    engine::TextureHandle bonusTex_ = engine::INVALID_HANDLE;

    // Random generator
    std::mt19937 rng_;

    void spawnBonus(Registry& registry, BonusType type);
    void handleBonusCollection(Registry& registry);
    void updateSpeedBoosts(Registry& registry, float dt);
    bool checkCircleCollision(float x1, float y1, float r1, float x2, float y2, float w2, float h2);
};

#endif /* !BONUSSYSTEM_HPP_ */
