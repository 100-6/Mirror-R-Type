#include "screens/RoomLobbyScreen.hpp"
#include "NetworkClient.hpp"
#include "ScreenManager.hpp"
#include "AssetsPaths.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <set>

namespace rtype::client {

RoomLobbyScreen::RoomLobbyScreen(NetworkClient& network_client, int screen_width, int screen_height)
    : BaseScreen(network_client, screen_width, screen_height) {
}

void RoomLobbyScreen::initialize() {
    labels_.clear();
    buttons_.clear();

    float center_x = screen_width_ / 2.0f;
    float start_y = 100.0f;

    // Button positions are already set in the header file
    // No need to recalculate them here

    // Title
    auto title = std::make_unique<UILabel>(center_x, start_y, "Room Lobby", 36);
    title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(title));

    // Leave button
    auto leave_btn = std::make_unique<UIButton>(leave_button_x_, leave_button_y_,
                                                  leave_button_width_, leave_button_height_, "Leave");
    leave_btn->set_on_click([this]() {
        network_client_.send_leave_room();
        if (on_screen_change_) {
            on_screen_change_(GameScreen::MAIN_MENU);
        }
    });
    buttons_.push_back(std::move(leave_btn));

    // Min players controls (for host)
    // Decrease button
    auto decrease_btn = std::make_unique<UIButton>(decrease_button_x_, decrease_button_y_,
                                                     plus_minus_button_size_, plus_minus_button_size_, "-");
    decrease_btn->set_on_click([this]() {
        if (min_players_to_start_ > 1) {
            min_players_to_start_--;
        }
    });
    buttons_.push_back(std::move(decrease_btn));

    // Increase button
    auto increase_btn = std::make_unique<UIButton>(increase_button_x_, increase_button_y_,
                                                     plus_minus_button_size_, plus_minus_button_size_, "+");
    increase_btn->set_on_click([this]() {
        if (min_players_to_start_ < max_players_) {
            min_players_to_start_++;
        }
    });
    buttons_.push_back(std::move(increase_btn));

    // Start Game button (for host)
    auto start_btn = std::make_unique<UIButton>(start_button_x_, start_button_y_,
                                                  start_button_width_, start_button_height_, "Start Game");
    start_btn->set_on_click([this]() {
        if (current_players_ < min_players_to_start_) {
            set_error_message("Need at least " + std::to_string(min_players_to_start_) + " players to start!");
            std::cout << "[RoomLobbyScreen] " << error_message_ << "\n";
        } else {
            network_client_.send_start_game();
        }
    });
    buttons_.push_back(std::move(start_btn));

    // Change Name button
    auto change_name_btn = std::make_unique<UIButton>(change_name_button_x_, change_name_button_y_,
                                                       change_name_button_width_, change_name_button_height_, "Change Name");
    change_name_btn->set_on_click([this]() {
        editing_name_ = true;
        name_input_buffer_ = "";  // Start with empty buffer
        std::cout << "[RoomLobbyScreen] Entering name edit mode\n";
    });
    buttons_.push_back(std::move(change_name_btn));

    // Change Skin button
    auto change_skin_btn = std::make_unique<UIButton>(change_skin_button_x_, change_skin_button_y_,
                                                       change_skin_button_width_, change_skin_button_height_, "Change Skin");
    change_skin_btn->set_on_click([this]() {
        if (skin_selector_dialog_) {
            skin_selector_dialog_->show(local_player_skin_);
            std::cout << "[RoomLobbyScreen] Opening skin selector\n";
        }
    });
    buttons_.push_back(std::move(change_skin_btn));

    // Initialize skin selector dialog
    skin_selector_dialog_ = std::make_unique<SkinSelectorDialog>(screen_width_, screen_height_);
    skin_selector_dialog_->initialize();
    skin_selector_dialog_->set_spaceship_manager(spaceship_manager_.get());
    skin_selector_dialog_->set_confirm_callback([this](uint8_t skin_id) {
        local_player_skin_ = skin_id;
        network_client_.send_set_player_skin(skin_id);
        std::cout << "[RoomLobbyScreen] Selected skin " << static_cast<int>(skin_id) << "\n";
    });

    // Initialize chat overlay
    chat_overlay_ = std::make_unique<ChatOverlay>(
        static_cast<float>(screen_width_),
        static_cast<float>(screen_height_)
    );
    chat_overlay_->set_send_callback([this](const std::string& message) {
        network_client_.send_chat_message(message);
    });

    // Set up chat message callback
    network_client_.set_on_chat_message([this](uint32_t sender_id, const std::string& sender_name, const std::string& message) {
        if (chat_overlay_) {
            chat_overlay_->add_message(sender_id, sender_name, message);
        }
    });
}

void RoomLobbyScreen::on_enter() {
    // Re-register chat callback when entering this screen
    // This ensures our ChatOverlay receives messages after navigating between screens
    network_client_.set_on_chat_message([this](uint32_t sender_id, const std::string& sender_name, const std::string& message) {
        std::cout << "[RoomLobbyScreen] Chat callback received: " << sender_name << ": " << message << "\n";
        if (chat_overlay_) {
            chat_overlay_->add_message(sender_id, sender_name, message);
            std::cout << "[RoomLobbyScreen] Message added to chat overlay\n";
        } else {
            std::cout << "[RoomLobbyScreen] ERROR: chat_overlay_ is null!\n";
        }
    });
    std::cout << "[RoomLobbyScreen] Chat callback registered on enter\n";
}

void RoomLobbyScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    // Edit mode controls
    if (edit_mode_) {
        bool changed = false;

        // Tab to switch between buttons
        if (input->is_key_pressed(engine::Key::Tab)) {
            selected_button_ = (selected_button_ + 1) % 4;
            std::cout << "[EditMode] Selected button: " << selected_button_ << "\n";
        }

        // Arrow keys to move selected button
        if (input->is_key_pressed(engine::Key::Left)) {
            switch (selected_button_) {
                case 0: leave_button_x_ -= move_speed_; break;
                case 1: decrease_button_x_ -= move_speed_; break;
                case 2: increase_button_x_ -= move_speed_; break;
                case 3: start_button_x_ -= move_speed_; break;
            }
            changed = true;
        }
        if (input->is_key_pressed(engine::Key::Right)) {
            switch (selected_button_) {
                case 0: leave_button_x_ += move_speed_; break;
                case 1: decrease_button_x_ += move_speed_; break;
                case 2: increase_button_x_ += move_speed_; break;
                case 3: start_button_x_ += move_speed_; break;
            }
            changed = true;
        }
        if (input->is_key_pressed(engine::Key::Up)) {
            switch (selected_button_) {
                case 0: leave_button_y_ -= move_speed_; break;
                case 1: decrease_button_y_ -= move_speed_; break;
                case 2: increase_button_y_ -= move_speed_; break;
                case 3: start_button_y_ -= move_speed_; break;
            }
            changed = true;
        }
        if (input->is_key_pressed(engine::Key::Down)) {
            switch (selected_button_) {
                case 0: leave_button_y_ += move_speed_; break;
                case 1: decrease_button_y_ += move_speed_; break;
                case 2: increase_button_y_ += move_speed_; break;
                case 3: start_button_y_ += move_speed_; break;
            }
            changed = true;
        }

        // P to print positions
        if (input->is_key_pressed(engine::Key::P)) {
            std::cout << "\n=== ROOM LOBBY BUTTON POSITIONS ===\n";
            std::cout << "leave_button_x_ = " << leave_button_x_ << "f;\n";
            std::cout << "leave_button_y_ = " << leave_button_y_ << "f;\n";
            std::cout << "decrease_button_x_ = " << decrease_button_x_ << "f;\n";
            std::cout << "decrease_button_y_ = " << decrease_button_y_ << "f;\n";
            std::cout << "increase_button_x_ = " << increase_button_x_ << "f;\n";
            std::cout << "increase_button_y_ = " << increase_button_y_ << "f;\n";
            std::cout << "start_button_x_ = " << start_button_x_ << "f;\n";
            std::cout << "start_button_y_ = " << start_button_y_ << "f;\n";
            std::cout << "===================================\n\n";
        }

        // E to exit edit mode
        if (input->is_key_pressed(engine::Key::E)) {
            edit_mode_ = false;
            std::cout << "[EditMode] Edit mode disabled\n";
        }

        // Update button positions if changed
        if (changed && !buttons_.empty()) {
            if (buttons_.size() > 0) buttons_[0]->set_position(leave_button_x_, leave_button_y_);
            if (buttons_.size() > 1) buttons_[1]->set_position(decrease_button_x_, decrease_button_y_);
            if (buttons_.size() > 2) buttons_[2]->set_position(increase_button_x_, increase_button_y_);
            if (buttons_.size() > 3) buttons_[3]->set_position(start_button_x_, start_button_y_);
        }

        // Don't update button interactions in edit mode
        return;
    }

    // Chat overlay - T to open, F1 to close
    bool t_pressed = input->is_key_pressed(engine::Key::T);
    bool f1_pressed = input->is_key_pressed(engine::Key::F1);

    // Open chat with T (only when not already open and not editing name)
    if (t_pressed && !t_was_pressed_ && !editing_name_ && chat_overlay_ && !chat_overlay_->is_visible()) {
        chat_overlay_->set_visible(true);
    }
    t_was_pressed_ = t_pressed;

    // Close chat with F1
    if (f1_pressed && !f1_was_pressed_ && chat_overlay_ && chat_overlay_->is_visible()) {
        chat_overlay_->set_visible(false);
    }
    f1_was_pressed_ = f1_pressed;

    // Update chat if visible
    if (chat_overlay_ && chat_overlay_->is_visible()) {
        chat_overlay_->update(graphics, input);
        return;  // Don't process other inputs when chat is open
    }

    // Normal mode: Update buttons and slider
    // Update Leave button (always)
    if (buttons_.size() > 0) {
        buttons_[0]->update(graphics, input);
    }

    // Update Start button (only for host when no countdown)
    if (is_host_ && countdown_value_ == 0 && buttons_.size() > 3) {
        buttons_[3]->update(graphics, input);

        // Update min players slider
        if (input->is_mouse_button_pressed(engine::MouseButton::Left)) {
            engine::Vector2f mouse_pos = input->get_mouse_position();

            // Slider dimensions
            float slider_x = decrease_button_x_;
            float slider_y = decrease_button_y_;
            float slider_width = (increase_button_x_ + plus_minus_button_size_) - decrease_button_x_;
            float slider_height = plus_minus_button_size_;

            // Check if mouse is over slider
            if (mouse_pos.x >= slider_x && mouse_pos.x <= slider_x + slider_width &&
                mouse_pos.y >= slider_y && mouse_pos.y <= slider_y + slider_height) {

                // Calculate which value was clicked (1 to max_players_)
                float click_ratio = (mouse_pos.x - slider_x) / slider_width;
                int new_value = 1 + static_cast<int>(click_ratio * (max_players_ - 1) + 0.5f);
                new_value = std::max(1, std::min(static_cast<int>(max_players_), new_value));
                min_players_to_start_ = new_value;
            }
        }
    }

    // Update error message timer
    if (error_timer_ > 0.0f) {
        error_timer_ -= 0.016f;  // Assuming ~60 FPS
        if (error_timer_ < 0.0f) {
            error_timer_ = 0.0f;
            error_message_ = "";
        }
    }

    // Name editing mode - handle keyboard input
    if (editing_name_) {
        // Cursor blinking
        cursor_blink_timer_ += 0.016f;
        if (cursor_blink_timer_ >= 0.5f) {
            cursor_blink_timer_ = 0.0f;
            cursor_visible_ = !cursor_visible_;
        }

        // Handle text input (A-Z, 0-9, space) - use is_key_just_pressed for single trigger
        for (int key = static_cast<int>(engine::Key::A); key <= static_cast<int>(engine::Key::Z); ++key) {
            if (input->is_key_just_pressed(static_cast<engine::Key>(key))) {
                if (name_input_buffer_.length() < 20) {
                    char c = 'A' + (key - static_cast<int>(engine::Key::A));
                    name_input_buffer_ += c;
                }
            }
        }
        for (int key = static_cast<int>(engine::Key::Num0); key <= static_cast<int>(engine::Key::Num9); ++key) {
            if (input->is_key_just_pressed(static_cast<engine::Key>(key))) {
                if (name_input_buffer_.length() < 20) {
                    char c = '0' + (key - static_cast<int>(engine::Key::Num0));
                    name_input_buffer_ += c;
                }
            }
        }
        if (input->is_key_just_pressed(engine::Key::Space)) {
            if (name_input_buffer_.length() < 20) {
                name_input_buffer_ += ' ';
            }
        }

        // Backspace to delete
        if (input->is_key_just_pressed(engine::Key::Backspace)) {
            if (!name_input_buffer_.empty()) {
                name_input_buffer_.pop_back();
            }
        }

        // Enter to confirm
        if (input->is_key_just_pressed(engine::Key::Enter)) {
            if (!name_input_buffer_.empty()) {
                network_client_.send_set_player_name(name_input_buffer_);
            }
            editing_name_ = false;
            name_input_buffer_ = "";
        }

        // Escape to cancel
        if (input->is_key_just_pressed(engine::Key::Escape)) {
            editing_name_ = false;
            name_input_buffer_ = "";
        }
        return;  // Don't process other inputs while editing
    }

    // Update Change Name button (always visible)
    if (buttons_.size() > 4) {
        buttons_[4]->update(graphics, input);
    }

    // Update Change Skin button (always visible, unless dialog is open)
    if (buttons_.size() > 5 && (!skin_selector_dialog_ || !skin_selector_dialog_->is_visible())) {
        buttons_[5]->update(graphics, input);
    }

    // Update skin selector dialog
    if (skin_selector_dialog_ && skin_selector_dialog_->is_visible()) {
        skin_selector_dialog_->update(graphics, input);
    }
}

