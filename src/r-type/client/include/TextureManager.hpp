#pragma once

#include <array>
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/CommonTypes.hpp"

namespace rtype::client {

/**
 * @brief Manages all game textures
 */
class TextureManager {
public:
    explicit TextureManager(engine::IGraphicsPlugin& graphics);

    /**
     * @brief Load all game textures
     * @return true if all critical textures loaded successfully
     */
    bool load_all();

    // Texture accessors
    engine::TextureHandle get_background() const { return background_; }
    engine::TextureHandle get_menu_background() const { return menu_background_; }
    engine::TextureHandle get_player_frame(size_t index) const;
    engine::TextureHandle get_enemy() const { return enemy_; }
    engine::TextureHandle get_projectile() const { return projectile_; }
    engine::TextureHandle get_wall() const { return wall_; }

    const std::array<engine::TextureHandle, 4>& get_player_frames() const { return player_frames_; }

private:
    engine::IGraphicsPlugin& graphics_;

    engine::TextureHandle background_;
    engine::TextureHandle menu_background_;
    std::array<engine::TextureHandle, 4> player_frames_;
    engine::TextureHandle enemy_;
    engine::TextureHandle projectile_;
    engine::TextureHandle wall_;
};

}
