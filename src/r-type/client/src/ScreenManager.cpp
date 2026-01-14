#include "ScreenManager.hpp"
#include <string>

namespace rtype::client {

ScreenManager::ScreenManager(Registry& registry, int screen_width, int screen_height,
                             engine::TextureHandle background_tex, engine::TextureHandle menu_bg_tex,
                             engine::IGraphicsPlugin* graphics_plugin)
    : registry_(registry)
    , screen_width_(screen_width)
    , screen_height_(screen_height)
    , background_texture_(background_tex)
    , menu_background_texture_(menu_bg_tex)
    , graphics_plugin_(graphics_plugin)
    , current_screen_(GameScreen::WAITING) {
}

void ScreenManager::initialize() {
    // Status overlay (always visible)
    status_entity_ = registry_.spawn_entity();
    registry_.add_component(status_entity_, Position{30.0f, 30.0f});
    registry_.add_component(status_entity_, UIText{
        "Connecting...",
        engine::Color::White,
        engine::Color{0, 0, 0, 180},
        22,
        true,
        2.0f,
        2.0f,
        true,
        102
    });

    // Waiting screen background
    waiting_screen_bg_ = registry_.spawn_entity();
    registry_.add_component(waiting_screen_bg_, Position{0.0f, 0.0f});
    registry_.add_component(waiting_screen_bg_, Sprite{
        menu_background_texture_,
        static_cast<float>(screen_width_),
        static_cast<float>(screen_height_),
        0.0f,
        engine::Color::White,
        0.0f,
        0.0f,
        200
    });

    // Waiting screen text
    waiting_screen_text_ = registry_.spawn_entity();
    registry_.add_component(waiting_screen_text_, Position{
        static_cast<float>(screen_width_) / 2.0f - 200.0f,
        static_cast<float>(screen_height_) / 2.0f - 50.0f
    });
    registry_.add_component(waiting_screen_text_, UIText{
        "En attente de joueurs...",
        engine::Color::White,
        engine::Color{0, 0, 0, 200},
        36,
        true,
        4.0f,
        4.0f,
        true,
        201
    });

    // Result screen background (hidden initially)
    result_screen_bg_ = registry_.spawn_entity();
    registry_.add_component(result_screen_bg_, Position{0.0f, 0.0f});
    registry_.add_component(result_screen_bg_, Sprite{
        menu_background_texture_,
        static_cast<float>(screen_width_),
        static_cast<float>(screen_height_),
        0.0f,
        engine::Color{255, 255, 255, 0},  // Transparent initially
        0.0f,
        0.0f,
        200
    });

    // Create back to menu button (centered at bottom)
    float button_width = 300.0f;
    float button_height = 60.0f;
    float button_x = (screen_width_ - button_width) / 2.0f;
    float button_y = screen_height_ - 130.0f;

    back_to_menu_button_ = std::make_unique<rtype::UIButton>(
        button_x, button_y, button_width, button_height, "Back to Menu"
    );
    back_to_menu_button_->set_enabled(false);  // Initially disabled
}

void ScreenManager::set_screen(GameScreen screen) {
    current_screen_ = screen;

    switch (screen) {
        case GameScreen::WAITING:
            show_waiting_screen();
            break;
        case GameScreen::PLAYING:
            hide_waiting_screen();
            hide_result_screen();
            break;
        case GameScreen::VICTORY:
        case GameScreen::DEFEAT:
            // Result screens are shown via show_result()
            break;
        case GameScreen::MAIN_MENU:
        case GameScreen::CREATE_ROOM:
        case GameScreen::BROWSE_ROOMS:
        case GameScreen::ROOM_LOBBY:
            // Hide game screens when returning to menu
            hide_waiting_screen();
            hide_result_screen();
            break;
    }
}

void ScreenManager::hide_waiting_screen() {
    auto& sprites = registry_.get_components<Sprite>();
    auto& texts = registry_.get_components<UIText>();

    if (sprites.has_entity(waiting_screen_bg_))
        sprites[waiting_screen_bg_].tint = engine::Color{255, 255, 255, 0};
    if (texts.has_entity(waiting_screen_text_))
        texts[waiting_screen_text_].active = false;
}

void ScreenManager::show_waiting_screen() {
    auto& sprites = registry_.get_components<Sprite>();
    auto& texts = registry_.get_components<UIText>();

    if (sprites.has_entity(waiting_screen_bg_))
        sprites[waiting_screen_bg_].tint = engine::Color::White;
    if (texts.has_entity(waiting_screen_text_))
        texts[waiting_screen_text_].active = true;
}

void ScreenManager::hide_result_screen() {
    auto& sprites = registry_.get_components<Sprite>();

    // Hide the result screen background by making it transparent
    if (sprites.has_entity(result_screen_bg_))
        sprites[result_screen_bg_].tint = engine::Color{255, 255, 255, 0};

    // Disable the back to menu button
    if (back_to_menu_button_)
        back_to_menu_button_->set_enabled(false);
}

void ScreenManager::load_result_textures() {
    if (!result_textures_loaded_) {
        win_background_texture_ = graphics_plugin_->load_texture("assets/sprite/backgrounds/win-screen.png");
        loose_background_texture_ = graphics_plugin_->load_texture("assets/sprite/backgrounds/loose-screen.png");
        result_textures_loaded_ = true;
    }
}

void ScreenManager::show_result(bool victory, int final_score) {
    auto& sprites = registry_.get_components<Sprite>();
    auto& positions = registry_.get_components<Position>();

    // Store the final score
    final_score_ = final_score;

    // Load result textures if not already loaded
    load_result_textures();

    // Choose the correct background texture based on victory/defeat
    engine::TextureHandle bg_texture = victory ? win_background_texture_ : loose_background_texture_;

    // Recreate background sprite if needed
    if (!sprites.has_entity(result_screen_bg_) || !positions.has_entity(result_screen_bg_)) {
        result_screen_bg_ = registry_.spawn_entity();
        registry_.add_component(result_screen_bg_, Position{0.0f, 0.0f});
        registry_.add_component(result_screen_bg_, Sprite{
            bg_texture,
            static_cast<float>(screen_width_),
            static_cast<float>(screen_height_),
            0.0f,
            engine::Color::White,
            0.0f,
            0.0f,
            200
        });
    } else {
        sprites[result_screen_bg_].texture = bg_texture;
        sprites[result_screen_bg_].tint = engine::Color::White;
    }

    // Enable the back to menu button
    back_to_menu_button_->set_enabled(true);

    current_screen_ = victory ? GameScreen::VICTORY : GameScreen::DEFEAT;
}

void ScreenManager::update_result_screen(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    if (current_screen_ == GameScreen::VICTORY || current_screen_ == GameScreen::DEFEAT) {
        // Set callback if not already set
        if (!back_to_menu_button_->has_callback()) {
            back_to_menu_button_->set_on_click([this]() {
                if (back_to_menu_callback_) {
                    back_to_menu_callback_();
                }
            });
        }

        back_to_menu_button_->update(graphics, input);
    }
}

void ScreenManager::draw_result_screen(engine::IGraphicsPlugin* graphics) {
    if (current_screen_ == GameScreen::VICTORY || current_screen_ == GameScreen::DEFEAT) {
        // Draw final score in large text centered
        std::string score_text = "SCORE: " + std::to_string(final_score_);

        // Position: centered horizontally, lower on screen
        float text_x = screen_width_ / 2.0f - 340.0f;  // Better centered for large text
        float text_y = 460.0f;

        // Draw shadow for depth
        engine::Vector2f shadow_pos{text_x + 6, text_y + 6};
        graphics->draw_text(score_text, shadow_pos, {0, 0, 0, 200}, engine::INVALID_HANDLE, 120);

        // Draw main text in violet
        engine::Vector2f text_pos{text_x, text_y};
        graphics->draw_text(score_text, text_pos, {150, 100, 255, 255}, engine::INVALID_HANDLE, 120);

        // Draw button
        back_to_menu_button_->draw(graphics);
    }
}

}
