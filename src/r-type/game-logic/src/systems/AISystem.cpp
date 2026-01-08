/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AISystem
*/

#include "systems/AISystem.hpp"
#include "components/CombatHelpers.hpp"
#include "ecs/events/GameEvents.hpp"
#include "AssetsPaths.hpp"
#include <iostream>
#include <cmath>
#include <cstdlib> // for rand()

namespace {
constexpr float ENEMY_PROJECTILE_WIDTH = 28.0f;
constexpr float ENEMY_PROJECTILE_HEIGHT = 12.0f;
}

AISystem::AISystem(engine::IGraphicsPlugin& graphics)
    : graphics_(graphics)
{
}

void AISystem::init(Registry& registry)
{
    std::cout << "AISystem: Initialisation" << std::endl;

    // Load textures for bullets only (enemies are spawned by WaveSpawnerSystem)
    bulletTex_ = graphics_.load_texture(assets::paths::BULLET_ANIMATION);
}

void AISystem::shutdown()
{
    std::cout << "AISystem: Shutdown" << std::endl;
    if (bulletTex_) graphics_.unload_texture(bulletTex_);
}

void AISystem::update(Registry& registry, float dt)
{
    // AI Behavior only - spawning is handled by WaveSpawnerSystem
    updateEnemyBehavior(registry, dt);
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
                 
                 const float bulletWidth = ENEMY_PROJECTILE_WIDTH;
                 const float bulletHeight = ENEMY_PROJECTILE_HEIGHT;
                 float spawnX = pos.x;
                 float spawnY = pos.y;
                 if (sprites.has_entity(e)) {
                     spawnY += sprites[e].height / 2.0f;
                 }
                 
                 auto createBullet = [&](float vy_offset) {
                     Entity bullet = registry.spawn_entity();
                     registry.add_component(bullet, Position{spawnX, spawnY});
                     registry.add_component(bullet, Velocity{-400.0f, vy_offset}); // Shoot left with optional Y spread
                     Sprite bulletSprite{
                         bulletTex_,
                         bulletWidth,
                         bulletHeight,
                         0.0f,
                         engine::Color{255, 100, 100, 255}
                     };
                     bulletSprite.source_rect = {16.0f, 0.0f, 16.0f, 16.0f};
                     bulletSprite.origin_x = bulletWidth / 2.0f;
                     bulletSprite.origin_y = bulletHeight / 2.0f;
                     registry.add_component(bullet, bulletSprite);
                     registry.add_component(bullet, Collider{bulletWidth, bulletHeight});
                     registry.add_component(bullet, Projectile{180.0f, 5.0f, 0.0f, ProjectileFaction::Enemy});
                     registry.add_component(bullet, NoFriction{});
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
