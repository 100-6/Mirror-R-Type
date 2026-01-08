#include "screens/BrowseRoomsScreen.hpp"
#include "NetworkClient.hpp"
#include "ScreenManager.hpp"
#include <iostream>

namespace rtype::client {

BrowseRoomsScreen::BrowseRoomsScreen(NetworkClient& network_client, int screen_width, int screen_height)
    : BaseScreen(network_client, screen_width, screen_height) {
}

void BrowseRoomsScreen::initialize() {
    labels_.clear();
    buttons_.clear();

    float center_x = screen_width_ / 2.0f;
    float start_y = 80.0f;

    // Title removed per user request

    // Back button
    auto back_btn = std::make_unique<UIButton>(back_button_x_, back_button_y_,
                                                 back_button_width_, back_button_height_, "Back");
    back_btn->set_on_click([this]() {
        if (on_screen_change_) {
            on_screen_change_(GameScreen::MAIN_MENU);
        }
    });
    buttons_.push_back(std::move(back_btn));

    // Refresh button
    auto refresh_btn = std::make_unique<UIButton>(refresh_button_x_, refresh_button_y_,
                                                    refresh_button_width_, refresh_button_height_, "Refresh");
    refresh_btn->set_on_click([this]() {
        network_client_.send_request_room_list();
    });
    buttons_.push_back(std::move(refresh_btn));
}

void BrowseRoomsScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    // Edit mode controls
    if (edit_mode_) {
        bool changed = false;

        // Tab to switch between buttons
        if (input->is_key_pressed(engine::Key::Tab)) {
            selected_button_ = (selected_button_ + 1) % 3;
            std::cout << "[EditMode] Selected button: " << selected_button_ << "\n";
        }

        // Arrow keys to move selected button
        if (input->is_key_pressed(engine::Key::Left)) {
            switch (selected_button_) {
                case 0: back_button_x_ -= move_speed_; break;
                case 1: refresh_button_x_ -= move_speed_; break;
                case 2: join_button_offset_ -= move_speed_; break;
            }
            changed = true;
        }
        if (input->is_key_pressed(engine::Key::Right)) {
            switch (selected_button_) {
                case 0: back_button_x_ += move_speed_; break;
                case 1: refresh_button_x_ += move_speed_; break;
                case 2: join_button_offset_ += move_speed_; break;
            }
            changed = true;
        }
        if (input->is_key_pressed(engine::Key::Up)) {
            switch (selected_button_) {
                case 0: back_button_y_ -= move_speed_; break;
                case 1: refresh_button_y_ -= move_speed_; break;
                case 2: join_button_height_ += move_speed_; break;
            }
            changed = true;
        }
        if (input->is_key_pressed(engine::Key::Down)) {
            switch (selected_button_) {
                case 0: back_button_y_ += move_speed_; break;
                case 1: refresh_button_y_ += move_speed_; break;
                case 2: join_button_height_ -= move_speed_; break;
            }
            changed = true;
        }

        // P to print positions
        if (input->is_key_pressed(engine::Key::P)) {
            std::cout << "\n=== BROWSE ROOMS BUTTON POSITIONS ===\n";
            std::cout << "back_button_x_ = " << back_button_x_ << "f;\n";
            std::cout << "back_button_y_ = " << back_button_y_ << "f;\n";
            std::cout << "refresh_button_x_ = " << refresh_button_x_ << "f;\n";
            std::cout << "refresh_button_y_ = " << refresh_button_y_ << "f;\n";
            std::cout << "join_button_width_ = " << join_button_width_ << "f;\n";
            std::cout << "join_button_height_ = " << join_button_height_ << "f;\n";
            std::cout << "join_button_offset_ = " << join_button_offset_ << "f;\n";
            std::cout << "======================================\n\n";
        }

        // E to exit edit mode
        if (input->is_key_pressed(engine::Key::E)) {
            edit_mode_ = false;
            std::cout << "[EditMode] Edit mode disabled\n";
        }

        // Update button positions if changed
        if (changed) {
            if (!buttons_.empty()) {
                if (buttons_.size() > 0) buttons_[0]->set_position(back_button_x_, back_button_y_);
                if (buttons_.size() > 1) buttons_[1]->set_position(refresh_button_x_, refresh_button_y_);
            }
            // Recreate join buttons with new dimensions/position
            if (selected_button_ == 2) {
                create_room_join_buttons();
            }
        }

        // Don't update button interactions in edit mode
        return;
    }

