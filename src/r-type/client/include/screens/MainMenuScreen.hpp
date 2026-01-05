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
};

}  // namespace rtype::client
