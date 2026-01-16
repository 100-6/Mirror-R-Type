#pragma once

#include "screens/BaseScreen.hpp"
#include "ScreenManager.hpp"
#include "ScreenManager.hpp"
#include "ui/UIButton.hpp"
#include "ui/UILabel.hpp"
#include "ui/UIColorPicker.hpp"
#include "ui/UITextField.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace bagario {

/**
 * @brief Skin customization screen with pattern and color selection
 */
class SkinScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;

    SkinScreen(LocalGameState& game_state, int screen_width, int screen_height);

    void initialize() override;
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) override;
    void draw(engine::IGraphicsPlugin* graphics) override;

    void set_screen_change_callback(ScreenChangeCallback callback) {
        on_screen_change_ = callback;
    }

private:
    void rebuild_ui();
    void update_pattern_buttons();
    void draw_preview(engine::IGraphicsPlugin* graphics);
    void draw_grid_background(engine::IGraphicsPlugin* graphics);
    int get_color_count(SkinPattern pattern);

    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UIButton>> pattern_buttons_;
    std::unique_ptr<UIButton> back_button_;
    
    std::unique_ptr<UILabel> primary_label_;
    std::unique_ptr<UILabel> secondary_label_;
    std::unique_ptr<UILabel> tertiary_label_;

    std::unique_ptr<UIColorPicker> primary_picker_;
    std::unique_ptr<UIColorPicker> secondary_picker_;
    std::unique_ptr<UIColorPicker> tertiary_picker_;

    std::unique_ptr<UITextField> image_input_;
    std::unique_ptr<UIButton> browse_button_;
    engine::TextureHandle preview_texture_ = engine::INVALID_HANDLE;

    ScreenChangeCallback on_screen_change_;
};

}
