/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** WaveSpawnerSystem implementation
*/

#include "systems/WaveSpawnerSystem.hpp"
#include "components/CombatHelpers.hpp"
#include "AssetsPaths.hpp"
#include <iostream>
#include <cmath>
#include <cstdlib>

WaveSpawnerSystem::WaveSpawnerSystem(engine::IGraphicsPlugin& graphics)
    : graphics_(graphics)
{
}

void WaveSpawnerSystem::init(Registry& registry)
{
    std::cout << "WaveSpawnerSystem: Initialization" << std::endl;
    loadTextures();

    // Create a WaveController entity to track wave state
    Entity waveController = registry.spawn_entity();
    registry.add_component(waveController, WaveController{
        "assets::paths::WAVES_CONFIG",
        0.0f,  // totalScrollDistance
        0,     // currentWaveIndex
        0,     // currentWaveNumber
        0,     // totalWaveCount
        false  // allWavesCompleted
    });

    // Try to load default wave configuration
    if (loadWaveConfiguration("assets::paths::WAVES_CONFIG")) {
        std::cout << "WaveSpawnerSystem: Loaded wave configuration with walls" << std::endl;
        // Update total wave count in the component
        auto& waveControllers = registry.get_components<WaveController>();
        if (waveControllers.has_entity(waveController)) {
            waveControllers[waveController].totalWaveCount = config_.waves.size();
        }
    } else {
        std::cerr << "WaveSpawnerSystem: Warning - No wave configuration loaded" << std::endl;
    }
}

void WaveSpawnerSystem::shutdown()
{
    std::cout << "WaveSpawnerSystem: Shutdown" << std::endl;

    // Unload textures
    if (basicEnemyTex_ != engine::INVALID_HANDLE) {
        graphics_.unload_texture(basicEnemyTex_);
    }
    if (fastEnemyTex_ != engine::INVALID_HANDLE && fastEnemyTex_ != basicEnemyTex_) {
        graphics_.unload_texture(fastEnemyTex_);
    }
    if (tankEnemyTex_ != engine::INVALID_HANDLE && tankEnemyTex_ != basicEnemyTex_) {
        graphics_.unload_texture(tankEnemyTex_);
    }
    if (bossEnemyTex_ != engine::INVALID_HANDLE && bossEnemyTex_ != basicEnemyTex_) {
        graphics_.unload_texture(bossEnemyTex_);
    }
    if (wallTex_ != engine::INVALID_HANDLE) {
        graphics_.unload_texture(wallTex_);
    }
    if (obstacleTex_ != engine::INVALID_HANDLE) {
        graphics_.unload_texture(obstacleTex_);
    }
    if (bulletTex_ != engine::INVALID_HANDLE) {
        graphics_.unload_texture(bulletTex_);
    }
}

bool WaveSpawnerSystem::loadWaveConfiguration(const std::string& filepath)
{
    try {
        config_ = WaveLoader::loadWaveConfig(filepath);

        if (!WaveLoader::validateWaveConfig(config_)) {
            std::cerr << "WaveSpawnerSystem: Configuration validation failed" << std::endl;
            return false;
        }

        configLoaded_ = true;
        currentWaveIndex_ = 0;
        totalScrollDistance_ = 0.0f;
        waitingForTimeDelay_ = false;

        std::cout << "WaveSpawnerSystem: Loaded " << config_.waves.size()
                  << " waves from " << filepath << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "WaveSpawnerSystem: Failed to load configuration: "
                  << e.what() << std::endl;
        configLoaded_ = false;
        return false;
    }
}

void WaveSpawnerSystem::reset()
{
    currentWaveIndex_ = 0;
    totalScrollDistance_ = 0.0f;
    timeSinceWaveTrigger_ = 0.0f;
    waitingForTimeDelay_ = false;
    lastScrollPosition_ = 0.0f;

    // Reset all wave triggers
    for (auto& wave : config_.waves) {
        wave.trigger.triggered = false;
    }

    std::cout << "WaveSpawnerSystem: Reset" << std::endl;
}

