#include "screens/MainMenuScreen.hpp"
#include "NetworkClient.hpp"
#include "ScreenManager.hpp"
#include <iostream>

namespace rtype::client {

MainMenuScreen::MainMenuScreen(NetworkClient& network_client, int screen_width, int screen_height)
    : BaseScreen(network_client, screen_width, screen_height) {
}

void MainMenuScreen::initialize() {
    labels_.clear();
    buttons_.clear();

    // Position invisible buttons over the visual buttons in menu-background.png
    // The background image is 1248x832 and will be stretched to screen size
    // Visual buttons are on the right side of the image

    // Calculate scaling factors
    float scale_x = screen_width_ / 1248.0f;
    float scale_y = screen_height_ / 832.0f;

    // Final button positions (calibrated with edit mode)
    // Unscaled coordinates from edit mode calibration
    float button_width = 250.0f * scale_x;    // Button width
    float button_height = 80.0f * scale_y;    // Button height

    // PLAY button (top) - Unscaled: x=915.55 y=142.733
    auto play_btn = std::make_unique<UIButton>(
        915.55f * scale_x, 142.733f * scale_y, button_width, button_height, "PLAY");
    play_btn->set_on_click([this]() {
        std::cout << "[MainMenuScreen] Play (Create Room) selected\n";
        if (on_screen_change_) {
            on_screen_change_(GameScreen::CREATE_ROOM);
        }
    });
    buttons_.push_back(std::move(play_btn));

    // QUIT button (middle) - Unscaled: x=915.45 y=370.378
    auto quit_btn = std::make_unique<UIButton>(
        915.45f * scale_x, 370.378f * scale_y, button_width, button_height, "QUIT");
    quit_btn->set_on_click([this]() {
        std::cout << "[MainMenuScreen] Quit selected\n";
        network_client_.disconnect();
        exit(0);
    });
    buttons_.push_back(std::move(quit_btn));

    // BROWSE ROOMS button (bottom) - Unscaled: x=915.55 y=595
    auto browse_btn = std::make_unique<UIButton>(
        915.55f * scale_x, 595.0f * scale_y, button_width, button_height * 1.15f, "BROWSE");
    browse_btn->set_on_click([this]() {
        std::cout << "[MainMenuScreen] Browse Rooms selected\n";
        network_client_.send_request_room_list();
        if (on_screen_change_) {
            on_screen_change_(GameScreen::BROWSE_ROOMS);
        }
    });
    buttons_.push_back(std::move(browse_btn));
}

void MainMenuScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    if (edit_mode_) {
        // Edit mode controls:
        // 1/2/3 - Select button (PLAY/QUIT/BROWSE)
        // Arrow keys - Move selected button
        // WASD - Alternative move keys
        // P - Print coordinates
        std::cout << "[EDIT MODE ACTIVE] Button " << selected_button_ << " selected. Use arrows or WASD to move, 1/2/3 to select, P to print\n";

        // Select button
        if (input->is_key_pressed(engine::Key::Num1)) {
            selected_button_ = 0;
            std::cout << "[EDIT MODE] Selected PLAY button (0)\n";
        } else if (input->is_key_pressed(engine::Key::Num2)) {
            selected_button_ = 1;
            std::cout << "[EDIT MODE] Selected QUIT button (1)\n";
        } else if (input->is_key_pressed(engine::Key::Num3)) {
            selected_button_ = 2;
            std::cout << "[EDIT MODE] Selected BROWSE button (2)\n";
        }

        // Move selected button
        if (selected_button_ >= 0 && selected_button_ < buttons_.size()) {
            auto& button = buttons_[selected_button_];
            float current_x = button->get_x();
            float current_y = button->get_y();
            bool moved = false;

            // Arrow keys or WASD for movement
            if (input->is_key_pressed(engine::Key::Left) || input->is_key_pressed(engine::Key::A)) {
                button->set_position(current_x - move_speed_, current_y);
                moved = true;
            }
            if (input->is_key_pressed(engine::Key::Right) || input->is_key_pressed(engine::Key::D)) {
                button->set_position(current_x + move_speed_, current_y);
                moved = true;
            }
            if (input->is_key_pressed(engine::Key::Up) || input->is_key_pressed(engine::Key::W)) {
                button->set_position(current_x, current_y - move_speed_);
                moved = true;
            }
            if (input->is_key_pressed(engine::Key::Down) || input->is_key_pressed(engine::Key::S)) {
                button->set_position(current_x, current_y + move_speed_);
                moved = true;
            }

            if (moved) {
                std::cout << "[EDIT MODE] Button " << selected_button_ << " moved to: x="
                         << button->get_x() << " y=" << button->get_y() << "\n";
            }
        }

        // Print coordinates
        if (input->is_key_pressed(engine::Key::P)) {
            float scale_x = screen_width_ / 1248.0f;
            float scale_y = screen_height_ / 832.0f;

            std::cout << "\n========== BUTTON COORDINATES ==========\n";
            std::cout << "Scale factors: X=" << scale_x << " Y=" << scale_y << "\n";
            std::cout << "Screen size: " << screen_width_ << "x" << screen_height_ << "\n\n";

            for (size_t i = 0; i < buttons_.size(); ++i) {
                float x = buttons_[i]->get_x();
                float y = buttons_[i]->get_y();
                float w = buttons_[i]->get_width();
                float h = buttons_[i]->get_height();

                // Calculate unscaled values
                float unscaled_x = x / scale_x;
                float unscaled_y = y / scale_y;

                const char* names[] = {"PLAY", "QUIT", "BROWSE"};
                std::cout << names[i] << " button (" << i << "):\n";
                std::cout << "  Pixel position: x=" << x << " y=" << y << "\n";
                std::cout << "  Unscaled: x=" << unscaled_x << " y=" << unscaled_y << "\n";
                std::cout << "  Size: " << w << "x" << h << "\n\n";
            }
            std::cout << "========================================\n\n";
        }
    }

