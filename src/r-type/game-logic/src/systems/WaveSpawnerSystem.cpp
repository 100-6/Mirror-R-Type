/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** WaveSpawnerSystem implementation
*/

#include "systems/WaveSpawnerSystem.hpp"
#include "components/CombatHelpers.hpp"
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
        "assets/waves_hard_maze.json",
        0.0f,  // totalScrollDistance
        0,     // currentWaveIndex
        0,     // currentWaveNumber
        0,     // totalWaveCount
        false  // allWavesCompleted
    });

    // Try to load default wave configuration
    if (loadWaveConfiguration("assets/waves_hard_maze.json")) {
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
    const float SCROLL_SPEED = 100.0f; // Absolute value of scroll speed

    totalScrollDistance_ += SCROLL_SPEED * dt;

    // Debug output every 100 pixels
    static float lastDebugDistance = 0.0f;
    if (totalScrollDistance_ - lastDebugDistance >= 100.0f) {
        std::cout << "WaveSpawnerSystem: Total scroll distance = " << totalScrollDistance_ << std::endl;
        lastDebugDistance = totalScrollDistance_;
    }
}

void WaveSpawnerSystem::checkWaveTriggers(Registry& registry, float dt)
{
    if (currentWaveIndex_ >= config_.waves.size()) {
        if (config_.loopWaves) {
            // Reset and loop back to first wave
            reset();
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

    // Check if scroll distance trigger is met
    if (!wave.trigger.triggered && totalScrollDistance_ >= wave.trigger.scrollDistance) {
        wave.trigger.triggered = true;

        std::cout << "========================================" << std::endl;
        std::cout << "WAVE " << wave.waveNumber << " TRIGGERED!" << std::endl;
        std::cout << "Scroll distance: " << totalScrollDistance_ << "px" << std::endl;
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
            spawnEnemy(registry, spawnData.enemyType, spawnData.positionX, spawnData.positionY);
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

Entity WaveSpawnerSystem::spawnEnemy(Registry& registry, EnemyType type, float x, float y)
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
    registry.add_component(e, Enemy{});
    registry.add_component(e, AI{type, detection, cooldown, 0.0f, speed});
    registry.add_component(e, Health{health, health});

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
    engine::Vector2f bulletSize = graphics_.get_texture_size(bulletTex_);

    registry.add_component(e, Position{x, y});
    registry.add_component(e, Bonus{type, BONUS_RADIUS});
    registry.add_component(e, Collider{BONUS_RADIUS * 2, BONUS_RADIUS * 2});
    registry.add_component(e, Scrollable{1.0f, false, true}); // Scroll and destroy offscreen
    registry.add_component(e, Sprite{
        bulletTex_,
        bulletSize.x,
        bulletSize.y,
        0.0f,
        tint,
        0.0f,
        0.0f,
        0  // Layer
    });

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

    basicEnemyTex_ = graphics_.load_texture("assets/sprite/enemy.png");
    fastEnemyTex_ = basicEnemyTex_;  // Reuse for now
    tankEnemyTex_ = basicEnemyTex_;  // Reuse for now
    bossEnemyTex_ = basicEnemyTex_;  // Reuse for now
    bulletTex_ = graphics_.load_texture("assets/sprite/bullet.png");

    // Load wall/obstacle textures
    wallTex_ = graphics_.load_texture("assets/sprite/lock.png");
    obstacleTex_ = graphics_.load_texture("assets/sprite/lock.png");

    std::cout << "WaveSpawnerSystem: Textures loaded" << std::endl;
    std::cout << "  Wall/Obstacle textures: lock.png" << std::endl;
}