void WaveSpawnerSystem::update(Registry& registry, float dt)
{
    if (!configLoaded_) {
        return; // No configuration loaded
    }

    // Update scroll tracking
    updateScrollTracking(registry, dt);

    // Check and trigger waves
    checkWaveTriggers(registry, dt);
}

void WaveSpawnerSystem::updateScrollTracking(Registry& registry, float dt)
{
    // Track scrolling using ScrollingSystem's scroll speed (-100 px/s)
    // Since scrolling is constant, we can calculate distance from time
    // TODO: Get actual scroll speed from LevelSystem/Camera
    const float SCROLL_SPEED = 100.0f; // Absolute value of scroll speed

    totalScrollDistance_ += SCROLL_SPEED * dt;
    
    // Convert scroll distance to current chunk index (1 chunk = 480px = 30 tiles * 16px)
    int currentChunk = static_cast<int>(totalScrollDistance_ / (30.0f * 16.0f));
    
    // Debug output every new chunk
    static int lastDebugChunk = -1;
    if (currentChunk > lastDebugChunk) {
        std::cout << "WaveSpawnerSystem: Entered Chunk " << currentChunk 
                  << " (Scroll: " << totalScrollDistance_ << "px)" << std::endl;
        lastDebugChunk = currentChunk;
    }
}

void WaveSpawnerSystem::checkWaveTriggers(Registry& registry, float dt)
{
    // Sync procedural settings from LevelController if present
    if (registry.has_component_registered<LevelController>()) {
        auto& lcs = registry.get_components<LevelController>();
        auto& waveControllers = registry.get_components<WaveController>();
        
        if (lcs.size() > 0 && waveControllers.size() > 0) {
            const auto& lc = lcs.get_data_at(0);
            auto& wc = waveControllers.get_data_at(0);
            
            // Level 2 (Nebula) is infinite procedural
            if (lc.current_level == 2 && !wc.proceduralMobs) {
                wc.proceduralMobs = true;
                std::cout << "[WaveSpawnerSystem] Enabled procedural mobs for Level 2 (Client)" << std::endl;
            }
        }
    }

    if (currentWaveIndex_ >= config_.waves.size()) {
        if (config_.loopWaves) {
            // Reset and loop back to first wave
            reset();
        } else {
            // Check if procedural mobs are enabled
            auto& waveControllers = registry.get_components<WaveController>();
            if (waveControllers.size() > 0) {
                auto& wc = waveControllers.get_data_at(0);
                if (wc.proceduralMobs) {
                    generateProceduralWave(registry);
                    return; // Procedural wave added, logic will continue next frame
                }
            }
        }
        return; // All waves completed
    }

    auto& wave = config_.waves[currentWaveIndex_];

    // If already triggered and waiting for time delay
    if (waitingForTimeDelay_) {
        timeSinceWaveTrigger_ += dt;

        if (timeSinceWaveTrigger_ >= wave.trigger.timeDelay) {
            // Time delay complete, spawn the wave
            spawnWave(registry, wave);

            waitingForTimeDelay_ = false;
            timeSinceWaveTrigger_ = 0.0f;
            currentWaveIndex_++;
        }
        return;
    }

    // Check if chunk trigger is met
    // We trigger if:
    // 1. currentChunk > trigger.chunkId (passed the chunk)
    // 2. OR currentChunk == trigger.chunkId AND offset condition met
    
    // Calculate current chunk and offset
    float chunkSizePx = 30.0f * 16.0f; // 480px
    int currentChunk = static_cast<int>(totalScrollDistance_ / chunkSizePx);
    float currentOffset = (totalScrollDistance_ - (currentChunk * chunkSizePx)) / chunkSizePx;

    bool shouldTrigger = false;

    if (currentChunk > wave.trigger.chunkId) {
        shouldTrigger = true;
    } else if (currentChunk == wave.trigger.chunkId) {
        if (currentOffset >= wave.trigger.offset) {
            shouldTrigger = true;
        }
    }

    if (!wave.trigger.triggered && shouldTrigger) {
        wave.trigger.triggered = true;

        std::cout << "========================================" << std::endl;
        std::cout << "WAVE " << wave.waveNumber << " TRIGGERED!" << std::endl;
        std::cout << "Chunk: " << wave.trigger.chunkId << " (Current: " << currentChunk << ")" << std::endl;
        std::cout << "========================================" << std::endl;

        // Update WaveController component
        auto& waveControllers = registry.get_components<WaveController>();
        for (size_t i = 0; i < waveControllers.size(); ++i) {
            if (waveControllers.has_entity(i)) {
                waveControllers[i].currentWaveIndex = currentWaveIndex_;
                waveControllers[i].currentWaveNumber = wave.waveNumber;
                waveControllers[i].totalScrollDistance = totalScrollDistance_;
                waveControllers[i].allWavesCompleted = (currentWaveIndex_ + 1 >= config_.waves.size());
                break; // Only one WaveController should exist
            }
        }

        // Check if there's a time delay
        if (wave.trigger.timeDelay > 0.0f) {
            waitingForTimeDelay_ = true;
            timeSinceWaveTrigger_ = 0.0f;
        } else {
            // No delay, spawn immediately
            spawnWave(registry, wave);
            currentWaveIndex_++;
        }
    }
}

