/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** INetworkSystemListener - Interface for ServerNetworkSystem events
*/

#pragma once

#include <cstdint>
#include <vector>

namespace rtype::server {

/**
 * @brief Interface for receiving network system events (snapshots, spawns, destroys)
 *
 * This is used by ServerNetworkSystem to notify when:
 * - A state snapshot is ready
 * - An entity spawns
 * - An entity is destroyed
 * - A projectile spawns
 */
class INetworkSystemListener {
public:
    virtual ~INetworkSystemListener() = default;

    /**
     * @brief Called when a state snapshot is ready (20 Hz)
     */
    virtual void on_snapshot_ready(uint32_t session_id, const std::vector<uint8_t>& snapshot) = 0;

    /**
     * @brief Called when an entity spawns
     */
    virtual void on_entity_spawned(uint32_t session_id, const std::vector<uint8_t>& spawn_data) = 0;

    /**
     * @brief Called when an entity is destroyed
     */
    virtual void on_entity_destroyed(uint32_t session_id, uint32_t entity_id) = 0;

    /**
     * @brief Called when a projectile spawns
     */
    virtual void on_projectile_spawned(uint32_t session_id, const std::vector<uint8_t>& projectile_data) = 0;

    /**
     * @brief Called when an explosion event must be sent to clients
     */
    virtual void on_explosion_triggered(uint32_t session_id, const std::vector<uint8_t>& explosion_data) = 0;

    /**
     * @brief Called when score is updated
     */
    virtual void on_score_updated(uint32_t session_id, const std::vector<uint8_t>& score_data) = 0;

    /**
     * @brief Called when a powerup is collected
     */
    virtual void on_powerup_collected(uint32_t session_id, const std::vector<uint8_t>& powerup_data) = 0;
};

}