void RoomLobbyScreen::draw(engine::IGraphicsPlugin* graphics) {
    // Load textures on first draw
    if (!textures_loaded_) {
        background_texture_ = graphics->load_texture(assets::paths::UI_ROOM_BACKGROUND);
        spaceship_manager_ = std::make_unique<SpaceshipManager>(*graphics);
        spaceship_manager_->load_spritesheet(assets::paths::PLAYER_SPRITESHEET);
        // Update skin selector dialog with spaceship manager
        if (skin_selector_dialog_) {
            skin_selector_dialog_->set_spaceship_manager(spaceship_manager_.get());
        }
        textures_loaded_ = true;
    }

    // Clear with dark background
    graphics->clear(engine::Color{15, 15, 25, 255});

    // Draw background image
    engine::Sprite background_sprite;
    background_sprite.texture_handle = background_texture_;
    background_sprite.size = {static_cast<float>(screen_width_), static_cast<float>(screen_height_)};
    background_sprite.origin = {0.0f, 0.0f};
    background_sprite.tint = {255, 255, 255, 255};  // Full opacity, no dark filter
    graphics->draw_sprite(background_sprite, {0.0f, 0.0f});

    float center_x = screen_width_ / 2.0f;

    // Draw player slots in a grid (Fortnite style)
    float slot_width = 420.0f;   // Increased from 350
    float slot_height = 240.0f;  // Increased from 200
    float slot_spacing = 35.0f;  // Slightly increased spacing
    float slots_start_y = 280.0f; // Lowered for better positioning

    // Calculate grid layout (2x2 for 4 players)
    int cols = 2;
    int rows = (max_players_ + cols - 1) / cols;

    for (uint8_t i = 0; i < max_players_; ++i) {
        int row = i / cols;
        int col = i % cols;

        float slot_x = center_x - (cols * slot_width + (cols - 1) * slot_spacing) / 2.0f +
                      col * (slot_width + slot_spacing);
        float slot_y = slots_start_y + row * (slot_height + slot_spacing);

        draw_player_slot(graphics, i, slot_x, slot_y, slot_width, slot_height);
    }

    // Draw countdown if active - R-Type Evolution modern design
    if (countdown_value_ > 0) {
        // Panel dimensions - large and centered
        float panel_width = 900.0f;
        float panel_height = 200.0f;
        float panel_x = center_x - panel_width / 2.0f;
        float panel_y = screen_height_ / 2.0f - panel_height / 2.0f;
        float corner_radius = 20.0f;

        // Multi-layer shadow for dramatic depth
        for (int i = 0; i < 4; ++i) {
            float shadow_offset = 10.0f + i * 4.0f;
            float shadow_alpha = 40 - i * 8;
            engine::Rectangle shadow{panel_x + shadow_offset, panel_y + shadow_offset, panel_width, panel_height};
            graphics->draw_rectangle(shadow, engine::Color{0, 0, 0, static_cast<uint8_t>(shadow_alpha)});
        }

        // Main panel background - very dark purple with high opacity
        engine::Rectangle panel_main{panel_x + corner_radius, panel_y, panel_width - corner_radius * 2, panel_height};
        graphics->draw_rectangle(panel_main, engine::Color{15, 12, 25, 245});

        engine::Rectangle panel_left{panel_x, panel_y + corner_radius, corner_radius, panel_height - corner_radius * 2};
        graphics->draw_rectangle(panel_left, engine::Color{15, 12, 25, 245});

        engine::Rectangle panel_right{panel_x + panel_width - corner_radius, panel_y + corner_radius,
                                      corner_radius, panel_height - corner_radius * 2};
        graphics->draw_rectangle(panel_right, engine::Color{15, 12, 25, 245});

        // Rounded corners
        graphics->draw_circle({panel_x + corner_radius, panel_y + corner_radius},
                             corner_radius, engine::Color{15, 12, 25, 245});
        graphics->draw_circle({panel_x + panel_width - corner_radius, panel_y + corner_radius},
                             corner_radius, engine::Color{15, 12, 25, 245});
        graphics->draw_circle({panel_x + corner_radius, panel_y + panel_height - corner_radius},
                             corner_radius, engine::Color{15, 12, 25, 245});
        graphics->draw_circle({panel_x + panel_width - corner_radius, panel_y + panel_height - corner_radius},
                             corner_radius, engine::Color{15, 12, 25, 245});

        // Subtle gradient overlay for depth
        engine::Rectangle gradient_top{panel_x + corner_radius, panel_y,
                                       panel_width - corner_radius * 2, panel_height / 3};
        graphics->draw_rectangle(gradient_top, engine::Color{60, 50, 90, 50});

        // Pulsing neon violet border with glow effect
        float pulse = std::sin(countdown_value_ * 3.14159f);
        engine::Color border_color{160, 100, 255, static_cast<uint8_t>(220 + 35 * pulse)};
        float border_width = 6.0f;

        // Outer glow for neon effect
        engine::Color glow_color{160, 100, 255, 60};
        float glow_width = 2.0f;

        // Top border with glow
        engine::Rectangle glow_top{panel_x + corner_radius - glow_width, panel_y - glow_width,
                                   panel_width - corner_radius * 2 + glow_width * 2, border_width + glow_width * 2};
        graphics->draw_rectangle(glow_top, glow_color);
        engine::Rectangle border_top{panel_x + corner_radius, panel_y, panel_width - corner_radius * 2, border_width};
        graphics->draw_rectangle(border_top, border_color);

        // Bottom border with glow
        engine::Rectangle glow_bottom{panel_x + corner_radius - glow_width, panel_y + panel_height - border_width - glow_width,
                                      panel_width - corner_radius * 2 + glow_width * 2, border_width + glow_width * 2};
        graphics->draw_rectangle(glow_bottom, glow_color);
        engine::Rectangle border_bottom{panel_x + corner_radius, panel_y + panel_height - border_width,
                                       panel_width - corner_radius * 2, border_width};
        graphics->draw_rectangle(border_bottom, border_color);

        // Left border with glow
        engine::Rectangle glow_left{panel_x - glow_width, panel_y + corner_radius - glow_width,
                                   border_width + glow_width * 2, panel_height - corner_radius * 2 + glow_width * 2};
        graphics->draw_rectangle(glow_left, glow_color);
        engine::Rectangle border_left{panel_x, panel_y + corner_radius, border_width, panel_height - corner_radius * 2};
        graphics->draw_rectangle(border_left, border_color);

        // Right border with glow
        engine::Rectangle glow_right{panel_x + panel_width - border_width - glow_width, panel_y + corner_radius - glow_width,
                                    border_width + glow_width * 2, panel_height - corner_radius * 2 + glow_width * 2};
        graphics->draw_rectangle(glow_right, glow_color);
        engine::Rectangle border_right{panel_x + panel_width - border_width, panel_y + corner_radius,
                                      border_width, panel_height - corner_radius * 2};
        graphics->draw_rectangle(border_right, border_color);

        // Decorative accent line
        float accent_y = panel_y + panel_height / 2.0f + 55.0f;
        engine::Rectangle accent_line{panel_x + 150.0f, accent_y, panel_width - 300.0f, 2.0f};
        graphics->draw_rectangle(accent_line, engine::Color{120, 80, 200, 120});

        // "GAME STARTING" label text (smaller, above main countdown)
        float label_x = panel_x + panel_width / 2.0f;
        float label_y = panel_y + 35.0f;
        UILabel starting_label(label_x, label_y, "GAME STARTING", 28);
        starting_label.set_alignment(UILabel::Alignment::CENTER);
        starting_label.set_color(engine::Color{140, 120, 200, 255});
        starting_label.draw(graphics);

        // Main countdown number (HUGE and centered)
        float countdown_number_x = panel_x + panel_width / 2.0f;
        float countdown_number_y = panel_y + 75.0f;
        UILabel countdown_number(countdown_number_x, countdown_number_y, std::to_string(countdown_value_), 90);
        countdown_number.set_alignment(UILabel::Alignment::CENTER);
        countdown_number.set_color(engine::Color{100, 220, 255, 255});  // Bright cyan
        countdown_number.draw(graphics);

        // Bottom text "seconds remaining"
        float bottom_text_x = panel_x + panel_width / 2.0f;
        float bottom_text_y = panel_y + panel_height - 50.0f;
        std::string bottom_text = countdown_value_ == 1 ? "SECOND" : "SECONDS";
        UILabel bottom_label(bottom_text_x, bottom_text_y, bottom_text, 26);
        bottom_label.set_alignment(UILabel::Alignment::CENTER);
        bottom_label.set_color(engine::Color{160, 180, 220, 255});
        bottom_label.draw(graphics);
    }

    // Draw host controls
    if (is_host_ && countdown_value_ == 0) {
        if (!error_message_.empty() && error_timer_ > 0.0f) {
            UILabel error_label(center_x, screen_height_ - 250, error_message_, 24);
            error_label.set_alignment(UILabel::Alignment::CENTER);
            error_label.set_color(engine::Color{255, 100, 100, 255});
            error_label.draw(graphics);
        }
    } else if (!is_host_) {
        UILabel waiting_label(center_x, screen_height_ - 200,
                             "Waiting for host...", 22);
        waiting_label.set_alignment(UILabel::Alignment::CENTER);
        waiting_label.set_color(engine::Color{180, 180, 180, 255});
        waiting_label.draw(graphics);
    }

    // Draw buttons (Leave and Start Game only, skip +/- buttons)
    if (buttons_.size() > 0) {
        buttons_[0]->draw(graphics);  // Leave button
    }
    if (is_host_ && countdown_value_ == 0 && buttons_.size() > 3) {
        buttons_[3]->draw(graphics);  // Start Game button

        // Draw min players slider (R-Type Evolution style)
        float slider_x = decrease_button_x_;
        float slider_y = decrease_button_y_;
        float slider_width = (increase_button_x_ + plus_minus_button_size_) - decrease_button_x_;
        float slider_height = plus_minus_button_size_;

        // Background track with shadow
        engine::Rectangle track_shadow{slider_x + 4, slider_y + 4, slider_width, slider_height};
        graphics->draw_rectangle(track_shadow, {0, 0, 0, 120});

        // Main track
        engine::Rectangle track{slider_x, slider_y, slider_width, slider_height};
        graphics->draw_rectangle(track, {30, 35, 55, 240});

        // Inner shadow on track
        engine::Rectangle track_inner{slider_x + 3, slider_y + 3, slider_width - 6, slider_height - 6};
        graphics->draw_rectangle(track_inner, {20, 25, 40, 255});

        // Draw segments for each value (1 to max_players_)
        int num_segments = max_players_;
        float segment_width = slider_width / num_segments;

        for (int i = 0; i < num_segments; ++i) {
            float seg_x = slider_x + i * segment_width;
            bool is_active = (i < min_players_to_start_);

            // Segment background with glow if active
            if (is_active) {
                // Glow effect
                for (int j = 3; j > 0; j--) {
                    float glow_expand = 3.0f * j;
                    engine::Rectangle glow{
                        seg_x - glow_expand,
                        slider_y - glow_expand,
                        segment_width + glow_expand * 2,
                        slider_height + glow_expand * 2
                    };
                    graphics->draw_rectangle(glow, {150, 100, 255, static_cast<unsigned char>(30 / j)});
                }

                // Active segment
                engine::Rectangle segment{seg_x, slider_y, segment_width - 2, slider_height};
                graphics->draw_rectangle(segment, {100, 60, 160, 255});

                // Gradient overlay
                engine::Rectangle seg_gradient{seg_x, slider_y, segment_width - 2, slider_height / 2.0f};
                graphics->draw_rectangle(seg_gradient, {140, 90, 200, 200});
            }

            // Draw separator lines
            if (i > 0) {
                graphics->draw_line(
                    {seg_x, slider_y},
                    {seg_x, slider_y + slider_height},
                    {60, 60, 80, 150}, 2.0f
                );
            }

            // Draw value number
            std::string value_text = std::to_string(i + 1);
            engine::Color text_color = is_active ?
                engine::Color{230, 210, 255, 255} :
                engine::Color{120, 120, 150, 200};

            float text_x = seg_x + segment_width / 2.0f - 8.0f;
            float text_y = slider_y + slider_height / 2.0f - 12.0f;
            graphics->draw_text(value_text, {text_x, text_y}, text_color, engine::INVALID_HANDLE, 24);
        }

        // Border
        graphics->draw_rectangle_outline(track, {140, 100, 255, 255}, 3.0f);

        // Current value indicator (highlight)
        float indicator_x = slider_x + (min_players_to_start_ - 1) * segment_width;
        engine::Rectangle indicator{indicator_x, slider_y - 5, segment_width - 2, 3};
        graphics->draw_rectangle(indicator, {200, 180, 255, 255});
    }

    // Draw edit mode overlay
    if (edit_mode_) {
        // Draw semi-transparent overlay with instructions
        engine::Rectangle overlay{0, 0, static_cast<float>(screen_width_), 100.0f};
        graphics->draw_rectangle(overlay, {0, 0, 0, 200});

        // Draw instructions
        engine::Vector2f instr_pos{10, 10};
        graphics->draw_text("EDIT MODE: Tab=Switch Button | Arrows=Move | P=Print | E=Exit",
                          instr_pos, engine::Color{255, 255, 0, 255}, engine::INVALID_HANDLE, 20);

        // Draw current button info
        const char* button_names[] = {"Leave", "Decrease (-)", "Increase (+)", "Start Game"};
        engine::Vector2f info_pos{10, 40};
        std::string info_text = "Selected: " + std::string(button_names[selected_button_]);
        graphics->draw_text(info_text, info_pos, engine::Color{255, 255, 255, 255}, engine::INVALID_HANDLE, 18);

        // Draw position values
        engine::Vector2f values_pos{10, 65};
        std::string values_text;
        switch (selected_button_) {
            case 0:
                values_text = "X: " + std::to_string(static_cast<int>(leave_button_x_)) +
                             " Y: " + std::to_string(static_cast<int>(leave_button_y_));
                break;
            case 1:
                values_text = "X: " + std::to_string(static_cast<int>(decrease_button_x_)) +
                             " Y: " + std::to_string(static_cast<int>(decrease_button_y_));
                break;
            case 2:
                values_text = "X: " + std::to_string(static_cast<int>(increase_button_x_)) +
                             " Y: " + std::to_string(static_cast<int>(increase_button_y_));
                break;
            case 3:
                values_text = "X: " + std::to_string(static_cast<int>(start_button_x_)) +
                             " Y: " + std::to_string(static_cast<int>(start_button_y_));
                break;
        }
        graphics->draw_text(values_text, values_pos, engine::Color{255, 255, 255, 255}, engine::INVALID_HANDLE, 18);

        // Draw crosshair on selected button
        if (selected_button_ < buttons_.size()) {
            float btn_center_x = 0, btn_center_y = 0;
            switch (selected_button_) {
                case 0:
                    btn_center_x = leave_button_x_ + leave_button_width_ / 2.0f;
                    btn_center_y = leave_button_y_ + leave_button_height_ / 2.0f;
                    break;
                case 1:
                    btn_center_x = decrease_button_x_ + plus_minus_button_size_ / 2.0f;
                    btn_center_y = decrease_button_y_ + plus_minus_button_size_ / 2.0f;
                    break;
                case 2:
                    btn_center_x = increase_button_x_ + plus_minus_button_size_ / 2.0f;
                    btn_center_y = increase_button_y_ + plus_minus_button_size_ / 2.0f;
                    break;
                case 3:
                    btn_center_x = start_button_x_ + start_button_width_ / 2.0f;
                    btn_center_y = start_button_y_ + start_button_height_ / 2.0f;
                    break;
            }

            // Draw crosshair
            graphics->draw_line({btn_center_x - 15, btn_center_y}, {btn_center_x + 15, btn_center_y},
                              {255, 0, 0, 255}, 2.0f);
            graphics->draw_line({btn_center_x, btn_center_y - 15}, {btn_center_x, btn_center_y + 15},
                              {255, 0, 0, 255}, 2.0f);
        }
    }

    // Draw Change Name button (always visible)
    if (buttons_.size() > 4) {
        buttons_[4]->draw(graphics);
    }

    // Draw Change Skin button (always visible)
    if (buttons_.size() > 5) {
        buttons_[5]->draw(graphics);
    }

    // Draw name input dialog (on top of everything)
    draw_name_input(graphics);

    // Draw skin selector dialog (on top of everything else)
    if (skin_selector_dialog_) {
        skin_selector_dialog_->draw(graphics);
    }

    // Draw chat overlay and notification badge
    if (chat_overlay_) {
        chat_overlay_->draw(graphics);
        chat_overlay_->draw_notification_badge(graphics);
    }
}

