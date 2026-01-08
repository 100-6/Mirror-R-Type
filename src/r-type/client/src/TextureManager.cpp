#include "TextureManager.hpp"
#include "AssetsPaths.hpp"
#include <iostream>
#include <random>

namespace rtype::client {

TextureManager::TextureManager(engine::IGraphicsPlugin& graphics)
    : graphics_(graphics)
    , ship_manager_(std::make_unique<SpaceshipManager>(graphics))
    , background_(engine::INVALID_HANDLE)
    , menu_background_(engine::INVALID_HANDLE)
    , enemy_(engine::INVALID_HANDLE)
    , projectile_(engine::INVALID_HANDLE)
    , wall_(engine::INVALID_HANDLE)
    , shot_frame_1_(engine::INVALID_HANDLE)
    , shot_frame_2_(engine::INVALID_HANDLE)
    , bullet_animation_(engine::INVALID_HANDLE)
    , explosion_(engine::INVALID_HANDLE) {
    player_frames_.fill(engine::INVALID_HANDLE);
}

bool TextureManager::load_all() {
    using namespace assets::paths;

    background_ = graphics_.load_texture(BACKGROUND_PARALLAX);
    menu_background_ = graphics_.load_texture(BACKGROUND_MENU);

    // Fallback to regular background if menu background not found
    if (menu_background_ == engine::INVALID_HANDLE) {
        menu_background_ = background_;
    }

    // Load spaceship spritesheet
    if (!ship_manager_->load_spritesheet(PLAYER_SPRITESHEET)) {
        std::cerr << "[TextureManager] Failed to load Spaceships.png (player sprites unavailable)\n";
    } else {
        // Use spritesheet handle for compatibility
        player_frames_[0] = ship_manager_->get_spritesheet_handle();
        player_frames_[1] = ship_manager_->get_spritesheet_handle();
        player_frames_[2] = ship_manager_->get_spritesheet_handle();
        player_frames_[3] = ship_manager_->get_spritesheet_handle();
    }

    enemy_ = graphics_.load_texture(ENEMY_BASIC);
    wall_ = graphics_.load_texture(WALL);
    
    // Load shot animation spritesheet
    engine::TextureHandle shot_spritesheet = graphics_.load_texture(SHOT_ANIMATION);
    shot_frame_1_ = shot_spritesheet;
    shot_frame_2_ = shot_spritesheet;
    
    // Load bullet animation spritesheet (48x16, 3 frames of 16x16)
    engine::TextureHandle bullet_sheet = graphics_.load_texture(BULLET_ANIMATION);
    bullet_animation_ = bullet_sheet;
    projectile_ = bullet_sheet;
    explosion_ = graphics_.load_texture(EXPLOSION_ANIMATION);

    // Check critical textures
    if (background_ == engine::INVALID_HANDLE ||
        enemy_ == engine::INVALID_HANDLE ||
        wall_ == engine::INVALID_HANDLE ||
        projectile_ == engine::INVALID_HANDLE) {
        std::cerr << "[TextureManager] Failed to load critical textures\n";
        return false;
    }

    if (explosion_ == engine::INVALID_HANDLE) {
        std::cerr << "[TextureManager] Warning: Explosion texture failed to load\n";
    }

    return true;
}

engine::TextureHandle TextureManager::get_player_frame(size_t index) const {
    if (index >= player_frames_.size())
        return engine::INVALID_HANDLE;
    return player_frames_[index];
}

engine::Sprite TextureManager::get_random_ship_sprite(float scale) const {
    if (!ship_manager_->is_loaded()) {
        return engine::Sprite{};
    }

    // Générateur aléatoire
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> color_dist(0, 2);  // 3 couleurs
    static std::uniform_int_distribution<> type_dist(0, 4);   // 5 types

    ShipColor random_color = static_cast<ShipColor>(color_dist(gen));
    ShipType random_type = static_cast<ShipType>(type_dist(gen));

    return ship_manager_->create_ship_sprite(random_color, random_type, scale);
}

engine::Vector2f TextureManager::get_texture_size(engine::TextureHandle handle) const {
    if (handle == engine::INVALID_HANDLE)
        return {0.0f, 0.0f};
    return graphics_.get_texture_size(handle);
}

}
