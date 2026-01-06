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
    float start_y = 100.0f;

    // Title
    auto title = std::make_unique<UILabel>(center_x, start_y, "Available Rooms", 36);
    title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(title));

    // Back button
    auto back_btn = std::make_unique<UIButton>(50, 50, 150, 40, "Back");
    back_btn->set_on_click([this]() {
        if (on_screen_change_) {
            on_screen_change_(GameScreen::MAIN_MENU);
        }
    });
    buttons_.push_back(std::move(back_btn));

    // Refresh button
    auto refresh_btn = std::make_unique<UIButton>(220, 50, 150, 40, "Refresh");
    refresh_btn->set_on_click([this]() {
        network_client_.send_request_room_list();
    });
    buttons_.push_back(std::move(refresh_btn));
}

void BrowseRoomsScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    for (auto& button : buttons_) {
        button->update(graphics, input);
    }

    for (auto& button : room_join_buttons_) {
        button->update(graphics, input);
    }
}

void BrowseRoomsScreen::draw(engine::IGraphicsPlugin* graphics) {
    graphics->clear(engine::Color{20, 20, 30, 255});

    for (auto& label : labels_) {
        label->draw(graphics);
    }

    // Draw rooms list
    float start_y = 200.0f;
    for (size_t i = 0; i < available_rooms_.size() && i < 10; ++i) {
        const auto& room = available_rooms_[i];
        std::string room_text = std::string(room.room_name) + " - " +
                               std::to_string(room.current_players) + "/" +
                               std::to_string(room.max_players) + " players";

        if (room.is_private) {
            room_text += " [PRIVATE]";
        }

        UILabel room_label(100, start_y + i * 50, room_text, 20);
        room_label.draw(graphics);
    }

    for (auto& button : buttons_) {
        button->draw(graphics);
    }

    for (auto& button : room_join_buttons_) {
        button->draw(graphics);
    }
}

void BrowseRoomsScreen::set_room_list(const std::vector<protocol::RoomInfo>& rooms) {
    available_rooms_ = rooms;
    create_room_join_buttons();
}

void BrowseRoomsScreen::create_room_join_buttons() {
    room_join_buttons_.clear();

    float start_y = 200.0f;
    float button_width = 100.0f;
    float button_height = 40.0f;

    for (size_t i = 0; i < available_rooms_.size() && i < 10; ++i) {
        const auto& room = available_rooms_[i];

        auto join_btn = std::make_unique<UIButton>(
            screen_width_ - 250, start_y + i * 50 - 5, button_width, button_height, "Join");

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