void RoomLobbyScreen::set_room_info(uint32_t room_id, const std::string& room_name,
                                     uint8_t current_players, uint8_t max_players, bool is_host) {
    room_id_ = room_id;
    room_name_ = room_name;
    current_players_ = current_players;
    max_players_ = max_players;
    is_host_ = is_host;
}

void RoomLobbyScreen::set_countdown(uint8_t seconds) {
    countdown_value_ = seconds;
}

void RoomLobbyScreen::set_error_message(const std::string& message, float duration) {
    error_message_ = message;
    error_timer_ = duration;
}

void RoomLobbyScreen::add_player(uint32_t player_id, const std::string& name, uint8_t ship_type) {
    // Check if player already exists
    for (auto& player : players_) {
        if (player.player_id == player_id) {
            player.name = name;
            player.ship_type = ship_type;
            player.is_connected = true;
            return;
        }
    }

    // Calculate slot index
    int slot = -1;
    if (players_.empty()) {
        // First player gets slot 0 (host or first to join from this client's perspective)
        slot = 0;
    } else {
        // Find next available slot
        std::set<int> used_slots;
        for (const auto& p : players_) {
            if (p.slot_index >= 0) {
                used_slots.insert(p.slot_index);
            }
        }
        for (int i = 0; i < static_cast<int>(max_players_); ++i) {
            if (used_slots.find(i) == used_slots.end()) {
                slot = i;
                break;
            }
        }
    }

    // Add new player
    LobbyPlayer new_player;
    new_player.player_id = player_id;
    new_player.name = name;
    new_player.ship_type = ship_type;
    new_player.slot_index = slot;
    new_player.is_connected = true;
    new_player.is_ready = false;
    players_.push_back(new_player);
}

