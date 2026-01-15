#include "screens/SettingsScreen.hpp"
#include <iostream>

namespace bagario {

SettingsScreen::SettingsScreen(LocalGameState& game_state, int screen_width, int screen_height)
    : BaseScreen(game_state, screen_width, screen_height) {
}

void SettingsScreen::initialize() {
    rebuild_ui();
}

void SettingsScreen::rebuild_ui() {
    labels_.clear();
    buttons_.clear();
    music_value_label_ = nullptr;
    sfx_value_label_ = nullptr;

    float center_x = screen_width_ / 2.0f;
    float center_y = screen_height_ / 2.0f;
    float start_y = center_y - 150.0f;

    // Title
    auto title = std::make_unique<UILabel>(
        center_x, start_y, "SETTINGS", 60);
    title->set_color(engine::Color{76, 175, 80, 255});
    title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(title));

    // Audio section title
    auto audio_title = std::make_unique<UILabel>(
        center_x, start_y + 100.0f, "Audio", 36);
    audio_title->set_color(engine::Color{200, 200, 200, 255});
    audio_title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(audio_title));

    // Music Volume
    float music_y = start_y + 170.0f;
    auto music_label = std::make_unique<UILabel>(
        center_x - 200.0f, music_y, "Music Volume", 28);
    music_label->set_color(engine::Color{255, 255, 255, 255});
    labels_.push_back(std::move(music_label));

    auto music_minus = std::make_unique<UIButton>(
        center_x + 50.0f, music_y - 15.0f, 50.0f, 50.0f, "-");
    music_minus->set_on_click([this]() {
        if (game_state_.music_volume > 0) {
            game_state_.music_volume -= 10;
            if (game_state_.music_volume < 0) game_state_.music_volume = 0;
            update_volume_labels();
        }
    });
    buttons_.push_back(std::move(music_minus));

    auto music_value = std::make_unique<UILabel>(
        center_x + 130.0f, music_y, std::to_string(game_state_.music_volume) + "%", 28);
    music_value->set_color(engine::Color{129, 199, 132, 255});
    music_value_label_ = music_value.get();
    labels_.push_back(std::move(music_value));

    auto music_plus = std::make_unique<UIButton>(
        center_x + 190.0f, music_y - 15.0f, 50.0f, 50.0f, "+");
    music_plus->set_on_click([this]() {
        if (game_state_.music_volume < 100) {
            game_state_.music_volume += 10;
            if (game_state_.music_volume > 100) game_state_.music_volume = 100;
            update_volume_labels();
        }
    });
    buttons_.push_back(std::move(music_plus));

    // SFX Volume
    float sfx_y = start_y + 250.0f;
    auto sfx_label = std::make_unique<UILabel>(
        center_x - 200.0f, sfx_y, "SFX Volume", 28);
    sfx_label->set_color(engine::Color{255, 255, 255, 255});
    labels_.push_back(std::move(sfx_label));

    auto sfx_minus = std::make_unique<UIButton>(
        center_x + 50.0f, sfx_y - 15.0f, 50.0f, 50.0f, "-");
    sfx_minus->set_on_click([this]() {
        if (game_state_.sfx_volume > 0) {
            game_state_.sfx_volume -= 10;
            if (game_state_.sfx_volume < 0) game_state_.sfx_volume = 0;
            update_volume_labels();
        }
    });
    buttons_.push_back(std::move(sfx_minus));

    auto sfx_value = std::make_unique<UILabel>(
        center_x + 130.0f, sfx_y, std::to_string(game_state_.sfx_volume) + "%", 28);
    sfx_value->set_color(engine::Color{129, 199, 132, 255});
    sfx_value_label_ = sfx_value.get();
    labels_.push_back(std::move(sfx_value));

    auto sfx_plus = std::make_unique<UIButton>(
        center_x + 190.0f, sfx_y - 15.0f, 50.0f, 50.0f, "+");
    sfx_plus->set_on_click([this]() {
        if (game_state_.sfx_volume < 100) {
            game_state_.sfx_volume += 10;
            if (game_state_.sfx_volume > 100) game_state_.sfx_volume = 100;
            update_volume_labels();
        }
    });
    buttons_.push_back(std::move(sfx_plus));

    // Back button
    float button_width = 200.0f;
    float button_height = 60.0f;
    auto back_btn = std::make_unique<UIButton>(
        center_x - button_width / 2.0f, screen_height_ - 120.0f, button_width, button_height, "Back");
    back_btn->set_on_click([this]() {
        if (on_screen_change_) {
            on_screen_change_(GameScreen::WELCOME);
        }
    });
    buttons_.push_back(std::move(back_btn));
}

void SettingsScreen::update_volume_labels() {
    if (music_value_label_) {
        music_value_label_->set_text(std::to_string(game_state_.music_volume) + "%");
    }
    if (sfx_value_label_) {
        sfx_value_label_->set_text(std::to_string(game_state_.sfx_volume) + "%");
    }
}

void SettingsScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    for (auto& button : buttons_) {
        button->update(graphics, input);
    }
}

void SettingsScreen::draw(engine::IGraphicsPlugin* graphics) {
    // Dark background
    graphics->clear(engine::Color{20, 25, 30, 255});

    // Grid pattern
    engine::Color grid_color{40, 45, 55, 100};
    float grid_spacing = 50.0f;

    for (float x = 0; x < screen_width_; x += grid_spacing) {
        graphics->draw_line({x, 0}, {x, static_cast<float>(screen_height_)}, grid_color, 1.0f);
    }
    for (float y = 0; y < screen_height_; y += grid_spacing) {
        graphics->draw_line({0, y}, {static_cast<float>(screen_width_), y}, grid_color, 1.0f);
    }

    // Draw labels
    for (auto& label : labels_) {
        label->draw(graphics);
    }

    // Draw buttons
    for (auto& button : buttons_) {
        button->draw(graphics);
    }
}

}  // namespace bagario
