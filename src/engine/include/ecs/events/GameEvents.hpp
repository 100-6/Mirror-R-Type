/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GameEvents
*/

#pragma once

#include "core/event/Event.hpp"
#include "ecs/CoreComponents.hpp" // For Entity
#include <string>
#include <vector>

namespace ecs {

/**
 * @brief Event fired when a player is hit by something
 */
struct PlayerHitEvent : public core::Event {
    Entity player;
    Entity attacker;

    PlayerHitEvent(Entity p, Entity a = -1) : player(p), attacker(a) {}
};

/**
 * @brief Event fired when a player collects a power-up
 */
struct PowerUpCollectedEvent : public core::Event {
    Entity player;
    Entity powerUp;

    PowerUpCollectedEvent(Entity p, Entity pu) : player(p), powerUp(pu) {}
};

/**
 * @brief Event fired when a projectile is successfully fired
 */
struct ShotFiredEvent : public core::Event {
    Entity shooter;
    Entity projectile;

    ShotFiredEvent(Entity s, Entity p) : shooter(s), projectile(p) {}
};

/**
 * @brief Event fired when an explosion should be spawned (typically when an enemy dies)
 */
struct ExplosionEvent : public core::Event {
    Entity source;
    float x;
    float y;
    float scale;

    ExplosionEvent(Entity src, float px, float py, float s = 1.0f)
        : source(src)
        , x(px)
        , y(py)
        , scale(s) {}
};

/**
 * @brief Event fired when a bonus should be spawned (typically when an enemy dies with bonusDrop)
 */
struct BonusSpawnEvent : public core::Event {
    float x;
    float y;
    int bonusType;  // Cast to BonusType enum

    BonusSpawnEvent(float px, float py, int type)
        : x(px)
        , y(py)
        , bonusType(type) {}
};

/**
 * @brief Event fired when a bonus is collected by a player (for network sync)
 */
struct BonusCollectedEvent : public core::Event {
    Entity player;
    int bonusType;  // Cast to BonusType enum

    BonusCollectedEvent(Entity p, int type)
        : player(p)
        , bonusType(type) {}
};

/**
 * @brief Event fired when a companion turret should be spawned for a player
 */
struct CompanionSpawnEvent : public core::Event {
    Entity player;
    uint32_t playerId;  // Network player ID for client identification

    CompanionSpawnEvent(Entity p, uint32_t id)
        : player(p)
        , playerId(id) {}
};

/**
 * @brief Event fired when a companion turret should be destroyed (player died)
 */
struct CompanionDestroyEvent : public core::Event {
    Entity player;

    explicit CompanionDestroyEvent(Entity p)
        : player(p) {}
};

/**
 * @brief Event fired when a muzzle flash effect should be spawned
 */
struct MuzzleFlashSpawnEvent : public core::Event {
    Entity shooter;       // Entity that fired (player, companion, or enemy)
    float projectileX;    // Position of the projectile
    float projectileY;
    bool isCompanion;     // True if shooter is a companion turret
    bool isEnemy;         // True if shooter is an enemy
    float shooterWidth;   // Width of the shooter for dynamic offset calculation

    MuzzleFlashSpawnEvent(Entity s, float px, float py, bool companion, bool enemy, float width)
        : shooter(s)
        , projectileX(px)
        , projectileY(py)
        , isCompanion(companion)
        , isEnemy(enemy)
        , shooterWidth(width) {}
};

/**
 * @brief Event fired when a muzzle flash should be destroyed (companion destroyed)
 */
struct MuzzleFlashDestroyEvent : public core::Event {
    Entity shooter;  // The shooter whose muzzle flash should be destroyed

    explicit MuzzleFlashDestroyEvent(Entity s)
        : shooter(s) {}
};

/**
 * @brief Event fired when a companion turret fires (for audio)
 */
struct CompanionShotEvent : public core::Event {
    Entity companion;
    float x;
    float y;

    CompanionShotEvent(Entity c, float px, float py)
        : companion(c)
        , x(px)
        , y(py) {}
};

// ========== AUDIO-RELATED EVENTS ==========

/**
 * @brief Event fired when the game scene/level changes (for music transitions)
 */
struct SceneChangeEvent : public core::Event {
    enum class SceneType {
        MENU,
        GAMEPLAY,
        BOSS_FIGHT,
        VICTORY,
        GAME_OVER
    };

    SceneType newScene;
    int levelId;

    SceneChangeEvent(SceneType scene, int level = 0)
        : newScene(scene)
        , levelId(level) {}
};

/**
 * @brief Event fired when an explosion sound should play (differentiated by type)
 */
struct ExplosionSoundEvent : public core::Event {
    enum class ExplosionType {
        ENEMY_BASIC,
        ENEMY_TANK,
        ENEMY_BOSS,
        PLAYER
    };

    ExplosionType type;
    float x;
    float y;
    float scale;

    ExplosionSoundEvent(ExplosionType t, float px, float py, float s = 1.0f)
        : type(t)
        , x(px)
        , y(py)
        , scale(s) {}
};

/**
 * @brief Event to request music change with fade transition
 */
struct MusicChangeRequestEvent : public core::Event {
    std::string musicId;
    float fadeOutDuration;
    float fadeInDuration;
    bool loop;

    MusicChangeRequestEvent(const std::string& id, float fadeOut = 1.0f,
                            float fadeIn = 1.0f, bool shouldLoop = true)
        : musicId(id)
        , fadeOutDuration(fadeOut)
        , fadeInDuration(fadeIn)
        , loop(shouldLoop) {}
};

/**
 * @brief Event to request ambiance change with crossfade
 */
struct AmbianceChangeRequestEvent : public core::Event {
    std::string ambianceId;
    float crossfadeDuration;

    AmbianceChangeRequestEvent(const std::string& id = "", float crossfade = 2.0f)
        : ambianceId(id)
        , crossfadeDuration(crossfade) {}
};

/**
 * @brief A single leaderboard entry for display
 */
struct LeaderboardEntryData {
    uint32_t player_id;
    std::string player_name;
    uint32_t score;
    uint8_t rank;
};

/**
 * @brief Event fired when leaderboard data is received from the server
 */
struct LeaderboardReceivedEvent : public core::Event {
    std::vector<LeaderboardEntryData> entries;

    explicit LeaderboardReceivedEvent(const std::vector<LeaderboardEntryData>& e)
        : entries(e) {}
};

}
