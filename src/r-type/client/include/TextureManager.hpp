#pragma once

#include <array>
#include <memory>
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/CommonTypes.hpp"
#include "SpaceshipManager.hpp"

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
    engine::TextureHandle get_shot_frame_1() const { return shot_frame_1_; }
    engine::TextureHandle get_shot_frame_2() const { return shot_frame_2_; }
    engine::TextureHandle get_bullet_animation() const { return bullet_animation_; }
    engine::TextureHandle get_explosion() const { return explosion_; }

    const std::array<engine::TextureHandle, 4>& get_player_frames() const { return player_frames_; }

    /**
     * @brief Get a random spaceship sprite from the spritesheet
     * @param scale Scale factor for the sprite
     * @return Configured sprite with random ship
     */
    engine::Sprite get_random_ship_sprite(float scale = 1.0f) const;

    /**
     * @brief Get spaceship manager
     */
    SpaceshipManager& get_ship_manager() { return *ship_manager_; }
    const SpaceshipManager& get_ship_manager() const { return *ship_manager_; }

    engine::Vector2f get_texture_size(engine::TextureHandle handle) const;

private:
    engine::IGraphicsPlugin& graphics_;

    // Spaceship manager (spritesheet)
    std::unique_ptr<SpaceshipManager> ship_manager_;

    engine::TextureHandle background_;
    engine::TextureHandle menu_background_;
    std::array<engine::TextureHandle, 4> player_frames_;
    engine::TextureHandle enemy_;
    engine::TextureHandle projectile_;
    engine::TextureHandle wall_;
    engine::TextureHandle shot_frame_1_;
    engine::TextureHandle shot_frame_2_;
    engine::TextureHandle bullet_animation_;
    engine::TextureHandle explosion_;
};

}
