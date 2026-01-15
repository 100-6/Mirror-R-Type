#pragma once

#include "screens/BaseScreen.hpp"
#include "ScreenManager.hpp"
#include "ui/UIButton.hpp"
#include "ui/UILabel.hpp"
#include "ui/UITextField.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace bagario {

/**
 * @brief Welcome/landing screen with username input and play button
 */
class WelcomeScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;

    WelcomeScreen(LocalGameState& game_state, int screen_width, int screen_height);

    void initialize() override;
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) override;
    void draw(engine::IGraphicsPlugin* graphics) override;

    void set_screen_change_callback(ScreenChangeCallback callback) {
        on_screen_change_ = callback;
    }

private:
    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UIButton>> buttons_;
    std::unique_ptr<UITextField> username_field_;
    std::unique_ptr<UITextField> ip_field_;
    ScreenChangeCallback on_screen_change_;
};

}  // namespace bagario
