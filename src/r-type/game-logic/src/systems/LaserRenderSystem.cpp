/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LaserRenderSystem - Renders laser beams using draw_line
*/

#include "systems/LaserRenderSystem.hpp"
#include "components/CombatConfig.hpp"
#include <iostream>
#include <cmath>
#include "components/LevelComponents.hpp" // For Wall
#include "ecs/CoreComponents.hpp"      // For Collider, Position, etc.

void LaserRenderSystem::init(Registry& registry)
{
    std::cout << "LaserRenderSystem: Initialisation." << std::endl;
}

void LaserRenderSystem::shutdown()
{
    std::cout << "LaserRenderSystem: Arrêt." << std::endl;
}

static void performVisualRaycast(Registry& registry, float startX, float startY, float range, Entity shooter, LaserBeam& beam)
{
    auto& positions = registry.get_components<Position>();
    auto& colliders = registry.get_components<Collider>();
    // Note: On client, we might not have 'Enemy' component explicitly synchronized if it is just EntityType in EntityManager
    // But ClientGame registers Enemy component, so it might be there if EntityManager adds it.
    auto& enemies = registry.get_components<Enemy>(); 
    auto& walls = registry.get_components<Wall>();

    // Point final par défaut (pas de collision)
    beam.hit_x = startX + range;
    beam.hit_y = startY;
    beam.current_length = range;

    float closest_hit = range;

    // Vérifier la collision avec les ennemis (si disponibles client-side)
    if (registry.has_component_registered<Enemy>()) {
        for (size_t i = 0; i < enemies.size(); i++) {
            Entity enemy = enemies.get_entity_at(i);
            if (!positions.has_entity(enemy) || !colliders.has_entity(enemy))
                continue;

            const Position& enemyPos = positions[enemy];
            const Collider& enemyCol = colliders[enemy];

            // Intersection AABB avec une ligne horizontale
            float half_w = enemyCol.width * 0.5f;
            float half_h = enemyCol.height * 0.5f;

            float left = enemyPos.x - half_w;
            float top = enemyPos.y - half_h;
            float bottom = enemyPos.y + half_h;

            // Le rayon Y est-il dans les limites de l'ennemi et l'ennemi est-il devant ?
            if (startY >= top && startY <= bottom && left > startX && left < startX + range) {
                float hit_dist = left - startX;
                if (hit_dist < closest_hit) {
                    closest_hit = hit_dist;
                }
            }
        }
    }

    // Vérifier la collision avec les murs
    if (registry.has_component_registered<Wall>()) {
        for (size_t i = 0; i < walls.size(); i++) {
            Entity wall = walls.get_entity_at(i);
            if (!positions.has_entity(wall) || !colliders.has_entity(wall))
                continue;

            const Position& wallPos = positions[wall];
            const Collider& wallCol = colliders[wall];

            float half_w = wallCol.width * 0.5f;
            float half_h = wallCol.height * 0.5f;

            float left = wallPos.x - half_w;
            float top = wallPos.y - half_h;
            float bottom = wallPos.y + half_h;

            if (startY >= top && startY <= bottom && left > startX && left < startX + range) {
                float hit_dist = left - startX;
                if (hit_dist < closest_hit) {
                    closest_hit = hit_dist;
                }
            }
        }
    }

    beam.current_length = closest_hit;
    beam.hit_x = startX + closest_hit;
    beam.hit_y = startY;
}

void LaserRenderSystem::update(Registry& registry, float dt)
{
    if (!graphics_)
        return;

    // Accumuler le temps pour l'animation
    animation_time_ += dt;

    // Vérifier si le composant LaserBeam est enregistré
    if (!registry.has_component_registered<LaserBeam>())
        return;

    auto& lasers = registry.get_components<LaserBeam>();
    auto& positions = registry.get_components<Position>();
    auto& sprites = registry.get_components<Sprite>();

    for (size_t i = 0; i < lasers.size(); i++) {
        Entity entity = lasers.get_entity_at(i);
        LaserBeam& laser = lasers[entity]; // Non-const ref to update hit_x/hit_y

        if (!laser.active)
            continue;
        if (!positions.has_entity(entity))
            continue;

        const Position& pos = positions[entity];

        // Calculer les dimensions du joueur pour le point de départ
        float playerWidth = sprites.has_entity(entity) ? sprites[entity].width : 0.0f;
        float playerHeight = sprites.has_entity(entity) ? sprites[entity].height : 32.0f;

        // Point de départ du laser (au bout du vaisseau, centré verticalement)
        float startX = pos.x + playerWidth / 2.0f;  // Plus proche du vaisseau
        float startY = pos.y;  // Centré sur le vaisseau
        
        // Raycast visuel côté client pour déterminer la longueur
        performVisualRaycast(registry, startX, startY, laser.range, entity, laser);

        engine::Vector2f start{startX, startY};
        engine::Vector2f end{laser.hit_x, laser.hit_y};

        // Animation: pulsation de la largeur et de l'opacité
        float pulse = 0.8f + 0.4f * std::sin(animation_time_ * 15.0f);  // Pulsation rapide
        float flicker = 0.9f + 0.2f * std::sin(animation_time_ * 50.0f);  // Scintillement rapide
        float animatedWidth = laser.width * pulse;

        // Glow externe (additive blending, plus large et transparent)
        engine::Color glowColor = laser.beam_color;
        glowColor.a = static_cast<uint8_t>(80 * flicker);
        graphics_->begin_blend_mode(1);  // ADDITIVE
        graphics_->draw_line(start, end, glowColor, animatedWidth * 3.0f);
        graphics_->end_blend_mode();

        // Second glow layer for more intensity
        engine::Color innerGlow = laser.beam_color;
        innerGlow.a = static_cast<uint8_t>(150 * flicker);
        graphics_->begin_blend_mode(1);
        graphics_->draw_line(start, end, innerGlow, animatedWidth * 1.8f);
        graphics_->end_blend_mode();

        // Rayon principal
        graphics_->draw_line(start, end, laser.beam_color, animatedWidth);

        // Core brillant (centre du laser)
        engine::Color coreColor = laser.core_color;
        coreColor.a = static_cast<uint8_t>(255 * flicker);
        graphics_->draw_line(start, end, coreColor, animatedWidth * 0.5f);

        // Effet d'impact si le laser touche quelque chose avant sa portée max
        if (laser.current_length < laser.range - 1.0f) {
            float impactPulse = 1.0f + 0.5f * std::sin(animation_time_ * 20.0f);
            graphics_->begin_blend_mode(1);  // ADDITIVE
            graphics_->draw_circle(end, animatedWidth * 2.5f * impactPulse, laser.core_color);
            graphics_->draw_circle(end, animatedWidth * 1.5f * impactPulse, engine::Color{255, 255, 255, 200});
            graphics_->end_blend_mode();
        }
    }
}

