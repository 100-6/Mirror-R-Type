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

        // Center-based collision detection using half-extents
        float half_w1 = col1.width * 0.5f;
        float half_h1 = col1.height * 0.5f;
        float half_w2 = col2.width * 0.5f;
        float half_h2 = col2.height * 0.5f;

        float left1 = pos1.x - half_w1;
        float right1 = pos1.x + half_w1;
        float up1 = pos1.y - half_h1;
        float down1 = pos1.y + half_h1;

        float left2 = pos2.x - half_w2;
        float right2 = pos2.x + half_w2;
        float up2 = pos2.y - half_h2;
        float down2 = pos2.y + half_h2;

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

    // Collision Projectile (ennemi) vs Player : Applique les dégâts au joueur (ou casse le bouclier)
    auto& shields = registry.get_components<Shield>();
    scan_collisions<Projectile, Controllable>(registry, [&registry, &damages, &projectiles, &shields](Entity bullet, Entity player) {
        if (!projectiles.has_entity(bullet))
            return;

        if (projectiles[bullet].faction != ProjectileFaction::Enemy)
            return;

        registry.add_component(bullet, ToDestroy{});

        // Vérifier si le joueur a un bouclier actif
        if (shields.has_entity(player) && shields[player].active) {
            // Le bouclier absorbe le coup et se casse
            registry.remove_component<Shield>(player);
            std::cout << "CollisionSystem: Bouclier du joueur détruit!" << std::endl;
            return;
        }

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

        // Center-based collision resolution using half-extents
        float half_wP = colP.width * 0.5f;
        float half_hP = colP.height * 0.5f;
        float half_wW = colW.width * 0.5f;
        float half_hW = colW.height * 0.5f;

        // Calculate overlap on each axis
        float overlapLeft = (posP.x + half_wP) - (posW.x - half_wW);
        float overlapRight = (posW.x + half_wW) - (posP.x - half_wP);
        float overlapTop = (posP.y + half_hP) - (posW.y - half_hW);
        float overlapBottom = (posW.y + half_hW) - (posP.y - half_hP);

        float minOverlapX = std::min(overlapLeft, overlapRight);
        float minOverlapY = std::min(overlapTop, overlapBottom);

        // Push player out on axis with minimum overlap
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
