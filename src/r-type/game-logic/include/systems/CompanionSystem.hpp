/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CompanionSystem - Manages companion turret spawn/destroy via ECS events
*/

#ifndef COMPANIONSYSTEM_HPP_
#define COMPANIONSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include <cstdint>

/**
 * @brief System that manages companion turrets (bonus weapons) using ECS events
 *
 * Subscribes to:
 * - CompanionSpawnEvent: Creates a companion turret attached to a player
 * - CompanionDestroyEvent: Destroys a player's companion turret
 *
 * The companion turret follows the player using the Attached component
 * and is rendered using standard Sprite/Position components.
 */
class CompanionSystem : public ISystem {
public:
    explicit CompanionSystem(engine::IGraphicsPlugin* graphics = nullptr);
    ~CompanionSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

    /**
     * @brief Set the texture handle for companion turret sprite
     */
    void set_companion_texture(engine::TextureHandle tex) { companionTexture_ = tex; }

private:
    engine::IGraphicsPlugin* graphics_;
    engine::TextureHandle companionTexture_ = engine::INVALID_HANDLE;

    size_t spawnSubId_ = 0;
    size_t destroySubId_ = 0;

    /**
     * @brief Create a companion turret entity attached to a player
     */
    void spawnCompanion(Registry& registry, Entity playerEntity);

    /**
     * @brief Destroy a player's companion turret
     */
    void destroyCompanion(Registry& registry, Entity playerEntity);
};

#endif /* !COMPANIONSYSTEM_HPP_ */
