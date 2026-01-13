/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** IWaveListener - Interface for wave events
*/

#pragma once

#include <cstdint>
#include <string>

namespace rtype::server {

// Forward declarations
struct Wave;
struct BonusDropConfig;

/**
 * @brief Interface for receiving wave events
 *
 * Implement this interface to receive notifications when:
 * - A wave starts
 * - A wave is completed
 * - Enemies/walls/powerups need to spawn
 */
class IWaveListener {
public:
    virtual ~IWaveListener() = default;

    /**
     * @brief Called when a wave starts
     * @param wave The wave that started
     */
    virtual void on_wave_started(const Wave& wave) = 0;

    /**
     * @brief Called when a wave is completed
     * @param wave The completed wave
     */
    virtual void on_wave_completed(const Wave& wave) = 0;

    /**
     * @brief Called when an enemy should spawn
     * @param enemy_type Type of enemy ("basic", "fast", "tank", "boss")
     * @param x Spawn X position
     * @param y Spawn Y position
     * @param bonus_drop Optional bonus drop configuration
     */
    virtual void on_spawn_enemy(const std::string& enemy_type, float x, float y, const BonusDropConfig& bonus_drop) = 0;

    /**
     * @brief Called when a wall should spawn
     * @param x Spawn X position
     * @param y Spawn Y position
     */
    virtual void on_spawn_wall(float x, float y) = 0;

    /**
     * @brief Called when a powerup should spawn
     * @param bonus_type Type of bonus ("health", "shield", "speed")
     * @param x Spawn X position
     * @param y Spawn Y position
     */
    virtual void on_spawn_powerup(const std::string& bonus_type, float x, float y) = 0;
};

}
