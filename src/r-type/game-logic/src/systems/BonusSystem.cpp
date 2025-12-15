/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** BonusSystem
*/

#include "systems/BonusSystem.hpp"
#include "components/GameComponents.hpp"
#include "ecs/CoreComponents.hpp"
#include <iostream>
#include <cmath>

BonusSystem::BonusSystem(engine::IGraphicsPlugin& graphics, int screenWidth, int screenHeight)
    : graphicsPlugin_(graphics)
    , screenWidth_(screenWidth)
    , screenHeight_(screenHeight)
    , rng_(std::random_device{}())
{
}

void BonusSystem::init(Registry& registry)
{
    std::cout << "BonusSystem: Initialisation" << std::endl;
    std::cout << "  - Bonus HP (vert): toutes les " << HEALTH_SPAWN_INTERVAL << "s" << std::endl;
    std::cout << "  - Bonus Bouclier (violet): toutes les " << SHIELD_SPAWN_INTERVAL << "s" << std::endl;
    std::cout << "  - Bonus Vitesse (bleu): toutes les " << SPEED_SPAWN_INTERVAL << "s" << std::endl;

    // Charger la texture pour les bonus
    bonusTex_ = graphicsPlugin_.load_texture("assets/sprite/bullet.png");
    if (bonusTex_ == engine::INVALID_HANDLE) {
        std::cerr << "BonusSystem: Failed to load bonus texture!" << std::endl;
    }
}

void BonusSystem::shutdown()
{
    std::cout << "BonusSystem: Arrêt" << std::endl;
}

void BonusSystem::spawnBonus(Registry& registry, BonusType type)
{
    // Position aléatoire sur la partie droite de l'écran (pour laisser le temps de voir)
    constexpr float SPAWN_X_MIN_RATIO = 0.6f;
    constexpr float SPAWN_Y_MARGIN = 100.0f;

    std::uniform_real_distribution<float> distX(screenWidth_ * SPAWN_X_MIN_RATIO, screenWidth_ - BONUS_RADIUS * 2);
    std::uniform_real_distribution<float> distY(SPAWN_Y_MARGIN, screenHeight_ - SPAWN_Y_MARGIN);

    float x = distX(rng_);
    float y = distY(rng_);

    // Couleur selon le type de bonus
    engine::Color tint;
    std::string typeName;
    switch (type) {
        case BonusType::HEALTH:
            tint = engine::Color::Green;
            typeName = "HP";
            break;
        case BonusType::SHIELD:
            tint = engine::Color::Purple;
            typeName = "Bouclier";
            break;
        case BonusType::SPEED:
            tint = engine::Color::SpeedBlue;
            typeName = "Vitesse";
            break;
    }

    // Récupérer la taille du sprite
    engine::Vector2f texSize = graphicsPlugin_.get_texture_size(bonusTex_);

    Entity bonus = registry.spawn_entity();
    registry.add_component(bonus, Position{x, y});
    registry.add_component(bonus, Bonus{type, BONUS_RADIUS});
    registry.add_component(bonus, Collider{BONUS_RADIUS * 2, BONUS_RADIUS * 2});
    registry.add_component(bonus, Scrollable{1.0f, false, true}); // Scroll et détruit hors écran
    registry.add_component(bonus, Sprite{
        bonusTex_,
        texSize.x,
        texSize.y,
        0.0f,
        tint,
        0.0f,
        0.0f,
        0  // Layer
    });

    std::cout << "BonusSystem: Spawn bonus " << typeName << " à (" << x << ", " << y << ")" << std::endl;
}

bool BonusSystem::checkCircleCollision(float cx, float cy, float r, float rx, float ry, float rw, float rh)
{
    // Trouver le point le plus proche du cercle sur le rectangle
    float closestX = std::max(rx, std::min(cx, rx + rw));
    float closestY = std::max(ry, std::min(cy, ry + rh));

    // Calculer la distance entre le centre du cercle et ce point
    float dx = cx - closestX;
    float dy = cy - closestY;

    return (dx * dx + dy * dy) < (r * r);
}

