#pragma once

#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "components/GameComponents.hpp"
#include "plugin_manager/CommonTypes.hpp"

namespace rtype::client {

/**
 * @brief Game screen states
 */
enum class GameScreen {
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
                 engine::TextureHandle background_tex, engine::TextureHandle menu_bg_tex);

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
     */
    void show_result(bool victory);

    /**
     * @brief Get status overlay entity
     */
    Entity get_status_entity() const { return status_entity_; }

private:
    Registry& registry_;
    int screen_width_;
    int screen_height_;
    engine::TextureHandle background_texture_;
    engine::TextureHandle menu_background_texture_;

    GameScreen current_screen_;

    // Screen entities
    Entity waiting_screen_bg_;
    Entity waiting_screen_text_;
    Entity result_screen_bg_;
    Entity result_screen_text_;
    Entity status_entity_;

    void hide_waiting_screen();
    void show_waiting_screen();
};

}
