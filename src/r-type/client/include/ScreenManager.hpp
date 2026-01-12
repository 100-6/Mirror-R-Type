#pragma once

#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "components/GameComponents.hpp"
#include "plugin_manager/CommonTypes.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "ui/UIButton.hpp"
#include <memory>
#include <functional>

namespace rtype::client {

/**
 * @brief Game screen states
 */
enum class GameScreen {
    MAIN_MENU,
    CREATE_ROOM,
    BROWSE_ROOMS,
    ROOM_LOBBY,
    WAITING,
    PLAYING,
    VICTORY,
    DEFEAT
};

/**
 * @brief Manages game screen transitions and UI overlays
 */
class ScreenManager {
public:
    ScreenManager(Registry& registry, int screen_width, int screen_height,
                 engine::TextureHandle background_tex, engine::TextureHandle menu_bg_tex,
                 engine::IGraphicsPlugin* graphics_plugin);

    /**
     * @brief Initialize all screen entities
     */
    void initialize();

    /**
     * @brief Get current screen state
     */
    GameScreen get_current_screen() const { return current_screen_; }

    /**
     * @brief Transition to a new screen
     */
    void set_screen(GameScreen screen);

    /**
     * @brief Show result screen (victory or defeat)
     * @param victory True for victory, false for defeat
     * @param final_score Final score to display
     */
    void show_result(bool victory, int final_score = 0);

    /**
     * @brief Update result screen (handle button clicks)
     */
    void update_result_screen(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);

    /**
     * @brief Draw result screen button
     */
    void draw_result_screen(engine::IGraphicsPlugin* graphics);

    /**
     * @brief Get status overlay entity
     */
    Entity get_status_entity() const { return status_entity_; }

    /**
     * @brief Set callback for back to menu button
     */
    void set_back_to_menu_callback(std::function<void()> callback) { back_to_menu_callback_ = callback; }

private:
    Registry& registry_;
    int screen_width_;
    int screen_height_;
    engine::TextureHandle background_texture_;
    engine::TextureHandle menu_background_texture_;
    engine::IGraphicsPlugin* graphics_plugin_;

    GameScreen current_screen_;

    // Screen entities
    Entity waiting_screen_bg_;
    Entity waiting_screen_text_;
    Entity result_screen_bg_;
    Entity status_entity_;

    // Result screen textures
    engine::TextureHandle win_background_texture_;
    engine::TextureHandle loose_background_texture_;
    bool result_textures_loaded_ = false;

    // Result screen button
    std::unique_ptr<rtype::UIButton> back_to_menu_button_;
    std::function<void()> back_to_menu_callback_;

    // Result screen score display
    int final_score_ = 0;

    void hide_waiting_screen();
    void show_waiting_screen();
    void load_result_textures();
};

}