void WaveSpawnerSystem::spawnWave(Registry& registry, const WaveLoader::Wave& wave)
{
    std::cout << "WaveSpawnerSystem: Spawning wave with "
              << wave.spawnData.size() << " spawn groups" << std::endl;

    for (const auto& spawnData : wave.spawnData) {
        applySpawnPattern(registry, spawnData);
    }
}

void WaveSpawnerSystem::applySpawnPattern(Registry& registry, const WaveSpawnData& spawnData)
{
    switch (spawnData.pattern) {
        case SpawnPattern::SINGLE:
            // Spawn single entity at specified position
            spawnEntity(registry, spawnData);
            break;

        case SpawnPattern::LINE: {
            // Spawn entities in horizontal line
            for (int i = 0; i < spawnData.count; ++i) {
                WaveSpawnData singleSpawn = spawnData;
                singleSpawn.positionY = spawnData.positionY + (i * spawnData.spacing);

                // Clamp to valid range
                if (singleSpawn.positionY < WAVE_SPAWN_MIN_Y) {
                    singleSpawn.positionY = WAVE_SPAWN_MIN_Y;
                } else if (singleSpawn.positionY > WAVE_SPAWN_MAX_Y) {
                    singleSpawn.positionY = WAVE_SPAWN_MAX_Y;
                }

                spawnEntity(registry, singleSpawn);
            }
            break;
        }

        case SpawnPattern::GRID: {
            // Spawn entities in grid pattern
            int cols = static_cast<int>(std::ceil(std::sqrt(spawnData.count)));
            int rows = (spawnData.count + cols - 1) / cols;

            for (int i = 0; i < spawnData.count; ++i) {
                int row = i / cols;
                int col = i % cols;

                WaveSpawnData singleSpawn = spawnData;
                singleSpawn.positionX = spawnData.positionX + (col * WAVE_FORMATION_SPACING_X);
                singleSpawn.positionY = spawnData.positionY + (row * spawnData.spacing);

                // Clamp Y position
                if (singleSpawn.positionY < WAVE_SPAWN_MIN_Y) {
                    singleSpawn.positionY = WAVE_SPAWN_MIN_Y;
                } else if (singleSpawn.positionY > WAVE_SPAWN_MAX_Y) {
                    singleSpawn.positionY = WAVE_SPAWN_MAX_Y;
                }

                spawnEntity(registry, singleSpawn);
            }
            break;
        }

        case SpawnPattern::RANDOM: {
            // Spawn entities at random Y positions
            for (int i = 0; i < spawnData.count; ++i) {
                WaveSpawnData singleSpawn = spawnData;
                singleSpawn.positionY = WAVE_SPAWN_MIN_Y +
                    static_cast<float>(rand() % static_cast<int>(WAVE_SPAWN_MAX_Y - WAVE_SPAWN_MIN_Y));

                spawnEntity(registry, singleSpawn);
            }
            break;
        }

        case SpawnPattern::FORMATION: {
            // V-formation pattern
            int half = spawnData.count / 2;
            for (int i = 0; i < spawnData.count; ++i) {
                WaveSpawnData singleSpawn = spawnData;

                if (i < half) {
                    // Upper arm of V
                    singleSpawn.positionX = spawnData.positionX - (i * WAVE_FORMATION_SPACING_X);
                    singleSpawn.positionY = spawnData.positionY - (i * spawnData.spacing);
                } else {
                    // Lower arm of V
                    int offset = i - half;
                    singleSpawn.positionX = spawnData.positionX - (offset * WAVE_FORMATION_SPACING_X);
                    singleSpawn.positionY = spawnData.positionY + (offset * spawnData.spacing);
                }

                // Clamp Y position
                if (singleSpawn.positionY < WAVE_SPAWN_MIN_Y) {
                    singleSpawn.positionY = WAVE_SPAWN_MIN_Y;
                } else if (singleSpawn.positionY > WAVE_SPAWN_MAX_Y) {
                    singleSpawn.positionY = WAVE_SPAWN_MAX_Y;
                }

                spawnEntity(registry, singleSpawn);
            }
            break;
        }
    }
}

