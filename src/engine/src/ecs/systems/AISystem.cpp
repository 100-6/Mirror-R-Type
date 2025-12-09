/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AISystem
*/

#include "ecs/systems/AISystem.hpp"
#include "ecs/events/GameEvents.hpp"
#include <iostream>
#include <cmath>
#include <cstdlib> // for rand()

AISystem::AISystem(engine::IGraphicsPlugin& graphics)
    : graphics_(graphics)
{
}

void AISystem::init(Registry& registry)
{
    std::cout << "AISystem: Initialisation" << std::endl;

    // Load textures
    // Note: In a real engine, these might be cached in a ResourceManager
    basicEnemyTex_ = graphics_.load_texture("assets/sprite/enemy.png");
    fastEnemyTex_ = graphics_.load_texture("assets/sprite/enemy.png"); // Reuse for now
    tankEnemyTex_ = graphics_.load_texture("assets/sprite/enemy.png"); // Reuse for now
    bulletTex_ = graphics_.load_texture("assets/sprite/bullet.png");

    // Setup Waves (5 waves as requested)
    waves_.push_back({2, 0, 0, 3.0f}); // Wave 1: 2 Basic
    waves_.push_back({3, 1, 0, 2.5f}); // Wave 2: 3 Basic, 1 Fast
    waves_.push_back({2, 2, 1, 2.0f}); // Wave 3: 2 Basic, 2 Fast, 1 Tank
    waves_.push_back({0, 4, 2, 1.5f}); // Wave 4: 4 Fast, 2 Tank
    waves_.push_back({5, 3, 3, 1.0f}); // Wave 5: Swarm

    startNextWave();
}

void AISystem::shutdown()
{
    std::cout << "AISystem: Shutdown" << std::endl;
    // Unload textures if this system owns them. 
    // If Resource Manager exists, it would handle this.
    // Assuming we should cleanup:
    if (basicEnemyTex_) graphics_.unload_texture(basicEnemyTex_);
    if (fastEnemyTex_ != basicEnemyTex_) graphics_.unload_texture(fastEnemyTex_);
    if (tankEnemyTex_ != basicEnemyTex_) graphics_.unload_texture(tankEnemyTex_);
    if (bulletTex_) graphics_.unload_texture(bulletTex_);
}

void AISystem::startNextWave()
{
    if (currentWaveIndex_ >= waves_.size()) {
        std::cout << "AISystem: All waves completed!" << std::endl;
        isWaveInProgress_ = false;
        return;
    }

    std::cout << "AISystem: Starting Wave " << (currentWaveIndex_ + 1) << std::endl;
    isWaveInProgress_ = true;
    enemiesSpawnedInWave_ = 0;
    spawnTimer_ = 0.0f;
}

void AISystem::update(Registry& registry, float dt)
{
    // 1. Wave Management
    auto& enemies = registry.get_components<AI>();
    int aliveEnemies = 0;
    // Count alive enemies that are part of the wave (AI component)
    for (size_t i = 0; i < enemies.size(); ++i) {
        if (enemies.has_entity(enemies.get_entity_at(i))) {
            aliveEnemies++;
        }
    }

    if (isWaveInProgress_) {
        const auto& wave = waves_[currentWaveIndex_];
        int totalEnemiesInWave = wave.basicCount + wave.fastCount + wave.tankCount;

        spawnTimer_ += dt;

        if (enemiesSpawnedInWave_ < totalEnemiesInWave) {
            if (spawnTimer_ >= wave.spawnInterval) {
                spawnTimer_ = 0.0f;
                
                // Determine what to spawn
                EnemyType typeToSpawn = EnemyType::Basic;
                // Simple logic: spawn basics, then fast, then tanks
                if (enemiesSpawnedInWave_ < wave.basicCount) {
                    typeToSpawn = EnemyType::Basic;
                } else if (enemiesSpawnedInWave_ < wave.basicCount + wave.fastCount) {
                    typeToSpawn = EnemyType::Fast;
                } else {
                    typeToSpawn = EnemyType::Tank;
                }

                spawnEnemy(registry, typeToSpawn);
                enemiesSpawnedInWave_++;
            }
        } else if (aliveEnemies == 0) {
            // Wave Complete
            std::cout << "AISystem: Wave " << (currentWaveIndex_ + 1) << " Complete!" << std::endl;
            isWaveInProgress_ = false;
            waveTimer_ = timeBetweenWaves_;
            currentWaveIndex_++;
        }
    } else {
        // Between waves
        if (currentWaveIndex_ < waves_.size()) {
            waveTimer_ -= dt;
            if (waveTimer_ <= 0.0f) {
                startNextWave();
            }
        }
    }

    // 2. AI Behavior
    updateEnemyBehavior(registry, dt);
}

