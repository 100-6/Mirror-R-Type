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

    // Effacer l'écran (fond gris foncé) - skip if cleared externally
    if (!skip_clear_) {
        graphics_plugin.clear(engine::Color{30, 30, 40, 255});
    }

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

        // Transmettre le rectangle source pour le découpage (spritesheet)
        temp_sprite.source_rect.x = sprite.source_rect.x;
        temp_sprite.source_rect.y = sprite.source_rect.y;
        temp_sprite.source_rect.width = sprite.source_rect.width;
        temp_sprite.source_rect.height = sprite.source_rect.height;

        // Dessiner le sprite à la position de l'entité
        graphics_plugin.draw_sprite(temp_sprite, engine::Vector2f(pos.x, pos.y));

        // Si l'entité a un FlashOverlay, redessiner le sprite en blanc avec blend additif
        if (registry.has_component_registered<FlashOverlay>()) {
            auto& overlays = registry.get_components<FlashOverlay>();
            if (overlays.has_entity(render_data.entity)) {
                const FlashOverlay& overlay = overlays[render_data.entity];

                // Calculer l'alpha basé sur le temps restant (plus lumineux au début)
                float progress = overlay.time_remaining / overlay.total_duration;
                uint8_t flash_alpha = static_cast<uint8_t>(overlay.max_alpha * progress);

                // Activer le blend mode additif pour ajouter du blanc aux pixels
                graphics_plugin.begin_blend_mode(1); // 1 = ADDITIVE

                // Redessiner le sprite avec un tint blanc semi-transparent
                temp_sprite.tint = engine::Color{255, 255, 255, flash_alpha};
                graphics_plugin.draw_sprite(temp_sprite, engine::Vector2f(pos.x, pos.y));

                // Désactiver le blend mode additif
                graphics_plugin.end_blend_mode();

                // Restaurer le tint original pour le prochain rendu
                temp_sprite.tint = sprite.tint;
            }
        }
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

            // Position is already center-based, just apply offset
            float centerX = pos.x + circle.offsetX;
            float centerY = pos.y + circle.offsetY;

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

    // === RENDU DES UI PANELS ===
    if (registry.has_component_registered<UIPanel>()) {
        auto& panels = registry.get_components<UIPanel>();

        for (size_t i = 0; i < panels.size(); i++) {
            Entity entity = panels.get_entity_at(i);
            const UIPanel& panel = panels[entity];

            if (!panel.active || !positions.has_entity(entity))
                continue;

            const Position& pos = positions[entity];

            // Background
            engine::Rectangle bg = {pos.x, pos.y, panel.width, panel.height};
            graphics_plugin.draw_rectangle(bg, panel.backgroundColor);

            // Border
            engine::Rectangle border = {pos.x, pos.y, panel.width, panel.height};
            graphics_plugin.draw_rectangle_outline(border, panel.borderColor, panel.borderThickness);
        }
    }

    // === RENDU DES UI BARS ===
    if (registry.has_component_registered<UIBar>()) {
        auto& bars = registry.get_components<UIBar>();

        for (size_t i = 0; i < bars.size(); i++) {
            Entity entity = bars.get_entity_at(i);
            const UIBar& bar = bars[entity];

            if (!bar.active || !positions.has_entity(entity))
                continue;

            const Position& pos = positions[entity];

            // Background
            engine::Rectangle barBg = {pos.x, pos.y, bar.width, bar.height};
            graphics_plugin.draw_rectangle(barBg, bar.backgroundColor);

            // Fill based on current/max value
            float fillPercent = bar.currentValue / bar.maxValue;
            if (fillPercent < 0.0f) fillPercent = 0.0f;
            if (fillPercent > 1.0f) fillPercent = 1.0f;

            float fillWidth = bar.width * fillPercent;
            engine::Rectangle barFill = {pos.x, pos.y, fillWidth, bar.height};
            graphics_plugin.draw_rectangle(barFill, bar.fillColor);

            // Border
            engine::Rectangle barOutline = {pos.x, pos.y, bar.width, bar.height};
            graphics_plugin.draw_rectangle_outline(barOutline, bar.borderColor, bar.borderThickness);
        }
    }

    // === RENDU DES UI TEXT ===
    if (registry.has_component_registered<UIText>()) {
        auto& uitexts = registry.get_components<UIText>();

        for (size_t i = 0; i < uitexts.size(); i++) {
            Entity entity = uitexts.get_entity_at(i);
            const UIText& uitext = uitexts[entity];

            if (!uitext.active || uitext.text.empty() || !positions.has_entity(entity))
                continue;

            const Position& pos = positions[entity];

            // Shadow
            if (uitext.hasShadow) {
                graphics_plugin.draw_text(uitext.text,
                    engine::Vector2f{pos.x + uitext.shadowOffsetX, pos.y + uitext.shadowOffsetY},
                    uitext.shadowColor, engine::INVALID_HANDLE, uitext.fontSize);
            }

            // Text
            graphics_plugin.draw_text(uitext.text, engine::Vector2f{pos.x, pos.y},
                                      uitext.color, engine::INVALID_HANDLE, uitext.fontSize);
        }
    }

    // Note: On n'appelle PAS display() ici pour permettre au main d'ajouter
    // des éléments UI/debug par-dessus avant d'afficher le frame
}
