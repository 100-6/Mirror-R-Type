/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MuzzleFlashSystem
*/

#include "systems/MuzzleFlashSystem.hpp"
#include "ecs/events/GameEvents.hpp"
#include <iostream>

MuzzleFlashSystem::MuzzleFlashSystem(engine::IGraphicsPlugin* graphics)
    : graphics_(graphics)
{
}

void MuzzleFlashSystem::init(Registry& registry)
{
    std::cout << "MuzzleFlashSystem: Initialisation" << std::endl;

    auto& eventBus = registry.get_event_bus();

    // Subscribe to muzzle flash spawn events
    spawnSubId_ = eventBus.subscribe<ecs::MuzzleFlashSpawnEvent>(
        [this, &registry](const ecs::MuzzleFlashSpawnEvent& event) {
            if (!hasActiveMuzzleFlash(event.shooter)) {
                spawnMuzzleFlash(registry, event.shooter, event.isCompanion);
            }
        }
    );

    // Subscribe to muzzle flash destroy events
    destroySubId_ = eventBus.subscribe<ecs::MuzzleFlashDestroyEvent>(
        [this, &registry](const ecs::MuzzleFlashDestroyEvent& event) {
            destroyMuzzleFlash(registry, event.shooter);
        }
    );

    // Also subscribe to CompanionDestroyEvent to clean up companion muzzle flashes
    eventBus.subscribe<ecs::CompanionDestroyEvent>(
        [this, &registry](const ecs::CompanionDestroyEvent& event) {
            std::cout << "[MuzzleFlashSystem] Received CompanionDestroyEvent for player " << event.player << std::endl;

            // First try to find the companion entity from BonusWeapon (if not yet removed)
            auto& bonusWeapons = registry.get_components<BonusWeapon>();
            if (bonusWeapons.has_entity(event.player)) {
                const BonusWeapon& bw = bonusWeapons[event.player];
                if (bw.weaponEntity != static_cast<size_t>(-1)) {
                    Entity companionEntity = static_cast<Entity>(bw.weaponEntity);
                    std::cout << "[MuzzleFlashSystem] Found companion " << companionEntity << " via BonusWeapon" << std::endl;
                    destroyMuzzleFlash(registry, companionEntity);
                    return;
                }
            }

            // Fallback: Check the muzzle flash's Attached component to find ones attached to companion
            // The muzzle flash is attached to the companion, and companion was attached to player
            auto& attacheds = registry.get_components<Attached>();
            std::vector<Entity> toDestroy;

            for (const auto& [shooter, flash] : shooterToFlash_) {
                // Check if the muzzle flash entity is attached to a shooter that was attached to this player
                // Since companion might already be destroyed, check if the flash's parent (shooter) is attached to player
                if (attacheds.has_entity(shooter)) {
                    if (attacheds[shooter].parentEntity == event.player) {
                        std::cout << "[MuzzleFlashSystem] Found muzzle flash via shooter attachment to player" << std::endl;
                        toDestroy.push_back(shooter);
                    }
                }
                // Also check if the muzzle flash itself is orphaned (its parent shooter no longer exists)
                else if (attacheds.has_entity(flash)) {
                    // Flash's parent is the shooter - if shooter doesn't have Attached, it might be destroyed
                    std::cout << "[MuzzleFlashSystem] Found orphaned muzzle flash, marking for destruction" << std::endl;
                    toDestroy.push_back(shooter);
                }
            }

            for (Entity shooter : toDestroy) {
                destroyMuzzleFlash(registry, shooter);
            }
        }
    );
}

void MuzzleFlashSystem::update(Registry& registry, float dt)
{
    auto& shotAnimations = registry.get_components<ShotAnimation>();
    auto& sprites = registry.get_components<Sprite>();
    auto& positions = registry.get_components<Position>();

    // Clean up destroyed flash entities and orphaned flashes (shooter no longer exists)
    std::vector<Entity> toRemove;
    std::vector<Entity> toDestroy;
    for (const auto& [shooter, flash] : shooterToFlash_) {
        if (!shotAnimations.has_entity(flash)) {
            toRemove.push_back(shooter);
        }
        // Check if shooter (parent) still exists - if not, destroy the muzzle flash
        else if (!positions.has_entity(shooter)) {
            toDestroy.push_back(shooter);
        }
    }
    for (Entity shooter : toRemove) {
        shooterToFlash_.erase(shooter);
    }
    // Destroy orphaned muzzle flashes
    for (Entity shooter : toDestroy) {
        std::cout << "[MuzzleFlashSystem] Cleaning up orphaned muzzle flash (shooter " << shooter << " no longer exists)" << std::endl;
        destroyMuzzleFlash(registry, shooter);
    }

    // Update all shot animations
    for (size_t i = 0; i < shotAnimations.size(); i++) {
        Entity entity = shotAnimations.get_entity_at(i);

        if (!shotAnimations.has_entity(entity))
            continue;

        auto& shotAnim = shotAnimations[entity];
        shotAnim.timer += dt;
        shotAnim.lifetime += dt;

        // Switch frame when timer exceeds frameDuration (continuous animation)
        if (shotAnim.timer >= shotAnim.frameDuration) {
            shotAnim.timer -= shotAnim.frameDuration;
            shotAnim.currentFrame = !shotAnim.currentFrame;

            // Update sprite source_rect
            if (sprites.has_entity(entity)) {
                auto& sprite = sprites[entity];
                // Frame 1 at x=0, Frame 2 at x=16 (each frame is 16x16 in a 32x16 image)
                sprite.source_rect.x = shotAnim.currentFrame ? 16.0f : 0.0f;
                sprite.source_rect.y = 0.0f;
                sprite.source_rect.width = 16.0f;
                sprite.source_rect.height = 16.0f;
            }
        }

        // Destroy the muzzle flash effect after 0.3 second (unless persistent)
        if (!shotAnim.persistent && shotAnim.lifetime >= 0.3f) {
            // Find and remove from tracking map
            for (auto it = shooterToFlash_.begin(); it != shooterToFlash_.end(); ++it) {
                if (it->second == entity) {
                    shooterToFlash_.erase(it);
                    break;
                }
            }
            registry.kill_entity(entity);
        }
    }
}