void AISystem::spawnEnemy(Registry& registry, EnemyType type)
{
    Entity e = registry.spawn_entity();
    
    // Default stats
    float speed = 100.0f;
    int health = 50;
    engine::TextureHandle tex = basicEnemyTex_;
    engine::Color tint = engine::Color::White;
    float cooldown = 2.0f;

    switch (type) {
        case EnemyType::Basic:
            speed = 100.0f;
            health = 30;
            tex = basicEnemyTex_;
            tint = engine::Color{200, 200, 200, 255}; // Light Grey
            cooldown = 2.0f;
            break;
        case EnemyType::Fast:
            speed = 250.0f;
            health = 20;
            tex = fastEnemyTex_;
            tint = engine::Color{255, 100, 100, 255}; // Reddish
            cooldown = 1.0f;
            break;
        case EnemyType::Tank:
            speed = 50.0f;
            health = 100;
            tex = tankEnemyTex_;
            tint = engine::Color{100, 100, 255, 255}; // Blueish
            cooldown = 3.0f;
            break;
        default: break;
    }

    engine::Vector2f size = graphics_.get_texture_size(tex);
    
    // Random Y position
    float yPos = 50.0f + static_cast<float>(rand() % 900); // Keep within 1080p roughly

    registry.add_component(e, Position{1920.0f + 50.0f, yPos}); // Spawn off-screen right
    registry.add_component(e, Velocity{-speed, 0.0f}); // Default move left
    registry.add_component(e, Sprite{tex, size.x, size.y, 0.0f, tint, 0.0f, 0.0f, 0});
    registry.add_component(e, Collider{size.x, size.y});
    registry.add_component(e, Enemy{});
    registry.add_component(e, AI{type, 800.0f, cooldown, 0.0f, speed});
    registry.add_component(e, Health{health, health});
    registry.add_component(e, Score{type == EnemyType::Tank ? 300 : (type == EnemyType::Fast ? 200 : 100)});
}

Entity AISystem::findNearestPlayer(Registry& registry, const Position& enemyPos)
{
    auto& players = registry.get_components<Controllable>();
    auto& positions = registry.get_components<Position>();
    
    Entity nearest = -1;
    float minDistSq = 999999999.0f;

    for (size_t i = 0; i < players.size(); ++i) {
        Entity p = players.get_entity_at(i);
        if (positions.has_entity(p)) {
            const auto& pos = positions[p];
            float dx = pos.x - enemyPos.x;
            float dy = pos.y - enemyPos.y;
            float distSq = dx*dx + dy*dy;
            
            if (distSq < minDistSq) {
                minDistSq = distSq;
                nearest = p;
            }
        }
    }
    return nearest;
}

void AISystem::updateEnemyBehavior(Registry& registry, float dt)
{
    auto& ais = registry.get_components<AI>();
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& sprites = registry.get_components<Sprite>();

    for (size_t i = 0; i < ais.size(); ++i) {
        Entity e = ais.get_entity_at(i);
        auto& ai = ais[e];
        
        if (!positions.has_entity(e) || !velocities.has_entity(e)) continue;

        auto& pos = positions[e];
        auto& vel = velocities[e];

        // 1. Movement Logic
        Entity target = findNearestPlayer(registry, pos);
        
        if (ai.type == EnemyType::Basic) {
             // Sine wave movement based on X position
             vel.x = -ai.moveSpeed;
             vel.y = std::sin(pos.x * 0.01f) * 50.0f;
        } else if (ai.type == EnemyType::Tank) {
             // Tank moves slow and steady
             vel.x = -ai.moveSpeed;
             vel.y = 0.0f;
        } else if (ai.type == EnemyType::Fast) {
             // Fast seeks player
            if (target != static_cast<size_t>(-1) && positions.has_entity(target)) {
                const auto& targetPos = positions[target];
                
                // Calculate direction
                float dx = targetPos.x - pos.x;
                float dy = targetPos.y - pos.y;
                float dist = std::sqrt(dx*dx + dy*dy);

                if (dist > 0 && dist < ai.detectionRange) {
                    // Move towards player
                    // Normalize and apply speed
                    float moveX = (dx / dist) * ai.moveSpeed;
                    float moveY = (dy / dist) * ai.moveSpeed;
                    
                    vel.x = moveX;
                    vel.y = moveY;
                } else {
                    vel.x = -ai.moveSpeed;
                    vel.y = 0.0f;
                }
            } else {
                 vel.x = -ai.moveSpeed;
                 vel.y = 0.0f;
            }
        }

        // 2. Shooting Logic
        ai.timeSinceLastShot += dt;
        if (ai.timeSinceLastShot >= ai.shootCooldown) {
            // Check if player is in front (roughly)
            bool shouldShoot = false;
            if (target != static_cast<size_t>(-1) && positions.has_entity(target)) {
                const auto& targetPos = positions[target];
                if (targetPos.x < pos.x) { // Player is to the left
                     shouldShoot = true;
                }
            } else if (pos.x < 1920.0f && pos.x > 0.0f) {
                // Also shoot if just on screen, even if no target found (suppressive fire)
                shouldShoot = true;
            }

            if (shouldShoot) {
                 ai.timeSinceLastShot = 0.0f;
                 
                 engine::Vector2f bulletSize = graphics_.get_texture_size(bulletTex_);
                 float spawnX = pos.x;
                 float spawnY = pos.y;
                 if (sprites.has_entity(e)) {
                     spawnY += sprites[e].height / 2.0f;
                 }
                 
                 auto createBullet = [&](float vy_offset) {
                     Entity bullet = registry.spawn_entity();
                     registry.add_component(bullet, Position{spawnX, spawnY});
                     registry.add_component(bullet, Velocity{-400.0f, vy_offset}); // Shoot left with optional Y spread
                     registry.add_component(bullet, Sprite{bulletTex_, bulletSize.x, bulletSize.y, 0.0f, engine::Color{255, 100, 100, 255}}); 
                     registry.add_component(bullet, Collider{bulletSize.x, bulletSize.y});
                     registry.add_component(bullet, Projectile{});
                     registry.add_component(bullet, IsEnemyProjectile{});
                 };

                 createBullet(0.0f); // Center bullet
                 
                 if (ai.type == EnemyType::Tank) {
                     // Spread shot
                     createBullet(-100.0f); // Up diagonal
                     createBullet(100.0f);  // Down diagonal
                 }
            }
        }
    }
}
