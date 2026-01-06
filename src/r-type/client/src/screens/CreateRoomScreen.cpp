#include "screens/CreateRoomScreen.hpp"
#include "NetworkClient.hpp"
#include "ScreenManager.hpp"
#include <iostream>

namespace rtype::client {

CreateRoomScreen::CreateRoomScreen(NetworkClient& network_client, int screen_width, int screen_height)
    : BaseScreen(network_client, screen_width, screen_height) {
}

void CreateRoomScreen::initialize() {
    labels_.clear();
    fields_.clear();
    buttons_.clear();
    max_players_ = 4;  // Reset to default

    float center_x = screen_width_ / 2.0f;
    float start_y = 150.0f;

    // Title
    auto title = std::make_unique<UILabel>(center_x, start_y, "Create Room", 36);
    title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(title));

    // Room Name field
    auto name_label = std::make_unique<UILabel>(center_x - 200, start_y + 100, "Room Name:", 20);
    labels_.push_back(std::move(name_label));

    auto name_field = std::make_unique<UITextField>(center_x - 200, start_y + 130, 400, 40, "My Room");
    fields_.push_back(std::move(name_field));

    // Password field (optional)
    auto pass_label = std::make_unique<UILabel>(center_x - 200, start_y + 190, "Password (optional):", 20);
    labels_.push_back(std::move(pass_label));

    auto pass_field = std::make_unique<UITextField>(center_x - 200, start_y + 220, 400, 40, "");
    pass_field->set_password_mode(true);
    fields_.push_back(std::move(pass_field));

    // Max players selector
    float selector_y = start_y + 290;

    // Decrease button
    auto decrease_btn = std::make_unique<UIButton>(center_x - 80, selector_y, 40, 40, "-");
    decrease_btn->set_on_click([this]() {
        if (max_players_ > 2) {
            max_players_--;
        }
    });
    buttons_.push_back(std::move(decrease_btn));

    // Increase button
    auto increase_btn = std::make_unique<UIButton>(center_x + 40, selector_y, 40, 40, "+");
    increase_btn->set_on_click([this]() {
        if (max_players_ < 4) {
            max_players_++;
        }
    });
    buttons_.push_back(std::move(increase_btn));

    // Create button
    float button_width = 200.0f;
    float button_height = 50.0f;

    auto create_btn = std::make_unique<UIButton>(
        center_x - button_width - 10, start_y + 360, button_width, button_height, "Create");
    create_btn->set_on_click([this]() {
        std::string room_name = fields_[0]->get_text();
        std::string password = fields_[1]->get_text();

        if (room_name.empty()) {
            room_name = "Room";
        }

        network_client_.send_create_room(room_name, password,
                                         protocol::GameMode::SQUAD,
                                         protocol::Difficulty::NORMAL, max_players_);

        if (on_room_created_) {
            on_room_created_(max_players_);
        }
    });
    buttons_.push_back(std::move(create_btn));

    // Back button
    auto back_btn = std::make_unique<UIButton>(
        center_x + 10, start_y + 360, button_width, button_height, "Back");
    back_btn->set_on_click([this]() {
        if (on_screen_change_) {
            on_screen_change_(GameScreen::MAIN_MENU);
        }
    });
    buttons_.push_back(std::move(back_btn));
}

void CreateRoomScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    // Update text fields first
    for (auto& field : fields_) {
        field->update(graphics, input);
    }

    // Update buttons only if no text field is focused
    bool any_field_focused = false;
    for (auto& field : fields_) {
        if (field->is_focused()) {
            any_field_focused = true;
            break;
        }
    }

    if (!any_field_focused) {
        for (auto& button : buttons_) {
            button->update(graphics, input);
        }
    }
}

void CreateRoomScreen::draw(engine::IGraphicsPlugin* graphics) {
    graphics->clear(engine::Color{20, 20, 30, 255});

    for (auto& label : labels_) {
        label->draw(graphics);
    }
    for (auto& field : fields_) {
        field->draw(graphics);
    }

    // Draw max players selector label
    float center_x = screen_width_ / 2.0f;
    float start_y = 150.0f;
    float selector_y = start_y + 290;

    UILabel max_players_text(center_x, selector_y - 25, "Max players:", 20);
    max_players_text.set_alignment(UILabel::Alignment::CENTER);
    max_players_text.set_color(engine::Color{200, 200, 200, 255});
    max_players_text.draw(graphics);

    UILabel max_players_value(center_x, selector_y + 10,
                              std::to_string(max_players_), 24);
    max_players_value.set_alignment(UILabel::Alignment::CENTER);
    max_players_value.set_color(engine::Color{255, 200, 100, 255});
    max_players_value.draw(graphics);

    for (auto& button : buttons_) {
        button->draw(graphics);
    }
}

}  // namespace rtype::client