void RoomLobbyScreen::remove_player(uint32_t player_id) {
    players_.erase(
        std::remove_if(players_.begin(), players_.end(),
            [player_id](const LobbyPlayer& p) { return p.player_id == player_id; }),
        players_.end()
    );
}

void RoomLobbyScreen::set_player_ready(uint32_t player_id, bool ready) {
    for (auto& player : players_) {
        if (player.player_id == player_id) {
            player.is_ready = ready;
            return;
        }
    }
}

void RoomLobbyScreen::draw_player_slot(engine::IGraphicsPlugin* graphics, int slot_index,
                                       float x, float y, float width, float height) {
    // Check if this slot has a player based on current_players_ count
    bool has_player = slot_index < current_players_;

    float corner_radius = 15.0f;

    // R-Type Evolution style colors - darker, more cinematic
    engine::Color base_color = has_player ?
        engine::Color{25, 30, 50, 245} :      // Deep space blue for connected
        engine::Color{18, 18, 25, 220};       // Almost black for empty

    engine::Color border_color = has_player ?
        engine::Color{120, 100, 255, 255} :   // Vibrant purple border for connected
        engine::Color{60, 60, 80, 150};       // Dim gray border for empty

    // Shadow effect for depth
    if (has_player) {
        float shadow_offset = 8.0f;
        engine::Rectangle shadow{x + shadow_offset, y + shadow_offset, width, height};
        graphics->draw_rectangle(shadow, {0, 0, 0, 100});
    }

    // Main slot background
    engine::Rectangle slot_bg{x, y, width, height};
    graphics->draw_rectangle(slot_bg, base_color);

    // Animated neon glow effect when player is connected
    if (has_player) {
        // Multi-layer glow for neon effect
        for (int i = 3; i > 0; i--) {
            float glow_expand = 4.0f * i;
            engine::Color glow_color = {150, 100, 255, static_cast<unsigned char>(40 / i)};

            engine::Rectangle glow{
                x - glow_expand,
                y - glow_expand,
                width + glow_expand * 2,
                height + glow_expand * 2
            };
            graphics->draw_rectangle(glow, glow_color);
        }
    }

    // Inner gradient layers for depth
    engine::Rectangle top_gradient{x, y, width, height * 0.3f};
    engine::Color top_color = has_player ?
        engine::Color{40, 45, 80, 200} :
        engine::Color{25, 25, 35, 150};
    graphics->draw_rectangle(top_gradient, top_color);

    // Scanline effect (R-Type style)
    for (int i = 0; i < static_cast<int>(height); i += 4) {
        engine::Rectangle scanline{x, y + i, width, 1};
        graphics->draw_rectangle(scanline, {0, 0, 0, 15});
    }

    // Border with gradient effect
    float border_thickness = has_player ? 3.5f : 2.0f;

    // Outer border (darker)
    graphics->draw_rectangle_outline(slot_bg, {0, 0, 0, 150}, border_thickness + 1.5f);

    // Main border (colored)
    graphics->draw_rectangle_outline(slot_bg, border_color, border_thickness);

    // Inner highlight border (top edge only)
    if (has_player) {
        engine::Rectangle inner_highlight{x + 8, y + 8, width - 16, 2};
        graphics->draw_rectangle(inner_highlight, {200, 180, 255, 80});
    }

    if (has_player) {
        // Get player data if available, otherwise use default
        std::string player_name = "Player " + std::to_string(slot_index + 1);
        uint8_t ship_type = slot_index % 3;  // Default color rotation (0-2 for GREEN, RED, BLUE)
        bool is_ready = false;

        // Find player data by slot_index - search all players
        for (const auto& player : players_) {
            if (player.slot_index == slot_index && player.is_connected && !player.name.empty()) {
                player_name = player.name;
                ship_type = player.ship_type;
                is_ready = player.is_ready;
                break;
            }
        }

        // Draw ship sprite from Spaceships.png spritesheet
        if (spaceship_manager_ && spaceship_manager_->is_loaded()) {
            // ship_type is now just the color (0-2)
            // In lobby, all players are level 1, so always show SCOUT ship
            ShipColor color = static_cast<ShipColor>(ship_type % 3);  // 0-2 (GREEN, RED, BLUE)
            ShipType type = ShipType::SCOUT;  // Level 1 = SCOUT

            // Create sprite from spritesheet with larger scale
            float ship_scale = 4.2f;  // Increased from 3.5 for bigger slots
            engine::Sprite ship_sprite = spaceship_manager_->create_ship_sprite(color, type, ship_scale);

            // Position ship in upper center of slot
            engine::Vector2f ship_pos{
                x + width / 2.0f,
                y + height * 0.38f  // Slightly adjusted for better positioning
            };

            graphics->draw_sprite(ship_sprite, ship_pos);
        }

        // Decorative corner accents (R-Type style)
        float accent_size = 12.0f;
        engine::Color accent_color = {150, 120, 255, 200};

        // Top-left corner accent
        graphics->draw_line({x, y + accent_size}, {x, y}, accent_color, 3.0f);
        graphics->draw_line({x, y}, {x + accent_size, y}, accent_color, 3.0f);

        // Top-right corner accent
        graphics->draw_line({x + width - accent_size, y}, {x + width, y}, accent_color, 3.0f);
        graphics->draw_line({x + width, y}, {x + width, y + accent_size}, accent_color, 3.0f);

        // Player name with background bar
        float name_bar_height = 52.0f;  // Increased for larger slots
        engine::Rectangle name_bg{x, y + height - name_bar_height, width, name_bar_height};
        graphics->draw_rectangle(name_bg, {15, 15, 30, 220});

        // Name bar gradient overlay
        engine::Rectangle name_gradient{x, y + height - name_bar_height, width, name_bar_height / 2};
        graphics->draw_rectangle(name_gradient, {30, 30, 60, 100});

        // Player name text with glow effect
        UILabel name_shadow(x + width / 2.0f + 2, y + height - 28, player_name, 28);
        name_shadow.set_alignment(UILabel::Alignment::CENTER);
        name_shadow.set_color(engine::Color{100, 80, 200, 150});
        name_shadow.draw(graphics);

        UILabel name_label(x + width / 2.0f, y + height - 30, player_name, 28);
        name_label.set_alignment(UILabel::Alignment::CENTER);
        name_label.set_color(engine::Color{230, 230, 255, 255});
        name_label.draw(graphics);

        // Ready status with animated glow
        if (is_ready) {
            // Ready indicator background
            float ready_y = y + 15;
            engine::Rectangle ready_bg{x + 15, ready_y, 80, 28};
            graphics->draw_rectangle(ready_bg, {20, 100, 50, 200});

            // Ready glow
            graphics->draw_rectangle_outline(ready_bg, {100, 255, 100, 255}, 2.0f);

            UILabel ready_label(x + 55, ready_y + 6, "READY", 18);
            ready_label.set_alignment(UILabel::Alignment::CENTER);
            ready_label.set_color(engine::Color{150, 255, 150, 255});
            ready_label.draw(graphics);
        }

        // Connection indicator with pulsing effect (top-right)
        float indicator_x = x + width - 20;
        float indicator_y = y + 20;

        // Outer glow
        graphics->draw_circle({indicator_x, indicator_y}, 10.0f, {100, 255, 100, 80});
        graphics->draw_circle({indicator_x, indicator_y}, 7.0f, {100, 255, 100, 150});

        // Core indicator
        graphics->draw_circle({indicator_x, indicator_y}, 5.0f, {150, 255, 150, 255});

        // Slot number badge (top-left)
        std::string slot_badge = "#" + std::to_string(slot_index + 1);
        float badge_size = 32.0f;
        engine::Rectangle badge_bg{x + 12, y + 12, badge_size, badge_size};
        graphics->draw_rectangle(badge_bg, {40, 35, 80, 200});
        graphics->draw_rectangle_outline(badge_bg, {120, 100, 255, 255}, 2.0f);

        UILabel badge_label(x + 12 + badge_size / 2, y + 20, slot_badge, 18);
        badge_label.set_alignment(UILabel::Alignment::CENTER);
        badge_label.set_color(engine::Color{200, 180, 255, 255});
        badge_label.draw(graphics);

    } else {
        // Empty slot with stylish waiting message
        // Decorative dashed border
        float dash_length = 15.0f;
        float dash_gap = 10.0f;
        engine::Color dash_color = {80, 80, 100, 150};

        for (float i = 0; i < width; i += (dash_length + dash_gap)) {
            graphics->draw_line({x + i, y}, {x + std::min(i + dash_length, width), y}, dash_color, 2.0f);
            graphics->draw_line({x + i, y + height}, {x + std::min(i + dash_length, width), y + height}, dash_color, 2.0f);
        }

        // Icon placeholder (empty player silhouette concept)
        float icon_y = y + height / 2.0f - 20;
        graphics->draw_circle({x + width / 2.0f, icon_y}, 25.0f, {40, 40, 50, 150});
        graphics->draw_circle({x + width / 2.0f, icon_y}, 20.0f, {25, 25, 35, 200});

        // Waiting text with subtle animation hint
        UILabel waiting_label(x + width / 2.0f, y + height / 2.0f + 25, "WAITING FOR PLAYER", 20);
        waiting_label.set_alignment(UILabel::Alignment::CENTER);
        waiting_label.set_color(engine::Color{120, 120, 140, 255});
        waiting_label.draw(graphics);

        // Slot number
        std::string slot_num = "Slot " + std::to_string(slot_index + 1);
        UILabel slot_label(x + width / 2.0f, y + height / 2.0f + 50, slot_num, 16);
        slot_label.set_alignment(UILabel::Alignment::CENTER);
        slot_label.set_color(engine::Color{90, 90, 100, 255});
        slot_label.draw(graphics);
    }
}