    for (auto& button : buttons_) {
        button->update(graphics, input);
    }
}

void MainMenuScreen::draw(engine::IGraphicsPlugin* graphics) {
    // Load background texture on first draw
    if (!background_loaded_) {
        background_texture_ = graphics->load_texture("assets/sprite/menu-background.png");
        background_loaded_ = true;
    }

    // Clear with dark background as fallback
    graphics->clear(engine::Color{15, 15, 25, 255});

    // Draw menu background image stretched to fill screen
    engine::Sprite background_sprite;
    background_sprite.texture_handle = background_texture_;
    background_sprite.size = {static_cast<float>(screen_width_), static_cast<float>(screen_height_)};
    background_sprite.origin = {0.0f, 0.0f};
    background_sprite.tint = {255, 255, 255, 255};

    graphics->draw_sprite(background_sprite, {0.0f, 0.0f});

    // Draw interactive buttons overlaying the background image buttons
    for (size_t i = 0; i < buttons_.size(); ++i) {
        buttons_[i]->draw(graphics);

        // Draw selection indicator in edit mode
        if (edit_mode_ && i == selected_button_) {
            float x = buttons_[i]->get_x();
            float y = buttons_[i]->get_y();
            float w = buttons_[i]->get_width();
            float h = buttons_[i]->get_height();

            // Draw red outline to show selected button
            engine::Rectangle outline{x - 5, y - 5, w + 10, h + 10};
            graphics->draw_rectangle_outline(outline, engine::Color{255, 0, 0, 255}, 3.0f);

            // Draw label
            const char* names[] = {"PLAY", "QUIT", "BROWSE"};
            engine::Vector2f label_pos{x, y - 25};
            graphics->draw_text(names[i], label_pos, engine::Color{255, 255, 0, 255}, engine::INVALID_HANDLE, 20);
        }
    }

    // Draw edit mode instructions
    if (edit_mode_) {
        engine::Vector2f instr_pos{10, 10};
        graphics->draw_text("EDIT MODE: 1/2/3=Select | Arrows/WASD=Move | P=Print Coords",
                          instr_pos, engine::Color{255, 255, 0, 255}, engine::INVALID_HANDLE, 18);
    }
}

}  // namespace rtype::client
