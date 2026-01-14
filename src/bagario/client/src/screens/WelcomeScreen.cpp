#include "screens/WelcomeScreen.hpp"
#include <iostream>

namespace bagario {

WelcomeScreen::WelcomeScreen(LocalGameState& game_state, int screen_width, int screen_height)
    : BaseScreen(game_state, screen_width, screen_height) {
}

void WelcomeScreen::initialize() {
    labels_.clear();
    buttons_.clear();

    float center_x = screen_width_ / 2.0f;

    // Title - "BAGARIO" with large font
    auto title = std::make_unique<UILabel>(
        center_x, 100.0f, "BAGARIO", 90);
    title->set_color(engine::Color{76, 175, 80, 255});  // Green
    title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(title));

    // Subtitle
    auto subtitle = std::make_unique<UILabel>(
        center_x, 190.0f, "Eat or be eaten!", 30);
    subtitle->set_color(engine::Color{200, 200, 200, 255});
    subtitle->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(subtitle));

    // Username label
    auto username_label = std::make_unique<UILabel>(
        center_x, 280.0f, "Enter your nickname:", 24);
    username_label->set_color(engine::Color{180, 180, 180, 255});
    username_label->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(username_label));

    // Username text field
    float field_width = 350.0f;
    float field_height = 60.0f;
    username_field_ = std::make_unique<UITextField>(
        center_x - field_width / 2.0f, 320.0f, field_width, field_height, "Player");
    username_field_->set_text(game_state_.username);
    username_field_->set_max_length(16);
    username_field_->set_on_change([this](const std::string& text) {
        game_state_.username = text;
    });

    // Play button
    float button_width = 250.0f;
    float button_height = 70.0f;
    auto play_btn = std::make_unique<UIButton>(
        center_x - button_width / 2.0f, 420.0f, button_width, button_height, "PLAY");
    play_btn->set_on_click([this]() {
        std::cout << "[WelcomeScreen] Play clicked! Username: " << game_state_.username << "\n";
        if (on_screen_change_) {
            on_screen_change_(GameScreen::PLAYING);
        }
    });
    buttons_.push_back(std::move(play_btn));

    // Settings button
    float settings_width = 180.0f;
    float settings_height = 55.0f;
    auto settings_btn = std::make_unique<UIButton>(
        center_x - settings_width / 2.0f, 510.0f, settings_width, settings_height, "Settings");
    settings_btn->set_on_click([this]() {
        std::cout << "[WelcomeScreen] Settings clicked\n";
        if (on_screen_change_) {
            on_screen_change_(GameScreen::SETTINGS);
        }
    });
    buttons_.push_back(std::move(settings_btn));

    // Footer/credits
    auto footer = std::make_unique<UILabel>(
        center_x, screen_height_ - 40.0f, "Made with love", 18);
    footer->set_color(engine::Color{100, 100, 100, 200});
    footer->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(footer));
}

void WelcomeScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    // Update username field
    if (username_field_) {
        username_field_->update(graphics, input);
    }

    // Update buttons
    for (auto& button : buttons_) {
        button->update(graphics, input);
    }
}

void WelcomeScreen::draw(engine::IGraphicsPlugin* graphics) {
    // Clear with dark gradient background (Agar.io style)
    graphics->clear(engine::Color{20, 25, 30, 255});

    // Draw a subtle grid pattern (Agar.io style background)
    engine::Color grid_color{40, 45, 55, 100};
    float grid_spacing = 50.0f;

    for (float x = 0; x < screen_width_; x += grid_spacing) {
        engine::Vector2f start{x, 0};
        engine::Vector2f end{x, static_cast<float>(screen_height_)};
        graphics->draw_line(start, end, grid_color, 1.0f);
    }
    for (float y = 0; y < screen_height_; y += grid_spacing) {
        engine::Vector2f start{0, y};
        engine::Vector2f end{static_cast<float>(screen_width_), y};
        graphics->draw_line(start, end, grid_color, 1.0f);
    }

    // Draw decorative circles (like cells in background)
    graphics->draw_circle({100, 150}, 40, engine::Color{76, 175, 80, 30});
    graphics->draw_circle({screen_width_ - 150.0f, 200}, 60, engine::Color{244, 67, 54, 25});
    graphics->draw_circle({200, screen_height_ - 150.0f}, 50, engine::Color{33, 150, 243, 25});
    graphics->draw_circle({screen_width_ - 100.0f, screen_height_ - 100.0f}, 35, engine::Color{255, 193, 7, 30});

    // Draw labels
    for (auto& label : labels_) {
        label->draw(graphics);
    }

    // Draw username field
    if (username_field_) {
        username_field_->draw(graphics);
    }

    // Draw buttons
    for (auto& button : buttons_) {
        button->draw(graphics);
    }
}

}  // namespace bagario
