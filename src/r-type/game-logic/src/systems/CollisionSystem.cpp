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

    // Collision Player vs Wall : Scroll-aware collision
    // Players are in SCREEN coordinates, Walls are in WORLD coordinates
    // Convert player position to world coordinates using m_currentScroll
    auto& positions = registry.get_components<Position>();
    auto& colliders = registry.get_components<Collider>();
    auto& controllables = registry.get_components<Controllable>();
    auto& walls = registry.get_components<Wall>();

    for (size_t i = 0; i < controllables.size(); i++) {
        Entity player = controllables.get_entity_at(i);

        if (!positions.has_entity(player) || !colliders.has_entity(player))
            continue;

        Position& posP = positions[player];
        const Collider& colP = colliders[player];

        // Convert player SCREEN position to WORLD position
        float player_world_x = posP.x + m_currentScroll;

        // Player hitbox in world coordinates (center-based)
        float half_pw = colP.width * 0.5f;
        float half_ph = colP.height * 0.5f;
        float p_left = player_world_x - half_pw;
        float p_right = player_world_x + half_pw;
        float p_top = posP.y - half_ph;
        float p_bottom = posP.y + half_ph;

        for (size_t j = 0; j < walls.size(); j++) {
            Entity wall = walls.get_entity_at(j);

            if (!positions.has_entity(wall) || !colliders.has_entity(wall))
                continue;

            const Position& posW = positions[wall];
            const Collider& colW = colliders[wall];

            // Wall hitbox in world coordinates (center-based)
            float half_ww = colW.width * 0.5f;
            float half_wh = colW.height * 0.5f;
            float w_left = posW.x - half_ww;
            float w_right = posW.x + half_ww;
            float w_top = posW.y - half_wh;
            float w_bottom = posW.y + half_wh;

            // AABB collision check
            if (p_right > w_left && p_left < w_right && p_bottom > w_top && p_top < w_bottom) {
                // Calculate penetration depths for each side
                float pen_left = p_right - w_left;
                float pen_right = w_right - p_left;
                float pen_top = p_bottom - w_top;
                float pen_bottom = w_bottom - p_top;

                // Find minimum penetration
                float min_pen = pen_left;
                int push_dir = 0; // 0=left, 1=right, 2=up, 3=down

                if (pen_right < min_pen) { min_pen = pen_right; push_dir = 1; }
                if (pen_top < min_pen) { min_pen = pen_top; push_dir = 2; }
                if (pen_bottom < min_pen) { min_pen = pen_bottom; push_dir = 3; }

                // Push player out (in SCREEN coordinates, so don't modify world X directly)
                switch (push_dir) {
                    case 0: posP.x -= min_pen; break; // Push left
                    case 1: posP.x += min_pen; break; // Push right
                    case 2: posP.y -= min_pen; break; // Push up
                    case 3: posP.y += min_pen; break; // Push down
                }
            }
        }
    }

    // Collision Projectile vs Wall : Scroll-aware collision
    // Projectiles are in SCREEN coordinates, Walls are in WORLD coordinates
    for (size_t i = 0; i < projectiles.size(); i++) {
        Entity bullet = projectiles.get_entity_at(i);

        // Only player projectiles collide with walls
        if (projectiles[bullet].faction != ProjectileFaction::Player)
            continue;

        if (!positions.has_entity(bullet) || !colliders.has_entity(bullet))
            continue;

        const Position& posB = positions[bullet];
        const Collider& colB = colliders[bullet];

        // Convert bullet SCREEN position to WORLD position
        float bullet_world_x = posB.x + m_currentScroll;

        // Bullet hitbox in world coordinates
        float half_bw = colB.width * 0.5f;
        float half_bh = colB.height * 0.5f;
        float b_left = bullet_world_x - half_bw;
        float b_right = bullet_world_x + half_bw;
        float b_top = posB.y - half_bh;
        float b_bottom = posB.y + half_bh;

        for (size_t j = 0; j < walls.size(); j++) {
            Entity wall = walls.get_entity_at(j);

            if (!positions.has_entity(wall) || !colliders.has_entity(wall))
                continue;

            const Position& posW = positions[wall];
            const Collider& colW = colliders[wall];

            // Wall hitbox in world coordinates
            float half_ww = colW.width * 0.5f;
            float half_wh = colW.height * 0.5f;
            float w_left = posW.x - half_ww;
            float w_right = posW.x + half_ww;
            float w_top = posW.y - half_wh;
            float w_bottom = posW.y + half_wh;

            // AABB collision check
            if (b_right > w_left && b_left < w_right && b_bottom > w_top && b_top < w_bottom) {
                registry.add_component(bullet, ToDestroy{});
                hide_projectile_sprite(bullet);
                break; // Bullet hit wall, no need to check more walls
            }
        }
    }
}
