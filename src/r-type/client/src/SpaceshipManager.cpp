/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** SpaceshipManager - Gestion des vaisseaux depuis une spritesheet
*/

#include "SpaceshipManager.hpp"
#include <iostream>

namespace rtype::client {

SpaceshipManager::SpaceshipManager(engine::IGraphicsPlugin& graphics)
    : graphics_(graphics)
    , spritesheet_handle_(engine::INVALID_HANDLE) {
}

bool SpaceshipManager::load_spritesheet(const std::string& path) {
    if (spritesheet_handle_ != engine::INVALID_HANDLE) {
        std::cerr << "[SpaceshipManager] Spritesheet already loaded\n";
        return true;
    }

    spritesheet_handle_ = graphics_.load_texture(path);
    
    if (spritesheet_handle_ == engine::INVALID_HANDLE) {
        std::cerr << "[SpaceshipManager] Failed to load spritesheet: " << path << "\n";
        return false;
    }

    // std::cout << "[SpaceshipManager] Loaded spritesheet: " << path << "\n";
    return true;
}

SpriteRect SpaceshipManager::get_ship_rect(ShipColor color, ShipType type) const {
    // Calcul de la position dans la grille
    // Les types sont en colonnes (SCOUT=0, FIGHTER=1, etc.)
    // Les couleurs sont en lignes (GREEN=0, RED=1, BLUE=2)
    int col = static_cast<int>(type);
    int row = static_cast<int>(color);
    
    return SpriteRect{
        col * SPRITE_WIDTH,
        row * SPRITE_HEIGHT,
        SPRITE_WIDTH,
        SPRITE_HEIGHT
    };
}

engine::Sprite SpaceshipManager::create_ship_sprite(ShipColor color, ShipType type, float scale) const {
    if (!is_loaded()) {
        std::cerr << "[SpaceshipManager] Cannot create sprite: spritesheet not loaded\n";
        return engine::Sprite{};
    }

    // Obtenir le rectangle source
    SpriteRect rect = get_ship_rect(color, type);
    
    // Créer le sprite avec rectangle source pour découper la spritesheet
    engine::Sprite sprite;
    sprite.texture_handle = spritesheet_handle_;
    
    // Définir le rectangle source (zone à découper dans la spritesheet)
    sprite.source_rect.x = static_cast<float>(rect.x);
    sprite.source_rect.y = static_cast<float>(rect.y);
    sprite.source_rect.width = static_cast<float>(rect.width);
    sprite.source_rect.height = static_cast<float>(rect.height);
    
    // Taille de rendu (peut être différente de la taille source avec le scale)
    sprite.size = engine::Vector2f{
        static_cast<float>(rect.width) * scale,
        static_cast<float>(rect.height) * scale
    };
    
    // Centrer l'origine pour une rotation autour du centre
    sprite.origin = engine::Vector2f{
        sprite.size.x / 2.0f,
        sprite.size.y / 2.0f
    };
    
    sprite.rotation = 0.0f;
    sprite.tint = engine::Color::White;

    return sprite;
}

void SpaceshipManager::unload() {
    if (spritesheet_handle_ != engine::INVALID_HANDLE) {
        graphics_.unload_texture(spritesheet_handle_);
        spritesheet_handle_ = engine::INVALID_HANDLE;
        // std::cout << "[SpaceshipManager] Unloaded spritesheet\n";
    }
}

} // namespace rtype::client
