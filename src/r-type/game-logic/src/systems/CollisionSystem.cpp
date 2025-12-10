/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CollisionSystem
*/

#include "systems/CollisionSystem.hpp"
#include "components/GameComponents.hpp"
#include "ecs/events/InputEvents.hpp"
#include "ecs/events/GameEvents.hpp"

bool CollisionSystem::check_collision(const Position& pos1, const Position& pos2,
    const Collider& col1, const Collider& col2)
    {
        // Ignore colliders with zero size
        if (col1.width <= 0 || col1.height <= 0 || col2.width <= 0 || col2.height <= 0)
            return false;

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

void CollisionSystem::init(Registry& registry)
{
    std::cout << "CollisionSystem: Initialisation." << std::endl;
}

void CollisionSystem::shutdown()
{
    std::cout << "CollisionSystem: Arrêt." << std::endl;
}

void CollisionSystem::update(Registry& registry, float dt)
{
    // Update Invulnerability timers
    auto& invulnerabilities = registry.get_components<Invulnerability>();
    for (size_t i = 0; i < invulnerabilities.size(); i++) {
        Entity entity = invulnerabilities.get_entity_at(i);
        auto& invul = invulnerabilities[entity];
        if (invul.time_remaining > 0.0f)
            invul.time_remaining -= dt;
    }

    auto& damages = registry.get_components<Damage>();
    auto& projectiles = registry.get_components<Projectile>();

    // Collision Projectile (joueur) vs Enemy : Applique les dégâts à l'ennemi
    scan_collisions<Projectile, Enemy>(registry, [&registry, &damages, &projectiles](Entity bullet, Entity enemy) {
        if (!projectiles.has_entity(bullet))
            return;

        if (projectiles[bullet].faction != ProjectileFaction::Player)
            return;

        registry.add_component(bullet, ToDestroy{});

        int dmg = damages.has_entity(bullet) ? damages[bullet].value : 10;
        registry.get_event_bus().publish(ecs::DamageEvent{enemy, bullet, dmg});
    });

    // Collision Projectile (ennemi) vs Player : Applique les dégâts au joueur
    scan_collisions<Projectile, Controllable>(registry, [&registry, &damages, &projectiles](Entity bullet, Entity player) {
        if (!projectiles.has_entity(bullet))
            return;

        if (projectiles[bullet].faction != ProjectileFaction::Enemy)
            return;

        registry.add_component(bullet, ToDestroy{});

        int dmg = damages.has_entity(bullet) ? damages[bullet].value : 10;
        registry.get_event_bus().publish(ecs::DamageEvent{player, bullet, dmg});
    });

    // Collision Projectile vs Wall : Marque le projectile pour destruction
    scan_collisions<Projectile, Wall>(registry, [&registry](Entity bullet, Entity wall) {
        (void)wall;
        registry.add_component(bullet, ToDestroy{});
    });

    // Collision Player (Controllable) vs Enemy : Publie PlayerHitEvent
    scan_collisions<Controllable, Enemy>(registry, [&registry](Entity player, Entity enemy) {
        (void)enemy;

        auto& invulnerabilities = registry.get_components<Invulnerability>();
        if (invulnerabilities.has_entity(player)) {
            auto& invul = invulnerabilities[player];
            if (invul.time_remaining > 0.0f)
                return;
            invul.time_remaining = 3.0f;
        }

        registry.get_event_bus().publish(ecs::PlayerHitEvent{player, enemy});
    });

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
