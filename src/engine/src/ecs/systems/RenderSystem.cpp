/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** RenderSystem
*/

#include "ecs/systems/RenderSystem.hpp"
#include "ecs/Components.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

RenderSystem::RenderSystem(engine::IGraphicsPlugin* plugin)
    : graphics_plugin(plugin)
{
    if (!graphics_plugin) {
        throw std::runtime_error("RenderSystem: plugin cannot be null");
    }
}

void RenderSystem::init(Registry& registry)
{
    std::cout << "RenderSystem: Initialisation avec " << graphics_plugin->get_name() << std::endl;
}

void RenderSystem::shutdown()
{
    std::cout << "RenderSystem: Arrêt" << std::endl;
}

void RenderSystem::update(Registry& registry)
{
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
        graphics_plugin->draw_sprite(temp_sprite, engine::Vector2f(pos.x, pos.y));
    }
}