void BonusSystem::handleBonusCollection(Registry& registry)
{
    auto& positions = registry.get_components<Position>();
    auto& bonuses = registry.get_components<Bonus>();
    auto& controllables = registry.get_components<Controllable>();
    auto& colliders = registry.get_components<Collider>();
    auto& healths = registry.get_components<Health>();

    // Pour chaque bonus
    for (size_t i = 0; i < bonuses.size(); i++) {
        Entity bonusEntity = bonuses.get_entity_at(i);

        if (!positions.has_entity(bonusEntity))
            continue;

        const Position& bonusPos = positions[bonusEntity];
        const Bonus& bonus = bonuses[bonusEntity];

        // Centre du bonus (cercle)
        float bonusCenterX = bonusPos.x + bonus.radius;
        float bonusCenterY = bonusPos.y + bonus.radius;

        // Pour chaque joueur
        for (size_t j = 0; j < controllables.size(); j++) {
            Entity playerEntity = controllables.get_entity_at(j);

            if (!positions.has_entity(playerEntity) || !colliders.has_entity(playerEntity))
                continue;

            const Position& playerPos = positions[playerEntity];
            const Collider& playerCol = colliders[playerEntity];

            // Vérifier collision cercle-rectangle
            if (checkCircleCollision(bonusCenterX, bonusCenterY, bonus.radius,
                                     playerPos.x, playerPos.y, playerCol.width, playerCol.height)) {

                // Appliquer l'effet du bonus
                switch (bonus.type) {
                    case BonusType::HEALTH:
                        if (healths.has_entity(playerEntity)) {
                            Health& health = healths[playerEntity];
                            health.current = std::min(health.current + HEALTH_BONUS_AMOUNT, health.max);
                            std::cout << "BonusSystem: Joueur récupère +" << HEALTH_BONUS_AMOUNT << " HP (HP: " << health.current << "/" << health.max << ")" << std::endl;
                        }
                        break;

                    case BonusType::SHIELD:
                        {
                            auto& shields = registry.get_components<Shield>();
                            if (!shields.has_entity(playerEntity)) {
                                registry.add_component(playerEntity, Shield{true});

                                // Ajouter l'effet visuel du bouclier (cercle violet)
                                float shieldRadius = std::max(playerCol.width, playerCol.height) / 2.0f + SHIELD_RADIUS_OFFSET;
                                registry.add_component(playerEntity, CircleEffect{
                                    shieldRadius,
                                    engine::Color::ShieldViolet,
                                    0.0f, 0.0f,
                                    true,
                                    CircleEffect::DEFAULT_LAYER
                                });

                                std::cout << "BonusSystem: Joueur obtient un bouclier!" << std::endl;
                            } else {
                                std::cout << "BonusSystem: Joueur a déjà un bouclier, bonus ignoré" << std::endl;
                            }
                        }
                        break;

                    case BonusType::SPEED:
                        {
                            auto& speedBoosts = registry.get_components<SpeedBoost>();
                            if (!speedBoosts.has_entity(playerEntity)) {
                                float originalSpeed = controllables[playerEntity].speed;
                                registry.add_component(playerEntity, SpeedBoost{SPEED_BOOST_DURATION, SPEED_BOOST_MULTIPLIER, originalSpeed});
                                controllables[playerEntity].speed = originalSpeed * SPEED_BOOST_MULTIPLIER;

                                // Ajouter l'indicateur de texte
                                registry.add_component(playerEntity, TextEffect{
                                    "SPEED BOOST: " + std::to_string(static_cast<int>(SPEED_BOOST_DURATION)) + "s",
                                    TextEffect::DEFAULT_POS_X,
                                    TextEffect::DEFAULT_POS_Y,
                                    engine::Color::SpeedBlue,
                                    TextEffect::DEFAULT_FONT_SIZE,
                                    true
                                });

                                std::cout << "BonusSystem: Joueur obtient +50% vitesse pendant " << SPEED_BOOST_DURATION << "s!" << std::endl;
                            } else {
                                // Reset le timer si déjà actif
                                speedBoosts[playerEntity].timeRemaining = SPEED_BOOST_DURATION;
                                std::cout << "BonusSystem: Boost vitesse prolongé!" << std::endl;
                            }
                        }
                        break;
                }

                // Marquer le bonus pour destruction
                registry.add_component(bonusEntity, ToDestroy{});
                break; // Un seul joueur peut ramasser le bonus
            }
        }
    }
}

void BonusSystem::updateSpeedBoosts(Registry& registry, float dt)
{
    auto& speedBoosts = registry.get_components<SpeedBoost>();
    auto& controllables = registry.get_components<Controllable>();
    auto& textEffects = registry.get_components<TextEffect>();

    std::vector<Entity> toRemove;

    for (size_t i = 0; i < speedBoosts.size(); i++) {
        Entity entity = speedBoosts.get_entity_at(i);
        SpeedBoost& boost = speedBoosts[entity];

        boost.timeRemaining -= dt;

        // Mettre à jour le texte de l'indicateur
        if (textEffects.has_entity(entity)) {
            textEffects[entity].text = "SPEED BOOST: " + std::to_string(static_cast<int>(boost.timeRemaining)) + "s";
        }

        if (boost.timeRemaining <= 0.0f) {
            // Restaurer la vitesse originale
            if (controllables.has_entity(entity)) {
                controllables[entity].speed = boost.originalSpeed;
                std::cout << "BonusSystem: Boost vitesse terminé pour entité " << entity << std::endl;
            }
            toRemove.push_back(entity);
        }
    }

    // Supprimer les composants expirés
    for (Entity e : toRemove) {
        registry.remove_component<SpeedBoost>(e);
        registry.remove_component<TextEffect>(e);
    }
}

void BonusSystem::update(Registry& registry, float dt)
{
    // Mettre à jour les timers de spawn
    healthSpawnTimer_ += dt;
    shieldSpawnTimer_ += dt;
    speedSpawnTimer_ += dt;

    // Spawn des bonus HP (toutes les 45s)
    if (healthSpawnTimer_ >= HEALTH_SPAWN_INTERVAL) {
        spawnBonus(registry, BonusType::HEALTH);
        healthSpawnTimer_ = 0.0f;
    }

    // Spawn des bonus bouclier (toutes les 30s)
    if (shieldSpawnTimer_ >= SHIELD_SPAWN_INTERVAL) {
        spawnBonus(registry, BonusType::SHIELD);
        shieldSpawnTimer_ = 0.0f;
    }

    // Spawn des bonus vitesse (toutes les 60s)
    if (speedSpawnTimer_ >= SPEED_SPAWN_INTERVAL) {
        spawnBonus(registry, BonusType::SPEED);
        speedSpawnTimer_ = 0.0f;
    }

    // Gérer la collecte des bonus
    handleBonusCollection(registry);

    // Mettre à jour les boosts de vitesse actifs
    updateSpeedBoosts(registry, dt);
}
