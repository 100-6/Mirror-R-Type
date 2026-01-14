#pragma once

#include "plugin_manager/IGraphicsPlugin.hpp"
#include "protocol/Payloads.hpp"
#include <vector>



namespace rtype::client::createroom {

/**
 * @brief Texture pack for CreateRoomScreen
 */
struct TexturePack {
    // Background texture
    engine::TextureHandle menu_background;

    // Map textures
    engine::TextureHandle map_nebula;
    engine::TextureHandle map_asteroid;
    engine::TextureHandle map_bydo;

    // Difficulty textures
    engine::TextureHandle difficulty_easy;
    engine::TextureHandle difficulty_medium;
    engine::TextureHandle difficulty_hard;

    // Game mode textures
    engine::TextureHandle gamemode_duo;
    engine::TextureHandle gamemode_trio;
    engine::TextureHandle gamemode_squad;

    bool loaded = false;

    /**
     * @brief Load all textures
     */
    void load(engine::IGraphicsPlugin* graphics);
};

/**
 * @brief Handles all rendering logic for CreateRoomScreen
 */
class Renderer {
public:
    /**
     * @brief Draw menu background image
     */
    static void draw_background(
        engine::IGraphicsPlugin* graphics,
        const TexturePack& textures,
        int screen_width,
        int screen_height
    );

    /**
     * @brief Draw step progress indicator
     */
    static void draw_stepper(
        engine::IGraphicsPlugin* graphics,
        int screen_width,
        int current_step_index,
        const char* step_title,
        int total_steps
    );



    /**
     * @brief Draw circular map selection images with dynamic thumbnails
     */
    static void draw_map_selection(
        engine::IGraphicsPlugin* graphics,
        const std::vector<engine::TextureHandle>& map_thumbnails,
        int screen_width,
        size_t selected_map_index
    );

    /**
     * @brief Draw circular difficulty selection images
     */
    static void draw_difficulty_selection(
        engine::IGraphicsPlugin* graphics,
        const TexturePack& textures,
        int screen_width,
        protocol::Difficulty selected_difficulty
    );

    /**
     * @brief Draw circular game mode selection images
     */
    static void draw_gamemode_selection(
        engine::IGraphicsPlugin* graphics,
        const TexturePack& textures,
        int screen_width,
        protocol::GameMode selected_mode
    );

    /**
     * @brief Draw circular navigation buttons (Previous/Next) with arrows
     */
    static void draw_navigation_buttons(
        engine::IGraphicsPlugin* graphics,
        int screen_width,
        int screen_height,
        bool show_previous,
        bool show_next,
        bool previous_hovered,
        bool next_hovered,
        float button_radius,
        float button_spacing,
        float button_y_offset
    );

private:
    /**
     * @brief Draw a circular image with border, shadow, and glow effects
     */
    static void draw_circular_image(
        engine::IGraphicsPlugin* graphics,
        engine::TextureHandle texture,
        float x, float y,
        float size,
        bool is_selected
    );
};

}  // namespace rtype::client::createroom
