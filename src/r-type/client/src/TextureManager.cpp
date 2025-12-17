#include "TextureManager.hpp"
#include <iostream>

namespace rtype::client {

TextureManager::TextureManager(engine::IGraphicsPlugin& graphics)
    : graphics_(graphics)
    , background_(engine::INVALID_HANDLE)
    , menu_background_(engine::INVALID_HANDLE)
    , enemy_(engine::INVALID_HANDLE)
    , projectile_(engine::INVALID_HANDLE)
    , wall_(engine::INVALID_HANDLE) {
    player_frames_.fill(engine::INVALID_HANDLE);
}

bool TextureManager::load_all() {
    background_ = graphics_.load_texture("assets/sprite/symmetry.png");
    menu_background_ = graphics_.load_texture("assets/sprite/background_rtype_menu.png");

    // Fallback to regular background if menu background not found
    if (menu_background_ == engine::INVALID_HANDLE) {
        menu_background_ = background_;
    }

    player_frames_[0] = graphics_.load_texture("assets/sprite/ship1.png");
    player_frames_[1] = graphics_.load_texture("assets/sprite/ship2.png");
    player_frames_[2] = graphics_.load_texture("assets/sprite/ship3.png");
    player_frames_[3] = graphics_.load_texture("assets/sprite/ship4.png");

    enemy_ = graphics_.load_texture("assets/sprite/enemy.png");
    projectile_ = graphics_.load_texture("assets/sprite/bullet.png");
    wall_ = graphics_.load_texture("assets/sprite/lock.png");

    // Check critical textures
    if (background_ == engine::INVALID_HANDLE ||
        player_frames_[0] == engine::INVALID_HANDLE ||
        enemy_ == engine::INVALID_HANDLE ||
        projectile_ == engine::INVALID_HANDLE) {
        std::cerr << "[TextureManager] Failed to load critical textures\n";
        return false;
    }

    return true;
}

engine::TextureHandle TextureManager::get_player_frame(size_t index) const {
    if (index >= player_frames_.size())
        return engine::INVALID_HANDLE;
    return player_frames_[index];
}

engine::Vector2f TextureManager::get_texture_size(engine::TextureHandle handle) const {
    if (handle == engine::INVALID_HANDLE)
        return {0.0f, 0.0f};
    return graphics_.get_texture_size(handle);
}

}