void WaveSpawnerSystem::spawnEntity(Registry& registry, const WaveSpawnData& spawnData)
{
    switch (spawnData.entityType) {
        case EntitySpawnType::ENEMY:
            spawnEnemy(registry, spawnData.enemyType, spawnData.positionX, spawnData.positionY, spawnData.bonusDrop);
            break;

        case EntitySpawnType::WALL:
            spawnWall(registry, spawnData.positionX, spawnData.positionY);
            break;

        case EntitySpawnType::OBSTACLE:
            spawnObstacle(registry, spawnData.positionX, spawnData.positionY);
            break;

        case EntitySpawnType::POWERUP:
            spawnBonus(registry, spawnData.bonusType, spawnData.positionX, spawnData.positionY);
            break;
    }
}

Entity WaveSpawnerSystem::spawnEnemy(Registry& registry, EnemyType type, float x, float y, const BonusDrop& bonusDrop)
{
    Entity e = registry.spawn_entity();

    // Get stats from config
    float detection = ENEMY_BASIC_DETECTION;
    float cooldown = ENEMY_BASIC_SHOOT_COOLDOWN;
    float speed = ENEMY_BASIC_SPEED;
    int health = ENEMY_BASIC_HEALTH;
    get_enemy_stats(type, detection, cooldown, speed, health);

    engine::TextureHandle tex = getEnemyTexture(type);
    engine::Color tint = engine::Color::White;

    switch (type) {
        case EnemyType::Basic:
            tint = engine::Color{200, 200, 200, 255}; // Light Grey
            break;
        case EnemyType::Fast:
            tint = engine::Color{255, 100, 100, 255}; // Reddish
            break;
        case EnemyType::Tank:
            tint = engine::Color{100, 100, 255, 255}; // Blueish
            break;
        case EnemyType::Boss:
            tint = engine::Color{255, 200, 0, 255}; // Golden
            break;
    }

    engine::Vector2f size = graphics_.get_texture_size(tex);

    registry.add_component(e, Position{x, y});
    registry.add_component(e, Velocity{-speed, 0.0f});
    registry.add_component(e, Sprite{tex, size.x, size.y, 0.0f, tint, 0.0f, 0.0f, 0});
    registry.add_component(e, Collider{size.x, size.y});
    registry.add_component(e, Enemy{bonusDrop});
    registry.add_component(e, AI{type, detection, cooldown, 0.0f, speed});
    registry.add_component(e, Health{health, health});

    if (bonusDrop.enabled) {
        std::cout << "[WaveSpawnerSystem] Enemy " << e << " has bonusDrop enabled (type: "
                  << static_cast<int>(bonusDrop.bonusType) << ", chance: " << bonusDrop.dropChance << ")" << std::endl;
    }

    // Score based on enemy type
    int scoreValue = WAVE_DEFAULT_ENEMY_TYPE;
    switch (type) {
        case EnemyType::Basic: scoreValue = 100; break;
        case EnemyType::Fast:  scoreValue = 200; break;
        case EnemyType::Tank:  scoreValue = 300; break;
        case EnemyType::Boss:  scoreValue = 1000; break;
    }
    registry.add_component(e, Score{scoreValue});

    return e;
}

