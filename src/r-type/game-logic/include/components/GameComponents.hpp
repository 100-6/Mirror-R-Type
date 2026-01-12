/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GameComponents
*/

#ifndef GAME_COMPONENTS_HPP_
#define GAME_COMPONENTS_HPP_

#include "ecs/CoreComponents.hpp"

// AI

enum class EnemyType {
    Basic,
    Fast,
    Tank,
    Boss
};

struct AI {
    EnemyType type = EnemyType::Basic;
    float detectionRange = 800.0f;
    float shootCooldown = 2.0f;
    float timeSinceLastShot = 0.0f;
    float moveSpeed = 100.0f;
};

// Scrolling

struct Scrollable {
    float speedMultiplier = 1.0f;  // Multiplier for scroll speed (1.0 = normal, 2.0 = twice as fast)
    bool wrap = false;              // If true, entity wraps around for infinite scrolling
    bool destroyOffscreen = false;  // If true, entity is destroyed when scrolling offscreen
};

// Combat

enum class WeaponType {
    BASIC,
    SPREAD,
    BURST,
    LASER,
    CHARGE
};

struct Weapon {
    WeaponType type = WeaponType::BASIC;
    float time_since_last_fire = 999.0f;
    int burst_count = 0;
    
    // Charge System
    bool trigger_held = false;
    bool is_charging = false;
    float current_charge_duration = 0.0f;
    size_t chargeEffectEntity = -1; // -1 = invalid

    Sprite projectile_sprite;
};

struct FireRate {
    float cooldown = 0.1f;
    float time_since_last_fire = 999.0f;
};

// Tags Spécifiques R-Type

struct Enemy {};

struct LocalPlayer {};

enum class ProjectileFaction {
    Player,
    Enemy
};

struct Projectile {
    float angle = 0.0f;
    float lifetime = 5.0f;
    float time_alive = 0.0f;
    ProjectileFaction faction = ProjectileFaction::Player;
};

struct ShotAnimation {
    float timer = 0.0f;
    float frameDuration = 0.5f;  // Switch frame every 0.5 seconds
    bool currentFrame = false;    // false = frame 1, true = frame 2
};

struct BulletAnimation {
    float timer = 0.0f;
    float frameDuration = 0.1f;  // Switch frame every 0.1 seconds
    int currentFrame = 0;         // 0, 1, or 2 (3 frames total)
};

struct ExplosionAnimation {
    float timer = 0.0f;
    float frameDuration = 0.05f;
    int currentFrame = 0;
    int totalFrames = 1;
    int framesPerRow = 1;
    int frameWidth = 32;
    int frameHeight = 32;
};

struct Wall {};
struct Background {};

struct HitFlash {
    float time_remaining = 0.0f;
    engine::Color original_color = engine::Color::White;
};

// Logique de jeu (Stats)

struct Health
{
    int max = 100;
    int current = 100;
};

struct Invulnerability {
    float time_remaining = 0.0f;
};

struct Damage
{
    int value = 10;
};

struct Score
{
    int value = 0;
};

// Bonus System

enum class BonusType {
    HEALTH,     // +20 HP (vert)
    SHIELD,     // Protection 1 hit (violet)
    SPEED       // +50% vitesse pendant 20s (bleu)
};

// Wave System

enum class SpawnPattern {
    SINGLE,      // Spawn single entity at position
    LINE,        // Spawn entities in horizontal line
    GRID,        // Spawn entities in grid pattern
    RANDOM,      // Spawn at random Y positions
    FORMATION    // Spawn in specific formation (V, diamond, etc.)
};

enum class EntitySpawnType {
    ENEMY,
    WALL,
    OBSTACLE,
    POWERUP
};

struct WaveSpawnData {
    EntitySpawnType entityType = EntitySpawnType::ENEMY;
    EnemyType enemyType = EnemyType::Basic;  // Used if entityType is ENEMY
    BonusType bonusType = BonusType::HEALTH; // Used if entityType is POWERUP
    float positionX = 0.0f;                   // Relative to scroll position
    float positionY = 0.0f;                   // Absolute Y position
    int count = 1;                            // Number of entities to spawn
    SpawnPattern pattern = SpawnPattern::SINGLE;
    float spacing = 0.0f;                     // Spacing between entities in pattern
};

struct WaveTrigger {
    float scrollDistance = 0.0f;              // Scroll distance to trigger wave
    float timeDelay = 0.0f;                   // Optional time delay after scroll trigger
    bool triggered = false;                   // Has this wave been triggered?
};

struct WaveController {
    std::string configFilePath;               // Path to JSON config file
    float totalScrollDistance = 0.0f;         // Total scrolling since start
    size_t currentWaveIndex = 0;              // Current wave being processed
    int currentWaveNumber = 0;                // Current wave number from JSON
    size_t totalWaveCount = 0;                // Total number of waves
    bool allWavesCompleted = false;           // All waves finished
};

struct Bonus {
    BonusType type = BonusType::HEALTH;
    float radius = 20.0f;
};

struct Shield {
    bool active = true;  // Se désactive après 1 hit
};

struct SpeedBoost {
    float timeRemaining = 20.0f;      // Durée restante
    float multiplier = 1.5f;          // +50% vitesse
    float originalSpeed = 0.0f;       // Vitesse originale pour restauration
};

// Game State System

enum class GameStateType {
    PLAYING,
    PAUSED,
    GAME_OVER,
    VICTORY
};

struct GameState {
    GameStateType currentState = GameStateType::PLAYING;
    float stateTimer = 0.0f;          // Timer for animations/transitions
    int finalScore = 0;               // Score at game end
    bool restartRequested = false;    // Player wants to restart
};

// Network

struct NetworkId {
    uint32_t server_entity_id = 0;  // Server-side entity ID for network sync
};

// Local player marker (client-side only) - used by HUD to find the right player
struct LocalPlayer {};

#endif /* !GAME_COMPONENTS_HPP_ */
