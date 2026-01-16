#pragma once

#include "screens/BaseScreen.hpp"
#include "ScreenManager.hpp"
#include "ui/UIButton.hpp"
#include "ui/UILabel.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace bagario {

/**
 * @brief Settings screen with audio controls
 */
class SettingsScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;

    SettingsScreen(LocalGameState& game_state, int screen_width, int screen_height);

    void initialize() override;
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) override;
    void draw(engine::IGraphicsPlugin* graphics) override;

    void set_screen_change_callback(ScreenChangeCallback callback) {
        on_screen_change_ = callback;
    }

private:
    void update_volume_labels();
    void update_vsync_label();
    void rebuild_ui();

    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UIButton>> buttons_;
    ScreenChangeCallback on_screen_change_;

    UILabel* music_value_label_ = nullptr;
    UILabel* sfx_value_label_ = nullptr;
    UILabel* vsync_value_label_ = nullptr;
    UIButton* vsync_toggle_btn_ = nullptr;
    bool last_vsync_state_ = true;  // Track changes to apply to graphics
};

}