    // Normal mode
    for (auto& button : buttons_) {
        button->update(graphics, input);
    }

    for (auto& button : room_join_buttons_) {
        button->update(graphics, input);
    }
}

void BrowseRoomsScreen::draw(engine::IGraphicsPlugin* graphics) {
    // Load background texture on first draw
    if (!background_loaded_) {
        background_texture_ = graphics->load_texture("assets/sprite/browse-background.png");
        background_loaded_ = true;
    }

    // Clear with dark background as fallback
    graphics->clear(engine::Color{15, 15, 25, 255});

    // Draw background image stretched to fill screen
    engine::Sprite background_sprite;
    background_sprite.texture_handle = background_texture_;
    background_sprite.size = {static_cast<float>(screen_width_), static_cast<float>(screen_height_)};
    background_sprite.origin = {0.0f, 0.0f};
    background_sprite.tint = {255, 255, 255, 255};

    graphics->draw_sprite(background_sprite, {0.0f, 0.0f});

    for (auto& label : labels_) {
        label->draw(graphics);
    }

    // Draw rooms list with stepper-style elegant design
    float start_y = 270.0f;
    float card_spacing = 85.0f;

    for (size_t i = 0; i < available_rooms_.size() && i < 10; ++i) {
        const auto& room = available_rooms_[i];

        // Card positioning
        float card_x = 250;
        float card_y = start_y + i * card_spacing;
        float card_width = screen_width_ - 500.0f;
        float card_height = 70.0f;
        float corner_radius = 18.0f;

        // Enhanced shadow for depth (stepper style)
        float shadow_offset = 4.0f;

        // Shadow rectangles
        engine::Rectangle shadow_h{card_x + shadow_offset + corner_radius, card_y + shadow_offset,
                                   card_width - corner_radius * 2, card_height};
        graphics->draw_rectangle(shadow_h, {0, 0, 0, 140});
        engine::Rectangle shadow_v{card_x + shadow_offset, card_y + shadow_offset + corner_radius,
                                   card_width, card_height - corner_radius * 2};
        graphics->draw_rectangle(shadow_v, {0, 0, 0, 140});

        // Shadow corners
        graphics->draw_circle({card_x + shadow_offset + corner_radius, card_y + shadow_offset + corner_radius},
                             corner_radius, {0, 0, 0, 140});
        graphics->draw_circle({card_x + card_width + shadow_offset - corner_radius, card_y + shadow_offset + corner_radius},
                             corner_radius, {0, 0, 0, 140});
        graphics->draw_circle({card_x + shadow_offset + corner_radius, card_y + card_height + shadow_offset - corner_radius},
                             corner_radius, {0, 0, 0, 140});
        graphics->draw_circle({card_x + card_width + shadow_offset - corner_radius, card_y + card_height + shadow_offset - corner_radius},
                             corner_radius, {0, 0, 0, 140});

        // Card base color (dark purple-blue gradient base)
        engine::Color base_color{50, 55, 85, 240};

        // Main card rectangles
        engine::Rectangle card_h{card_x + corner_radius, card_y, card_width - corner_radius * 2, card_height};
        graphics->draw_rectangle(card_h, base_color);
        engine::Rectangle card_v{card_x, card_y + corner_radius, card_width, card_height - corner_radius * 2};
        graphics->draw_rectangle(card_v, base_color);

        // Card corners
        graphics->draw_circle({card_x + corner_radius, card_y + corner_radius}, corner_radius, base_color);
        graphics->draw_circle({card_x + card_width - corner_radius, card_y + corner_radius}, corner_radius, base_color);
        graphics->draw_circle({card_x + corner_radius, card_y + card_height - corner_radius}, corner_radius, base_color);
        graphics->draw_circle({card_x + card_width - corner_radius, card_y + card_height - corner_radius}, corner_radius, base_color);

        // Top gradient overlay for depth (stepper style)
        engine::Color highlight_color{70, 80, 120, 240};

        engine::Rectangle top_gradient_h{card_x + corner_radius, card_y,
                                        card_width - corner_radius * 2, card_height / 2.5f};
        graphics->draw_rectangle(top_gradient_h, highlight_color);
        engine::Rectangle top_gradient_v{card_x, card_y + corner_radius,
                                        card_width, card_height / 2.5f - corner_radius};
        graphics->draw_rectangle(top_gradient_v, highlight_color);

        // Top corners gradient
        graphics->draw_circle({card_x + corner_radius, card_y + corner_radius}, corner_radius, highlight_color);
        graphics->draw_circle({card_x + card_width - corner_radius, card_y + corner_radius}, corner_radius, highlight_color);

        // Elegant border (purple accent like stepper)
        engine::Color border_color{100, 90, 150, 220};
        float border_width = 2.5f;

        // Border rectangles
        engine::Rectangle border_top{card_x + corner_radius, card_y, card_width - corner_radius * 2, border_width};
        graphics->draw_rectangle(border_top, border_color);
        engine::Rectangle border_bottom{card_x + corner_radius, card_y + card_height - border_width,
                                       card_width - corner_radius * 2, border_width};
        graphics->draw_rectangle(border_bottom, border_color);
        engine::Rectangle border_left{card_x, card_y + corner_radius, border_width, card_height - corner_radius * 2};
        graphics->draw_rectangle(border_left, border_color);
        engine::Rectangle border_right{card_x + card_width - border_width, card_y + corner_radius,
                                      border_width, card_height - corner_radius * 2};
        graphics->draw_rectangle(border_right, border_color);

        // Room name (main title)
        std::string room_name = std::string(room.room_name);
        UILabel name_label(card_x + 25, card_y + 12, room_name, 28);
        name_label.set_color(engine::Color{220, 210, 255, 255});
        name_label.draw(graphics);

        // Player count info
        std::string player_info = std::to_string(room.current_players) + "/" +
                                 std::to_string(room.max_players) + " Players";
        UILabel info_label(card_x + 25, card_y + 42, player_info, 18);
        info_label.set_color(engine::Color{160, 150, 200, 220});
        info_label.draw(graphics);

        // Private indicator with icon style
        if (room.is_private) {
            float lock_x = card_x + card_width - 255;
            float lock_y = card_y + card_height / 2.0f;

            UILabel private_label(lock_x, lock_y - 9, "PRIVATE", 16);
            private_label.set_color(engine::Color{255, 200, 100, 255});
            private_label.draw(graphics);
        }
    }

    for (auto& button : buttons_) {
        button->draw(graphics);
    }

    for (auto& button : room_join_buttons_) {
        button->draw(graphics);
    }

    // Draw edit mode overlay
    if (edit_mode_) {
        // Draw semi-transparent overlay with instructions
        engine::Rectangle overlay{0, 0, static_cast<float>(screen_width_), 100.0f};
        graphics->draw_rectangle(overlay, {0, 0, 0, 200});

        // Draw instructions
        engine::Vector2f instr_pos{10, 10};
        std::string instructions = selected_button_ == 2 ?
            "EDIT MODE: Tab=Switch | Left/Right=Move | Up/Down=Height | P=Print | E=Exit" :
            "EDIT MODE: Tab=Switch Button | Arrows=Move | P=Print | E=Exit";
        graphics->draw_text(instructions,
                          instr_pos, engine::Color{255, 255, 0, 255}, engine::INVALID_HANDLE, 20);

        // Draw current button info
        const char* button_names[] = {"Back", "Refresh", "Join"};
        engine::Vector2f info_pos{10, 40};
        std::string info_text = "Selected: " + std::string(button_names[selected_button_]);
        graphics->draw_text(info_text, info_pos, engine::Color{255, 255, 255, 255}, engine::INVALID_HANDLE, 18);

        // Draw position values
        engine::Vector2f values_pos{10, 65};
        std::string values_text;
        switch (selected_button_) {
            case 0:
                values_text = "X: " + std::to_string(static_cast<int>(back_button_x_)) +
                             " Y: " + std::to_string(static_cast<int>(back_button_y_));
                break;
            case 1:
                values_text = "X: " + std::to_string(static_cast<int>(refresh_button_x_)) +
                             " Y: " + std::to_string(static_cast<int>(refresh_button_y_));
                break;
            case 2:
                values_text = "Offset: " + std::to_string(static_cast<int>(join_button_offset_)) +
                             " Height: " + std::to_string(static_cast<int>(join_button_height_));
                break;
        }
        graphics->draw_text(values_text, values_pos, engine::Color{255, 255, 255, 255}, engine::INVALID_HANDLE, 18);

        // Draw crosshair on selected button
        float btn_center_x = 0, btn_center_y = 0;
        bool draw_crosshair = false;

        switch (selected_button_) {
            case 0:
                btn_center_x = back_button_x_ + back_button_width_ / 2.0f;
                btn_center_y = back_button_y_ + back_button_height_ / 2.0f;
                draw_crosshair = true;
                break;
            case 1:
                btn_center_x = refresh_button_x_ + refresh_button_width_ / 2.0f;
                btn_center_y = refresh_button_y_ + refresh_button_height_ / 2.0f;
                draw_crosshair = true;
                break;
            case 2:
                // Draw crosshair on first join button
                if (!room_join_buttons_.empty()) {
                    float start_y = 270.0f;
                    float card_height = 70.0f;
                    float btn_x = screen_width_ - 250.0f - join_button_width_ + join_button_offset_;
                    float btn_y = start_y + (card_height - join_button_height_) / 2.0f;
                    btn_center_x = btn_x + join_button_width_ / 2.0f;
                    btn_center_y = btn_y + join_button_height_ / 2.0f;
                    draw_crosshair = true;
                }
                break;
        }

        if (draw_crosshair) {
            // Draw crosshair
            graphics->draw_line({btn_center_x - 15, btn_center_y}, {btn_center_x + 15, btn_center_y},
                              {255, 0, 0, 255}, 2.0f);
            graphics->draw_line({btn_center_x, btn_center_y - 15}, {btn_center_x, btn_center_y + 15},
                              {255, 0, 0, 255}, 2.0f);
        }
    }
}