Entity WaveSpawnerSystem::spawnWall(Registry& registry, float x, float y)
{
    Entity e = registry.spawn_entity();

    engine::Vector2f size{WAVE_SPAWN_WALL_WIDTH, WAVE_SPAWN_WALL_HEIGHT};

    registry.add_component(e, Position{x, y});
    registry.add_component(e, Velocity{0.0f, 0.0f}); // Stationary

    // Use lock.png texture for walls
    registry.add_component(e, Sprite{wallTex_, size.x, size.y, 0.0f,
                                    engine::Color::White, 0.0f, 0.0f, -1});

    registry.add_component(e, Collider{size.x, size.y});
    registry.add_component(e, Wall{});
    registry.add_component(e, Health{WAVE_DEFAULT_WALL_HEALTH, WAVE_DEFAULT_WALL_HEALTH});
    registry.add_component(e, Scrollable{1.0f, false, true}); // Destroy when offscreen

    std::cout << "WaveSpawnerSystem: Spawned wall at (" << x << ", " << y << ")" << std::endl;

    return e;
}

Entity WaveSpawnerSystem::spawnObstacle(Registry& registry, float x, float y)
{
    Entity e = registry.spawn_entity();

    engine::Vector2f size{WAVE_SPAWN_WALL_WIDTH, WAVE_SPAWN_WALL_HEIGHT};

    registry.add_component(e, Position{x, y});
    registry.add_component(e, Velocity{0.0f, 0.0f});

    // Use lock.png texture for obstacles (with slight color tint to differentiate)
    registry.add_component(e, Sprite{obstacleTex_, size.x, size.y, 0.0f,
                                    engine::Color{200, 200, 200, 255}, 0.0f, 0.0f, 0});

    registry.add_component(e, Collider{size.x, size.y});
    registry.add_component(e, Wall{}); // Obstacles use Wall component too for collision
    registry.add_component(e, Health{WAVE_DEFAULT_OBSTACLE_HEALTH, WAVE_DEFAULT_OBSTACLE_HEALTH});
    registry.add_component(e, Scrollable{1.0f, false, true});

    std::cout << "WaveSpawnerSystem: Spawned obstacle at (" << x << ", " << y << ")" << std::endl;

    return e;
}

Entity WaveSpawnerSystem::spawnBonus(Registry& registry, BonusType type, float x, float y)
{
    Entity e = registry.spawn_entity();

    constexpr float BONUS_RADIUS = 20.0f;

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
    }

    // Récupérer la taille du sprite bullet
    const float bulletWidth = BONUS_RADIUS * 2.0f;
    const float bulletHeight = BONUS_RADIUS * 2.0f;

    registry.add_component(e, Position{x, y});
    registry.add_component(e, Bonus{type, BONUS_RADIUS});
    registry.add_component(e, Collider{BONUS_RADIUS * 2, BONUS_RADIUS * 2});
    registry.add_component(e, Scrollable{1.0f, false, true}); // Scroll and destroy offscreen
    Sprite sprite{
        bulletTex_,
        bulletWidth,
        bulletHeight,
        0.0f,
        tint,
        0.0f,
        0.0f,
        0
    };
    sprite.source_rect = {0.0f, 0.0f, 16.0f, 16.0f};
    sprite.origin_x = bulletWidth / 2.0f;
    sprite.origin_y = bulletHeight / 2.0f;
    registry.add_component(e, sprite);

    std::cout << "WaveSpawnerSystem: Spawned bonus " << typeName << " at (" << x << ", " << y << ")" << std::endl;

    return e;
}

