/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MuzzleFlashSystem - Manages muzzle flash effects via ECS events
*/

#ifndef MUZZLEFLASHSYSTEM_HPP_
#define MUZZLEFLASHSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include <cstdint>
#include <unordered_map>

/**
 * @brief System that manages muzzle flash effects using ECS events
 *
 * Subscribes to:
 * - MuzzleFlashSpawnEvent: Creates a muzzle flash attached to a shooter
 * - MuzzleFlashDestroyEvent: Destroys a shooter's muzzle flash
 *
 * The muzzle flash is rendered using standard Sprite/Position/Attached components
 * and uses ShotAnimation for frame switching.
 */
class MuzzleFlashSystem : public ISystem {
public:
    explicit MuzzleFlashSystem(engine::IGraphicsPlugin* graphics = nullptr);
    ~MuzzleFlashSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

    /**
     * @brief Set the texture handle for muzzle flash sprite
     */
    void set_muzzle_flash_texture(engine::TextureHandle tex) { muzzleFlashTexture_ = tex; }

private:
    engine::IGraphicsPlugin* graphics_;
    engine::TextureHandle muzzleFlashTexture_ = engine::INVALID_HANDLE;

    size_t spawnSubId_ = 0;
    size_t destroySubId_ = 0;

    // Track muzzle flash entities by shooter entity
    std::unordered_map<Entity, Entity> shooterToFlash_;

    /**
     * @brief Create a muzzle flash entity attached to a shooter
     */
    void spawnMuzzleFlash(Registry& registry, Entity shooter, bool isCompanion);

    /**
     * @brief Destroy a shooter's muzzle flash
     */
    void destroyMuzzleFlash(Registry& registry, Entity shooter);

    /**
     * @brief Check if a shooter already has an active muzzle flash
     */
    bool hasActiveMuzzleFlash(Entity shooter) const;
};

#endif /* !MUZZLEFLASHSYSTEM_HPP_ */