void MuzzleFlashSystem::shutdown()
{
    std::cout << "MuzzleFlashSystem: Arrêt" << std::endl;
    shooterToFlash_.clear();
}

void MuzzleFlashSystem::spawnMuzzleFlash(Registry& registry, Entity shooter, bool isCompanion)
{
    if (muzzleFlashTexture_ == engine::INVALID_HANDLE) {
        std::cerr << "[MuzzleFlashSystem] No texture set, cannot spawn muzzle flash" << std::endl;
        return;
    }

    Entity muzzleFlash = registry.spawn_entity();

    // Calculate offset in front of the shooter
    float flashOffsetX = isCompanion ? 85.0f : 80.0f;
    float flashOffsetY = isCompanion ? 15.0f : 0.0f;

    // Add Position component
    registry.add_component(muzzleFlash, Position{0.0f, 0.0f});

    // Add Attached component so it follows the shooter
    registry.add_component(muzzleFlash, Attached{
        shooter,
        flashOffsetX,
        flashOffsetY,
        0.0f  // No smooth follow for muzzle flash
    });

    // Add Sprite component
    float size = isCompanion ? 30.0f : 40.0f;
    Sprite flashSprite{
        muzzleFlashTexture_,
        size,
        size,
        0.0f,
        engine::Color::White,
        size / 2.0f,
        size / 2.0f,
        15  // Layer
    };
    flashSprite.source_rect = {0.0f, 0.0f, 16.0f, 16.0f};
    registry.add_component(muzzleFlash, flashSprite);

    // Add ShotAnimation component
    // For companion: persistent = true (stays active while companion exists)
    // For player: persistent = false (destroyed after 0.3s)
    registry.add_component(muzzleFlash, ShotAnimation{0.0f, 0.0f, 0.1f, false, isCompanion});

    // Track this muzzle flash
    shooterToFlash_[shooter] = muzzleFlash;

    std::cout << "[MuzzleFlashSystem] Created muzzle flash entity " << muzzleFlash
              << " for " << (isCompanion ? "companion" : "player") << " " << shooter
              << std::endl;
}

void MuzzleFlashSystem::destroyMuzzleFlash(Registry& registry, Entity shooter)
{
    auto it = shooterToFlash_.find(shooter);
    if (it == shooterToFlash_.end()) {
        return;
    }

    Entity flashEntity = it->second;

    // Remove rendering components first to stop display immediately
    auto& sprites = registry.get_components<Sprite>();
    auto& positions = registry.get_components<Position>();
    auto& attacheds = registry.get_components<Attached>();
    auto& shotAnims = registry.get_components<ShotAnimation>();

    if (sprites.has_entity(flashEntity))
        registry.remove_component<Sprite>(flashEntity);
    if (positions.has_entity(flashEntity))
        registry.remove_component<Position>(flashEntity);
    if (attacheds.has_entity(flashEntity))
        registry.remove_component<Attached>(flashEntity);
    if (shotAnims.has_entity(flashEntity))
        registry.remove_component<ShotAnimation>(flashEntity);

    // Mark for destruction via ECS
    auto& toDestroys = registry.get_components<ToDestroy>();
    if (!toDestroys.has_entity(flashEntity)) {
        registry.add_component(flashEntity, ToDestroy{});
    }

    shooterToFlash_.erase(it);

    std::cout << "[MuzzleFlashSystem] Destroyed muzzle flash entity " << flashEntity
              << " for shooter " << shooter << std::endl;
}

bool MuzzleFlashSystem::hasActiveMuzzleFlash(Entity shooter) const
{
    return shooterToFlash_.find(shooter) != shooterToFlash_.end();
}