engine::TextureHandle WaveSpawnerSystem::getEnemyTexture(EnemyType type) const
{
    switch (type) {
        case EnemyType::Basic: return basicEnemyTex_;
        case EnemyType::Fast:  return fastEnemyTex_;
        case EnemyType::Tank:  return tankEnemyTex_;
        case EnemyType::Boss:  return bossEnemyTex_;
        default: return basicEnemyTex_;
    }
}

void WaveSpawnerSystem::loadTextures()
{
    std::cout << "WaveSpawnerSystem: Loading textures..." << std::endl;
    using namespace assets::paths;

    basicEnemyTex_ = graphics_.load_texture(ENEMY_BASIC);
    fastEnemyTex_ = basicEnemyTex_;  // Reuse for now
    tankEnemyTex_ = basicEnemyTex_;  // Reuse for now
    bossEnemyTex_ = basicEnemyTex_;  // Reuse for now
    bulletTex_ = graphics_.load_texture(SHOT_ANIMATION);

    // Load wall/obstacle textures
    wallTex_ = graphics_.load_texture(WALL);
    obstacleTex_ = graphics_.load_texture(WALL);

    std::cout << "WaveSpawnerSystem: Textures loaded" << std::endl;
    std::cout << "  Wall/Obstacle textures: lock.png" << std::endl;
}

void WaveSpawnerSystem::generateProceduralWave(Registry& registry)
{
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }

    // Create a new wave
    WaveLoader::Wave newWave;
    newWave.waveNumber = config_.waves.size() + 1;
    
    // Set trigger at next chunk
    // One chunk = 30 tiles * 16px = 480px
    float chunkSizePx = 30.0f * 16.0f;
    int currentChunk = static_cast<int>(totalScrollDistance_ / chunkSizePx);
    
    newWave.trigger.chunkId = currentChunk + 1; // Trigger at next chunk boundary
    newWave.trigger.offset = 0.1f;              // Slightly inside the chunk (10%)
    newWave.trigger.timeDelay = 0.0f;
    newWave.trigger.triggered = false;
    
    // Randomize Content
    // 3 to 6 enemies
    int enemyCount = 3 + (std::rand() % 4);
    
    // Random type
    EnemyType types[] = {EnemyType::Basic, EnemyType::Fast, EnemyType::Tank};
    EnemyType selectedType = types[std::rand() % 3];
    
    // Random pattern
    SpawnPattern patterns[] = {SpawnPattern::LINE, SpawnPattern::GRID, SpawnPattern::RANDOM, SpawnPattern::FORMATION};
    SpawnPattern selectedPattern = patterns[std::rand() % 4];
    
    WaveSpawnData spawnData;
    spawnData.entityType = EntitySpawnType::ENEMY;
    spawnData.enemyType = selectedType;
    spawnData.count = enemyCount;
    spawnData.pattern = selectedPattern;
    spawnData.positionX = 100.0f; // Offset from right edge
    
    // Randomized Y position
    // Screen height ~1080, keep margins
    spawnData.positionY = 100.0f + static_cast<float>(std::rand() % 800);
    spawnData.spacing = 80.0f + static_cast<float>(std::rand() % 100);
    
    // 5% chance to drop bonus
    if ((std::rand() % 100) < 5) {
        spawnData.bonusDrop.enabled = true;
        spawnData.bonusDrop.dropChance = 1.0f;
        BonusType bonuses[] = {BonusType::HEALTH, BonusType::SHIELD, BonusType::SPEED, BonusType::BONUS_WEAPON};
        spawnData.bonusDrop.bonusType = bonuses[std::rand() % 4];
    }
    
    newWave.spawnData.push_back(spawnData);
    
    // Add to configuration
    config_.waves.push_back(newWave);
    
    // Update total count in component so UI knows/debugging works
    auto& waveControllers = registry.get_components<WaveController>();
    if (waveControllers.size() > 0) {
        waveControllers.get_data_at(0).totalWaveCount = config_.waves.size();
    }
    
    std::cout << "[WaveSpawnerSystem] Generated procedural wave " << newWave.waveNumber 
              << " for chunk " << newWave.trigger.chunkId 
              << " (Type: " << static_cast<int>(selectedType) 
              << ", Count: " << enemyCount << ")" << std::endl;
}
