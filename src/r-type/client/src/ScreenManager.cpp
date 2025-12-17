#include "ScreenManager.hpp"

namespace rtype::client {

ScreenManager::ScreenManager(Registry& registry, int screen_width, int screen_height,
                             engine::TextureHandle background_tex, engine::TextureHandle menu_bg_tex)
    : registry_(registry)
    , screen_width_(screen_width)
    , screen_height_(screen_height)
    , background_texture_(background_tex)
    , menu_background_texture_(menu_bg_tex)
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

    // Result screen text (hidden initially)
    result_screen_text_ = registry_.spawn_entity();
    registry_.add_component(result_screen_text_, Position{
        static_cast<float>(screen_width_) / 2.0f - 150.0f,
        static_cast<float>(screen_height_) / 2.0f - 50.0f
    });
    registry_.add_component(result_screen_text_, UIText{
        "",
        engine::Color::White,
        engine::Color{0, 0, 0, 200},
        48,
        true,
        4.0f,
        4.0f,
        false,
        201
    });
}

void ScreenManager::set_screen(GameScreen screen) {
    current_screen_ = screen;

    switch (screen) {
        case GameScreen::WAITING:
            show_waiting_screen();
            break;
        case GameScreen::PLAYING:
            hide_waiting_screen();
            break;
        case GameScreen::VICTORY:
        case GameScreen::DEFEAT:
            // Result screens are shown via show_result()
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

void ScreenManager::show_result(bool victory) {
    auto& sprites = registry_.get_components<Sprite>();
    auto& texts = registry_.get_components<UIText>();

    if (sprites.has_entity(result_screen_bg_))
        sprites[result_screen_bg_].tint = engine::Color::White;

    if (texts.has_entity(result_screen_text_)) {
        texts[result_screen_text_].text = victory ? "VICTOIRE !" : "DEFAITE...";
        texts[result_screen_text_].color = victory
            ? engine::Color{100, 255, 100, 255}
            : engine::Color{255, 100, 100, 255};
        texts[result_screen_text_].active = true;
    }

    current_screen_ = victory ? GameScreen::VICTORY : GameScreen::DEFEAT;
}

}
