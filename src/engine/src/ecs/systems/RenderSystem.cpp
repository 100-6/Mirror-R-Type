/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** RenderSystem
*/

#include "ecs/systems/RenderSystem.hpp"
#include "ecs/CoreComponents.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

RenderSystem::RenderSystem(engine::IGraphicsPlugin& plugin)
    : graphics_plugin(plugin)
{
    // Pas besoin de vérifier null - les références ne peuvent pas être nulles
}

void RenderSystem::init(Registry& registry)
{
    std::cout << "RenderSystem: Initialisation avec " << graphics_plugin.get_name() << std::endl;
}

void RenderSystem::shutdown()
{
    std::cout << "RenderSystem: Arrêt" << std::endl;
}

void RenderSystem::update(Registry& registry, float dt)
{
    (void)dt;

    // Effacer l'écran (fond gris foncé)
    graphics_plugin.clear(engine::Color{30, 30, 40, 255});

    // Récupérer les composants Position et Sprite
    auto& positions = registry.get_components<Position>();
    auto& sprites = registry.get_components<Sprite>();

    // Structure temporaire pour stocker les entités à dessiner
    struct RenderData {
        Entity entity;
        int layer;
    };

    // Collecter toutes les entités avec Position + Sprite
    std::vector<RenderData> render_queue;
    render_queue.reserve(positions.size()); // Pré-allouer pour éviter les réallocations

    for (size_t i = 0; i < positions.size(); i++) {
        Entity entity = positions.get_entity_at(i);

        // Vérifier que l'entité a aussi un Sprite
        if (!sprites.has_entity(entity)) {
            continue;
        }

        const Sprite& sprite = sprites.get_data_by_entity_id(entity);

        // Ignorer les sprites sans texture
        if (sprite.texture == engine::INVALID_HANDLE) {
            continue;
        }

        // Ajouter à la queue de rendu
        render_queue.push_back({entity, sprite.layer});
    }

    // Trier par layer (ordre croissant = fond en premier)
    std::sort(render_queue.begin(), render_queue.end(),
        [](const RenderData& a, const RenderData& b) {
            return a.layer < b.layer;
        });

    // Dessiner toutes les entités dans l'ordre des layers
    for (const auto& render_data : render_queue) {
        const Position& pos = positions.get_data_by_entity_id(render_data.entity);
        const Sprite& sprite = sprites.get_data_by_entity_id(render_data.entity);

        // Préparer le sprite pour le plugin (réutilise temp_sprite pour éviter allocations)
        temp_sprite.texture_handle = sprite.texture;
        temp_sprite.size = engine::Vector2f(sprite.width, sprite.height);
        temp_sprite.origin = engine::Vector2f(sprite.origin_x, sprite.origin_y);
        temp_sprite.rotation = sprite.rotation;
        temp_sprite.tint = sprite.tint;

        // Dessiner le sprite à la position de l'entité
        graphics_plugin.draw_sprite(temp_sprite, engine::Vector2f(pos.x, pos.y));
    }

    // === RENDU DES CERCLES (CircleEffect) ===
    if (registry.has_component_registered<CircleEffect>()) {
        auto& circles = registry.get_components<CircleEffect>();
        auto& colliders = registry.get_components<Collider>();

        for (size_t i = 0; i < circles.size(); i++) {
            Entity entity = circles.get_entity_at(i);
            const CircleEffect& circle = circles[entity];

            if (!circle.active)
                continue;
            if (!positions.has_entity(entity))
                continue;

            const Position& pos = positions[entity];

            // Calculer le centre (avec offset si collider présent)
            float centerX = pos.x + circle.offsetX;
            float centerY = pos.y + circle.offsetY;

            if (colliders.has_entity(entity)) {
                const Collider& col = colliders[entity];
                centerX = pos.x + col.width / 2.0f + circle.offsetX;
                centerY = pos.y + col.height / 2.0f + circle.offsetY;
            }

            graphics_plugin.draw_circle(engine::Vector2f{centerX, centerY}, circle.radius, circle.color);
        }
    }

    // === RENDU DES TEXTES (TextEffect) ===
    if (registry.has_component_registered<TextEffect>()) {
        auto& texts = registry.get_components<TextEffect>();

        for (size_t i = 0; i < texts.size(); i++) {
            Entity entity = texts.get_entity_at(i);
            const TextEffect& text = texts[entity];

            if (!text.active || text.text.empty())
                continue;

            graphics_plugin.draw_text(text.text, engine::Vector2f{text.posX, text.posY},
                                      text.color, engine::INVALID_HANDLE, text.fontSize);
        }
    }

    // Note: On n'appelle PAS display() ici pour permettre au main d'ajouter
    // des éléments UI/debug par-dessus avant d'afficher le frame
}