void BrowseRoomsScreen::set_room_list(const std::vector<protocol::RoomInfo>& rooms) {
    available_rooms_ = rooms;
    create_room_join_buttons();
}

void BrowseRoomsScreen::create_room_join_buttons() {
    room_join_buttons_.clear();

    float start_y = 270.0f;
    float card_spacing = 85.0f;
    float card_height = 70.0f;

    for (size_t i = 0; i < available_rooms_.size() && i < 10; ++i) {
        const auto& room = available_rooms_[i];

        // Position button on the right side of the card, vertically centered
        // Use member variables for width, height, and offset
        float btn_x = screen_width_ - 250.0f - join_button_width_ + join_button_offset_;
        float btn_y = start_y + i * card_spacing + (card_height - join_button_height_) / 2.0f;

        auto join_btn = std::make_unique<UIButton>(btn_x, btn_y, join_button_width_, join_button_height_, "Join");

        uint32_t room_id = room.room_id;
        bool is_private = room.is_private != 0;

        join_btn->set_on_click([this, room_id, is_private]() {
            std::cout << "[BrowseRoomsScreen] Joining room " << room_id << " (private: " << is_private << ")\n";
            if (is_private) {
                if (on_password_dialog_) {
                    on_password_dialog_(room_id);
                }
            } else {
                network_client_.send_join_room(room_id, "");
            }
        });

        room_join_buttons_.push_back(std::move(join_btn));
    }
}

}  // namespace rtype::client
