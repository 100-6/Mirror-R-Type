/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** BonusSystem
*/

#include "systems/BonusSystem.hpp"
#include "components/GameComponents.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/events/GameEvents.hpp"
#include "AssetsPaths.hpp"
#include <iostream>
#include <cmath>

BonusSystem::BonusSystem(engine::IGraphicsPlugin* graphics, int screenWidth, int screenHeight)
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
    if (graphicsPlugin_) {
        bonusTex_ = graphicsPlugin_->load_texture(assets::paths::SHOT_ANIMATION);
        if (bonusTex_ == engine::INVALID_HANDLE) {
            std::cerr << "BonusSystem: Failed to load bonus texture!" << std::endl;
        }

        // Note: Bonus weapon texture is handled by CompanionSystem (ECS architecture)
    }

    // S'abonner à l'événement de spawn de bonus (quand un ennemi meurt)
    // Seulement côté client (où graphicsPlugin_ est disponible)
    // Le serveur gère le spawn réseau via GameSession
    if (graphicsPlugin_) {
        auto& eventBus = registry.get_event_bus();
        bonusSpawnSubId_ = eventBus.subscribe<ecs::BonusSpawnEvent>(
            [this, &registry](const ecs::BonusSpawnEvent& event) {
                BonusType type = static_cast<BonusType>(event.bonusType);
                spawnBonusAt(registry, type, event.x, event.y, BONUS_LIFETIME);
                std::cout << "[BonusSystem] Spawned dropped bonus at (" << event.x << ", " << event.y << ")" << std::endl;
            }
        );
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
        case BonusType::BONUS_WEAPON:
            tint = engine::Color::Yellow;
            typeName = "Arme Bonus";
            break;
    }

    // Taille fixe pour les bonus (cercle)
    engine::Vector2f texSize = {BONUS_RADIUS * 2, BONUS_RADIUS * 2};

    Entity bonus = registry.spawn_entity();
    registry.add_component(bonus, Position{x, y});
    registry.add_component(bonus, Bonus{type, BONUS_RADIUS});
    registry.add_component(bonus, Collider{BONUS_RADIUS * 2, BONUS_RADIUS * 2});
    registry.add_component(bonus, Scrollable{1.0f, false, true}); // Scroll et détruit hors écran
    Sprite sprite{
        bonusTex_,
        texSize.x,
        texSize.y,
        0.0f,
        tint,
        0.0f,
        0.0f,
        0  // Layer
    };
    sprite.source_rect = {0.0f, 0.0f, 16.0f, 16.0f};
    sprite.origin_x = texSize.x / 2.0f;
    sprite.origin_y = texSize.y / 2.0f;
    registry.add_component(bonus, sprite);

    std::cout << "BonusSystem: Spawn bonus " << typeName << " à (" << x << ", " << y << ")" << std::endl;
}

void BonusSystem::spawnBonusAt(Registry& registry, BonusType type, float x, float y, float lifetime)
{
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
        case BonusType::BONUS_WEAPON:
            tint = engine::Color::Yellow;
            typeName = "Arme Bonus";
            break;
    }

    // Taille fixe pour les bonus (cercle)
    engine::Vector2f texSize = {BONUS_RADIUS * 2, BONUS_RADIUS * 2};

    Entity bonus = registry.spawn_entity();
    registry.add_component(bonus, Position{x, y});
    registry.add_component(bonus, Bonus{type, BONUS_RADIUS});
    registry.add_component(bonus, Collider{BONUS_RADIUS * 2, BONUS_RADIUS * 2});

    Sprite sprite{
        bonusTex_,
        texSize.x,
        texSize.y,
        0.0f,
        tint,
        0.0f,
        0.0f,
        5  // Layer au dessus des autres
    };
    sprite.source_rect = {0.0f, 0.0f, 16.0f, 16.0f};
    sprite.origin_x = texSize.x / 2.0f;
    sprite.origin_y = texSize.y / 2.0f;
    registry.add_component(bonus, sprite);

    // Ajouter un lifetime si spécifié (pour les bonus droppés par les ennemis)
    if (lifetime > 0.0f) {
        registry.add_component(bonus, BonusLifetime{lifetime});
    }

    std::cout << "[BonusSystem] Spawned bonus " << typeName << " at (" << x << ", " << y << ")";
    if (lifetime > 0.0f) {
        std::cout << " with lifetime " << lifetime << "s";
    }
    std::cout << std::endl;
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

                    case BonusType::BONUS_WEAPON:
                        {
                            std::cout << "[BonusSystem] BONUS_WEAPON collected by player " << playerEntity << std::endl;

                            auto& bonusWeapons = registry.get_components<BonusWeapon>();
                            if (!bonusWeapons.has_entity(playerEntity)) {
                                // Publish event to CompanionSystem (ECS architecture)
                                // CompanionSystem will handle the companion turret creation
                                registry.get_event_bus().publish(ecs::CompanionSpawnEvent{playerEntity, 0});
                                std::cout << "[BonusSystem] Published CompanionSpawnEvent for player " << playerEntity << std::endl;
                            } else {
                                std::cout << "[BonusSystem] Joueur a déjà l'arme bonus, bonus ignoré" << std::endl;
                            }
                        }
                        break;
                }

                // Publish event for network sync (server will send to clients)
                registry.get_event_bus().publish(ecs::BonusCollectedEvent{playerEntity, static_cast<int>(bonus.type)});
                std::cout << "[BonusSystem] Published BonusCollectedEvent for player " << playerEntity
                          << " type=" << static_cast<int>(bonus.type) << std::endl;

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

void BonusSystem::updateBonusLifetimes(Registry& registry, float dt)
{
    auto& bonusLifetimes = registry.get_components<BonusLifetime>();

    std::vector<Entity> toDestroy;

    for (size_t i = 0; i < bonusLifetimes.size(); i++) {
        Entity entity = bonusLifetimes.get_entity_at(i);
        BonusLifetime& lifetime = bonusLifetimes[entity];

        lifetime.timeRemaining -= dt;

        if (lifetime.timeRemaining <= 0.0f) {
            toDestroy.push_back(entity);
            std::cout << "[BonusSystem] Bonus " << entity << " expired" << std::endl;
        }
    }

    // Détruire les bonus expirés
    for (Entity e : toDestroy) {
        registry.add_component(e, ToDestroy{});
    }
}

void BonusSystem::update(Registry& registry, float dt)
{
    // Periodic bonus spawns disabled - bonuses only come from enemy drops now

    // Gérer la collecte des bonus
    handleBonusCollection(registry);

    // Mettre à jour les boosts de vitesse actifs
    updateSpeedBoosts(registry, dt);

    // Mettre à jour les lifetimes des bonus droppés
    updateBonusLifetimes(registry, dt);
}
