#pragma once

#include "BaseScreen.hpp"
#include "ScreenManager.hpp"
#include <functional>

namespace rtype::client {

/**
 * @brief Main menu screen with Quick Match, Browse Rooms, Create Room, and Quit
 */
class MainMenuScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;

    MainMenuScreen(NetworkClient& network_client, int screen_width, int screen_height);

    void initialize() override;
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) override;
    void draw(engine::IGraphicsPlugin* graphics) override;

    void set_screen_change_callback(ScreenChangeCallback callback) {
        on_screen_change_ = callback;
    }

private:
    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UIButton>> buttons_;
    ScreenChangeCallback on_screen_change_;
    engine::TextureHandle background_texture_ = engine::INVALID_HANDLE;
    bool background_loaded_ = false;

    // Edit mode for button positioning
    bool edit_mode_ = false;  // Set to true to enable edit mode
    int selected_button_ = 0;  // 0=PLAY, 1=QUIT, 2=BROWSE
    float move_speed_ = 1.0f;  // Pixels to move per key press
};

}  // namespace rtype::client
