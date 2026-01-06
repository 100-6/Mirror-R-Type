#include "screens/RoomLobbyScreen.hpp"
#include "NetworkClient.hpp"
#include "ScreenManager.hpp"
#include <iostream>

namespace rtype::client {

RoomLobbyScreen::RoomLobbyScreen(NetworkClient& network_client, int screen_width, int screen_height)
    : BaseScreen(network_client, screen_width, screen_height) {
}

void RoomLobbyScreen::initialize() {
    labels_.clear();
    buttons_.clear();

    float center_x = screen_width_ / 2.0f;
    float start_y = 100.0f;

    // Title
    auto title = std::make_unique<UILabel>(center_x, start_y, "Room Lobby", 36);
    title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(title));

    // Leave button
    auto leave_btn = std::make_unique<UIButton>(50, 50, 150, 40, "Leave");
    leave_btn->set_on_click([this]() {
        network_client_.send_leave_room();
        if (on_screen_change_) {
            on_screen_change_(GameScreen::MAIN_MENU);
        }
    });
    buttons_.push_back(std::move(leave_btn));

    // Min players controls (for host)
    float control_y = screen_height_ - 220;

    // Decrease button
    auto decrease_btn = std::make_unique<UIButton>(center_x - 130, control_y, 40, 40, "-");
    decrease_btn->set_on_click([this]() {
        if (min_players_to_start_ > 1) {
            min_players_to_start_--;
        }
    });
    buttons_.push_back(std::move(decrease_btn));

    // Increase button
    auto increase_btn = std::make_unique<UIButton>(center_x + 90, control_y, 40, 40, "+");
    increase_btn->set_on_click([this]() {
        if (min_players_to_start_ < max_players_) {
            min_players_to_start_++;
        }
    });
    buttons_.push_back(std::move(increase_btn));

    // Start Game button (for host)
    auto start_btn = std::make_unique<UIButton>(
        center_x - 100, screen_height_ - 150, 200, 50, "Start Game");
    start_btn->set_on_click([this]() {
        if (current_players_ < min_players_to_start_) {
            set_error_message("Need at least " + std::to_string(min_players_to_start_) + " players to start!");
            std::cout << "[RoomLobbyScreen] " << error_message_ << "\n";
        } else {
            network_client_.send_start_game();
        }
    });
    buttons_.push_back(std::move(start_btn));
}

void RoomLobbyScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    // Update buttons based on host status and countdown
    for (size_t i = 0; i < buttons_.size(); ++i) {
        // Button 0 is Leave button - always update
        // Buttons 1,2 are +/- buttons - only update for host when no countdown
        // Button 3 is Start button - only update for host when no countdown
        bool should_update = (i == 0);  // Always update Leave button
        if (is_host_ && countdown_value_ == 0 && i > 0) {
            should_update = true;  // Update all buttons for host when no countdown
        }

        if (should_update) {
            buttons_[i]->update(graphics, input);
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
}

void RoomLobbyScreen::draw(engine::IGraphicsPlugin* graphics) {
    graphics->clear(engine::Color{20, 20, 30, 255});

    for (auto& label : labels_) {
        label->draw(graphics);
    }

    float center_x = screen_width_ / 2.0f;
    float center_y = screen_height_ / 2.0f;

    // Draw room name
    std::string room_title = "Room: " + room_name_;
    UILabel room_name_label(center_x, center_y - 100, room_title, 32);
    room_name_label.set_alignment(UILabel::Alignment::CENTER);
    room_name_label.set_color(engine::Color{100, 150, 255, 255});
    room_name_label.draw(graphics);

    // Draw players count
    std::string players_text = "Players: " + std::to_string(current_players_) +
                              "/" + std::to_string(max_players_);
    UILabel players_label(center_x, center_y - 50, players_text, 24);
    players_label.set_alignment(UILabel::Alignment::CENTER);
    players_label.draw(graphics);

    // Draw player slots
    float slots_start_y = center_y;
    for (uint8_t i = 0; i < max_players_; ++i) {
        std::string slot_text = "Player " + std::to_string(i + 1) + ": ";
        if (i < current_players_) {
            slot_text += "Connected";
        } else {
            slot_text += "Waiting...";
        }

        engine::Color slot_color = (i < current_players_) ?
            engine::Color{100, 255, 100, 255} :  // Green for connected
            engine::Color{150, 150, 150, 255};   // Gray for waiting

        UILabel slot_label(center_x, slots_start_y + i * 35, slot_text, 20);
        slot_label.set_alignment(UILabel::Alignment::CENTER);
        slot_label.set_color(slot_color);
        slot_label.draw(graphics);
    }

    // Draw countdown if active
    if (countdown_value_ > 0) {
        float countdown_y = slots_start_y + max_players_ * 35 + 30;
        std::string countdown_text = "Game starting in " + std::to_string(countdown_value_) + "s...";
        UILabel countdown_label(center_x, countdown_y, countdown_text, 36);
        countdown_label.set_alignment(UILabel::Alignment::CENTER);
        countdown_label.set_color(engine::Color{255, 255, 100, 255});  // Yellow
        countdown_label.draw(graphics);
    }

    // Draw host controls (only if player is host and no countdown)
    if (is_host_ && countdown_value_ == 0) {
        float control_y = screen_height_ - 220;

        UILabel min_players_label(center_x, control_y + 10,
                                 "Min players: " + std::to_string(min_players_to_start_), 20);
        min_players_label.set_alignment(UILabel::Alignment::CENTER);
        min_players_label.set_color(engine::Color{255, 200, 100, 255});
        min_players_label.draw(graphics);

        UILabel instruction_label(center_x, screen_height_ - 180,
                                  "Adjust min players and click Start Game", 18);
        instruction_label.set_alignment(UILabel::Alignment::CENTER);
        instruction_label.set_color(engine::Color{200, 200, 200, 255});
        instruction_label.draw(graphics);

        // Draw error message if any
        if (!error_message_.empty() && error_timer_ > 0.0f) {
            UILabel error_label(center_x, screen_height_ - 250, error_message_, 22);
            error_label.set_alignment(UILabel::Alignment::CENTER);
            error_label.set_color(engine::Color{255, 100, 100, 255});
            error_label.draw(graphics);
        }
    } else if (!is_host_) {
        UILabel waiting_label(center_x, screen_height_ - 180,
                             "Waiting for host to start the game...", 20);
        waiting_label.set_alignment(UILabel::Alignment::CENTER);
        waiting_label.set_color(engine::Color{200, 200, 200, 255});
        waiting_label.draw(graphics);
    }

    // Draw buttons (filter based on host status and countdown)
    for (size_t i = 0; i < buttons_.size(); ++i) {
        bool should_draw = (i == 0);  // Always draw Leave button
        if (is_host_ && countdown_value_ == 0 && i > 0) {
            should_draw = true;  // Show all buttons for host when no countdown
        }

        if (should_draw) {
            buttons_[i]->draw(graphics);
        }
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

}  // namespace rtype::client