void RoomLobbyScreen::draw_name_input(engine::IGraphicsPlugin* graphics) {
    if (!editing_name_) return;

    float center_x = screen_width_ / 2.0f;
    float center_y = screen_height_ / 2.0f;

    // Semi-transparent overlay
    engine::Rectangle overlay{0, 0, static_cast<float>(screen_width_), static_cast<float>(screen_height_)};
    graphics->draw_rectangle(overlay, engine::Color{0, 0, 0, 180});

    // Dialog box
    float dialog_width = 500.0f;
    float dialog_height = 180.0f;
    float dialog_x = center_x - dialog_width / 2.0f;
    float dialog_y = center_y - dialog_height / 2.0f;

    // Dialog background
    engine::Rectangle dialog_bg{dialog_x, dialog_y, dialog_width, dialog_height};
    graphics->draw_rectangle(dialog_bg, engine::Color{30, 25, 45, 250});

    // Dialog border
    engine::Rectangle border_top{dialog_x, dialog_y, dialog_width, 3.0f};
    engine::Rectangle border_bottom{dialog_x, dialog_y + dialog_height - 3.0f, dialog_width, 3.0f};
    engine::Rectangle border_left{dialog_x, dialog_y, 3.0f, dialog_height};
    engine::Rectangle border_right{dialog_x + dialog_width - 3.0f, dialog_y, 3.0f, dialog_height};
    engine::Color border_color{140, 100, 220, 255};
    graphics->draw_rectangle(border_top, border_color);
    graphics->draw_rectangle(border_bottom, border_color);
    graphics->draw_rectangle(border_left, border_color);
    graphics->draw_rectangle(border_right, border_color);

    // Title
    UILabel title(center_x, dialog_y + 30.0f, "ENTER NEW NAME", 24);
    title.set_alignment(UILabel::Alignment::CENTER);
    title.set_color(engine::Color{220, 200, 255, 255});
    title.draw(graphics);

    // Input field background
    float input_width = 400.0f;
    float input_height = 50.0f;
    float input_x = center_x - input_width / 2.0f;
    float input_y = dialog_y + 70.0f;
    engine::Rectangle input_bg{input_x, input_y, input_width, input_height};
    graphics->draw_rectangle(input_bg, engine::Color{20, 15, 30, 255});

    // Input text
    std::string display_text = name_input_buffer_;
    if (cursor_visible_) {
        display_text += "|";
    }
    UILabel input_text(input_x + 15.0f, input_y + 15.0f, display_text.empty() && !cursor_visible_ ? "Type your name..." : display_text, 22);
    input_text.set_alignment(UILabel::Alignment::LEFT);
    input_text.set_color(display_text.empty() && !cursor_visible_ ? engine::Color{100, 100, 120, 255} : engine::Color{255, 255, 255, 255});
    input_text.draw(graphics);

    // Instructions
    UILabel hint(center_x, dialog_y + dialog_height - 30.0f, "ENTER to confirm  |  ESC to cancel", 16);
    hint.set_alignment(UILabel::Alignment::CENTER);
    hint.set_color(engine::Color{140, 140, 160, 255});
    hint.draw(graphics);
}

void RoomLobbyScreen::update_player_name(uint32_t player_id, const std::string& new_name) {
    for (auto& player : players_) {
        if (player.player_id == player_id) {
            player.name = new_name;
            return;
        }
    }
    // Player not found - add them
    add_player(player_id, new_name, 0);
}

void RoomLobbyScreen::update_player_skin(uint32_t player_id, uint8_t skin_id) {
    for (auto& player : players_) {
        if (player.player_id == player_id) {
            player.ship_type = skin_id;
            // Update local skin if it's our own
            if (player_id == network_client_.get_player_id()) {
                local_player_skin_ = skin_id;
            }
            return;
        }
    }
    // Player not found - add them with default name and the skin
    add_player(player_id, "Player " + std::to_string(player_id), skin_id);
}

}  // namespace rtype::client
