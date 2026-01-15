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

// Forward declarations for bonus system
enum class BonusType {
    HEALTH,       // +20 HP (vert)
    SHIELD,       // Protection 1 hit (violet)
    SPEED,        // +50% vitesse pendant 20s (bleu)
    BONUS_WEAPON  // Arme bonus qui tire automatiquement
};

struct BonusDrop {
    bool enabled = false;                     // Whether this enemy drops a bonus
    BonusType bonusType = BonusType::HEALTH; // Type of bonus to drop
    float dropChance = 1.0f;                  // Probability of drop (0.0 to 1.0)
};

// Tags Spécifiques R-Type

struct Enemy {
    BonusDrop bonusDrop;  // Information about bonus drop on death
};

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
    float timer = 0.0f;           // Timer for frame switching
    float lifetime = 0.0f;        // Total time alive (for non-persistent destruction)
    float frameDuration = 0.1f;   // Switch frame every 0.1 seconds (faster animation)
    bool currentFrame = false;    // false = frame 1, true = frame 2
    bool persistent = false;      // If true, animation stays active (for companion turret)
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

// Camera entity for scroll management via ECS
// The camera's Position.x represents the current scroll offset
// MovementSystem updates Position based on Velocity
struct Camera {
    float scroll_speed = 60.0f;  // Base scroll speed (stored for reference)
};

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

// BonusType enum is defined above (before Enemy struct)

// Wave System

// Tag entities with their source wave for completion tracking
struct WaveEntityTag {
    uint32_t wave_number = 0;  // Which wave spawned this entity
    bool is_boss = false;      // Special handling for boss entities (can't scroll away)
};

// Track active wave completion status
struct ActiveWave {
    uint32_t wave_number = 0;
    uint32_t entities_spawned = 0;     // Total entities spawned in this wave
    uint32_t entities_remaining = 0;   // Alive + on-screen entities
    float wave_start_time = 0.0f;      // When wave started (for completion time)
    bool completion_pending = false;   // Waiting for entities to clear
};

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

// BonusDrop struct is defined above (before Enemy struct)

struct WaveSpawnData {
    EntitySpawnType entityType = EntitySpawnType::ENEMY;
    EnemyType enemyType = EnemyType::Basic;  // Used if entityType is ENEMY
    BonusType bonusType = BonusType::HEALTH; // Used if entityType is POWERUP
    float positionX = 0.0f;                   // Relative to scroll position
    float positionY = 0.0f;                   // Absolute Y position
    int count = 1;                            // Number of entities to spawn
    SpawnPattern pattern = SpawnPattern::SINGLE;
    float spacing = 0.0f;                     // Spacing between entities in pattern
    BonusDrop bonusDrop;                      // Optional bonus drop on death (for enemies)
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

struct BonusLifetime {
    float timeRemaining = 10.0f;  // Durée de vie en secondes
};

struct Shield {
    bool active = true;  // Se désactive après 1 hit
};

struct SpeedBoost {
    float timeRemaining = 20.0f;      // Durée restante
    float multiplier = 1.5f;          // +50% vitesse
    float originalSpeed = 0.0f;       // Vitesse originale pour restauration
};

// Bonus Weapon System

struct BonusWeapon {
    size_t weaponEntity = -1;         // Entity du vaisseau bonus attaché
    float timeSinceLastFire = 0.0f;   // Cooldown du tir
    bool active = true;                // Arme active ou non
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



#endif /* !GAME_COMPONENTS_HPP_ */
