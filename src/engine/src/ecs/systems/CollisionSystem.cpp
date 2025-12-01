/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CollisionSystem
*/

#include "ecs/systems/CollisionSystem.hpp"
#include <vector>
#include <utility>

bool CollisionSystem::check_collision(const Position& pos1, const Position& pos2,
    const Collider& col1, const Collider& col2)
    {
        // Ignore colliders with zero size
        if (col1.width <= 0 || col1.height <= 0 || col2.width <= 0 || col2.height <= 0) {
            return false;
        }

        float left1 = pos1.x;
        float right1 = pos1.x + col1.width;
        float up1 = pos1.y;
        float down1 = pos1.y + col1.height;

        float left2 = pos2.x;
        float right2 = pos2.x + col2.width;
        float up2 = pos2.y;
        float down2 = pos2.y + col2.height;

        return (right1 > left2)
            && (left1 < right2)
            && (down2 > up1)
            && (up2 < down1);
    }

CollisionSystem::~CollisionSystem() = default;

void CollisionSystem::init(Registry& registry)
{
    std::cout << "CollisionSystem: Initialisation." << std::endl;
}

void CollisionSystem::shutdown()
{
    std::cout << "CollisionSystem: Arrêt." << std::endl;
}

void CollisionSystem::update(Registry& registry)
{
    // Collision Projectile vs Enemy : Détruit les deux
    std::vector<std::pair<Entity, Entity>> projectile_enemy_collisions;
    scan_collisions<Projectile, Enemy>(registry, [&projectile_enemy_collisions](Entity bullet, Entity enemy) {
        projectile_enemy_collisions.push_back({bullet, enemy});
    });
    
    // Détruit toutes les paires collectées
    for (const auto& [bullet, enemy] : projectile_enemy_collisions) {
        registry.kill_entity(enemy);
        registry.kill_entity(bullet);
    }

    // Collision Player vs Wall : Repousse le joueur
    scan_collisions<Controllable, Wall>(registry, [&registry](Entity player, Entity wall) {
        auto& positions = registry.get_components<Position>();
        auto& colliders = registry.get_components<Collider>();

        Position& posP = positions[player];
        const Collider& colP = colliders[player];

        const Position& posW = positions[wall];
        const Collider& colW = colliders[wall];

        float overlapLeft = (posP.x + colP.width) - posW.x;
        float overlapRight = (posW.x + colW.width) - posP.x;
        float overlapTop = (posP.y + colP.height) - posW.y;
        float overlapBottom = (posW.y + colW.height) - posP.y;

        float minOverlapX = std::min(overlapLeft, overlapRight);
        float minOverlapY = std::min(overlapTop, overlapBottom);

        if (minOverlapX < minOverlapY) {
            if (overlapLeft < overlapRight)
                posP.x -= overlapLeft;
            else
                posP.x += overlapRight;
        } else {
            if (overlapTop < overlapBottom)
                posP.y -= overlapTop;
            else
                posP.y += overlapBottom;
        }
    });
}
