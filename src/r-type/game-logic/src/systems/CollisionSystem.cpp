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
    auto hide_projectile_sprite = [&registry](Entity projectile) {
        if (!registry.has_component_registered<Sprite>())
            return;
        auto& sprites = registry.get_components<Sprite>();
        if (!sprites.has_entity(projectile))
            return;
        registry.remove_component<Sprite>(projectile);
    };

    // Collision Projectile (joueur) vs Enemy : Applique les dégâts à l'ennemi
    scan_collisions<Projectile, Enemy>(registry, [&registry, &damages, &projectiles, &hide_projectile_sprite](Entity bullet, Entity enemy) {
        if (!projectiles.has_entity(bullet))
            return;

        if (projectiles[bullet].faction != ProjectileFaction::Player)
            return;

        registry.add_component(bullet, ToDestroy{});
        hide_projectile_sprite(bullet);
        // TODO: publier un ProjectileHitEvent ici pour déclencher un VFX/SFX côté client.

        int dmg = damages.has_entity(bullet) ? damages[bullet].value : 10;
        registry.get_event_bus().publish(ecs::DamageEvent{enemy, bullet, dmg});
    });

    // Collision Projectile (ennemi) vs Player : Applique les dégâts au joueur (ou casse le bouclier)
    auto& shields = registry.get_components<Shield>();
    scan_collisions<Projectile, Controllable>(registry, [&registry, &damages, &projectiles, &shields, &hide_projectile_sprite](Entity bullet, Entity player) {
        if (!projectiles.has_entity(bullet))
            return;

        if (projectiles[bullet].faction != ProjectileFaction::Enemy)
            return;

        registry.add_component(bullet, ToDestroy{});
        hide_projectile_sprite(bullet);
        // TODO: publier un ProjectileHitEvent ici pour déclencher un VFX/SFX côté client.

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

    // Collision Projectile vs Wall : Handled by scroll-aware collision system
    // Walls are in WORLD coordinates (static), projectiles are in SCREEN coordinates
    // See GameSession::update() which calls check_projectile_wall_collisions()

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

    // Collision Player vs Wall : Handled by scroll-aware collision system
    // Walls are in WORLD coordinates (static), players are in SCREEN coordinates
    // We need to get the scroll offset to properly check collisions
    // This is done via a separate method that has access to the scroll position
    // See GameSession::update() which calls check_player_wall_collisions()
}
