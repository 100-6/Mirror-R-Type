#pragma once

#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "LocalGameState.hpp"
#include <memory>
#include <functional>

namespace bagario {

class BaseScreen;

namespace client {
    class NetworkManager;
}

/**
 * @brief Game screen states
 */
enum class GameScreen {
    WELCOME,
    SETTINGS,
    SKIN,
    PLAYING
};

/**
 * @brief Manages screen transitions
 */
class ScreenManager {
public:
    ScreenManager(LocalGameState& game_state, int screen_width, int screen_height);
    ~ScreenManager();

    void initialize();
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void draw(engine::IGraphicsPlugin* graphics);

    GameScreen get_current_screen() const { return current_screen_; }
    void set_screen(GameScreen screen);

    /**
     * @brief Set the network manager for the playing screen
     */
    void set_network_manager(client::NetworkManager* network);

private:
    LocalGameState& game_state_;
    int screen_width_;
    int screen_height_;
    GameScreen current_screen_ = GameScreen::WELCOME;

    std::unique_ptr<BaseScreen> welcome_screen_;
    std::unique_ptr<BaseScreen> settings_screen_;
    std::unique_ptr<BaseScreen> skin_screen_;
    std::unique_ptr<BaseScreen> playing_screen_;

    client::NetworkManager* network_manager_ = nullptr;

    void handle_screen_change(GameScreen new_screen);
};

}
