/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CompanionSystem
*/

#include "systems/CompanionSystem.hpp"
#include "ecs/events/GameEvents.hpp"
#include <iostream>

CompanionSystem::CompanionSystem(engine::IGraphicsPlugin* graphics)
    : graphics_(graphics)
{
}

void CompanionSystem::init(Registry& registry)
{
    std::cout << "CompanionSystem: Initialisation" << std::endl;

    auto& eventBus = registry.get_event_bus();

    // Subscribe to companion spawn events
    spawnSubId_ = eventBus.subscribe<ecs::CompanionSpawnEvent>(
        [this, &registry](const ecs::CompanionSpawnEvent& event) {
            std::cout << "[CompanionSystem] Received CompanionSpawnEvent for player entity "
                      << event.player << " (playerId: " << event.playerId << ")" << std::endl;
            spawnCompanion(registry, event.player);
        }
    );

    // Subscribe to companion destroy events
    destroySubId_ = eventBus.subscribe<ecs::CompanionDestroyEvent>(
        [this, &registry](const ecs::CompanionDestroyEvent& event) {
            std::cout << "[CompanionSystem] Received CompanionDestroyEvent for player entity "
                      << event.player << std::endl;
            destroyCompanion(registry, event.player);
        }
    );
}

void CompanionSystem::update(Registry& registry, float dt)
{
    (void)registry;
    (void)dt;
    // Companion movement is handled by AttachmentSystem
    // Companion firing is handled by BonusWeaponSystem
}

void CompanionSystem::shutdown()
{
    std::cout << "CompanionSystem: Arrêt" << std::endl;
}

void CompanionSystem::spawnCompanion(Registry& registry, Entity playerEntity)
{
    auto& bonusWeapons = registry.get_components<BonusWeapon>();
    auto& positions = registry.get_components<Position>();

    // Check if player already has a companion
    if (bonusWeapons.has_entity(playerEntity)) {
        std::cout << "[CompanionSystem] Player already has companion, ignoring spawn" << std::endl;
        return;
    }

    if (!positions.has_entity(playerEntity)) {
        std::cerr << "[CompanionSystem] Player has no position, cannot spawn companion" << std::endl;
        return;
    }

    const Position& playerPos = positions[playerEntity];

    // Create companion entity
    Entity companionEntity = registry.spawn_entity();

    // Size: 15% of original image (442x257)
    constexpr float BONUS_SCALE_PERCENT = 15.0f;
    float bonusWidth = 442.0f * (BONUS_SCALE_PERCENT / 100.0f);
    float bonusHeight = 257.0f * (BONUS_SCALE_PERCENT / 100.0f);

    // Position offset relative to player (to the right and slightly above)
    float bonusOffsetX = 120.0f;
    float bonusOffsetY = -70.0f;

    // Add Position component
    registry.add_component(companionEntity, Position{
        playerPos.x + bonusOffsetX,
        playerPos.y + bonusOffsetY
    });

    // Add Attached component so it follows the player
    registry.add_component(companionEntity, Attached{
        playerEntity,
        bonusOffsetX,
        bonusOffsetY,
        4.0f  // Smooth follow factor
    });

    // Add Sprite component
    Sprite companionSprite{
        companionTexture_,
        bonusWidth,
        bonusHeight,
        0.0f,
        engine::Color::White,
        0.0f,
        0.0f,
        15  // Layer (above most entities)
    };
    companionSprite.source_rect = {0.0f, 0.0f, 442.0f, 257.0f};
    registry.add_component(companionEntity, companionSprite);

    // Mark player as having a bonus weapon (stores companion entity reference)
    registry.add_component(playerEntity, BonusWeapon{companionEntity, 0.0f, true});

    std::cout << "[CompanionSystem] Created companion entity " << companionEntity
              << " for player " << playerEntity
              << " at (" << (playerPos.x + bonusOffsetX) << ", " << (playerPos.y + bonusOffsetY) << ")"
              << std::endl;
}

void CompanionSystem::destroyCompanion(Registry& registry, Entity playerEntity)
{
    auto& bonusWeapons = registry.get_components<BonusWeapon>();

    if (!bonusWeapons.has_entity(playerEntity)) {
        std::cout << "[CompanionSystem] Player has no companion to destroy" << std::endl;
        return;
    }

    const BonusWeapon& bonusWeapon = bonusWeapons[playerEntity];

    if (bonusWeapon.weaponEntity != static_cast<size_t>(-1)) {
        Entity companionEntity = static_cast<Entity>(bonusWeapon.weaponEntity);

        // Remove rendering components first to stop display immediately
        auto& sprites = registry.get_components<Sprite>();
        auto& positions = registry.get_components<Position>();
        auto& attacheds = registry.get_components<Attached>();

        if (sprites.has_entity(companionEntity))
            registry.remove_component<Sprite>(companionEntity);
        if (positions.has_entity(companionEntity))
            registry.remove_component<Position>(companionEntity);
        if (attacheds.has_entity(companionEntity))
            registry.remove_component<Attached>(companionEntity);

        // Mark for destruction via ECS
        auto& toDestroys = registry.get_components<ToDestroy>();
        if (!toDestroys.has_entity(companionEntity)) {
            registry.add_component(companionEntity, ToDestroy{});
        }

        std::cout << "[CompanionSystem] Marked companion entity " << companionEntity
                  << " for destruction (player " << playerEntity << ")" << std::endl;
    }

    // Remove BonusWeapon component from player
    registry.remove_component<BonusWeapon>(playerEntity);
}
