#include "screens/CreateRoomScreen.hpp"
#include "NetworkClient.hpp"
#include "ScreenManager.hpp"
#include <iostream>

namespace rtype::client {

CreateRoomScreen::CreateRoomScreen(NetworkClient& network_client, int screen_width, int screen_height)
    : BaseScreen(network_client, screen_width, screen_height) {
}

uint8_t CreateRoomScreen::get_configured_max_players() const {
    switch (game_mode_) {
        case protocol::GameMode::DUO:
            return 2;
        case protocol::GameMode::TRIO:
            return 3;
        case protocol::GameMode::SQUAD:
            return 4;
        default:
            return 4;
    }
}

void CreateRoomScreen::initialize() {
    labels_.clear();
    fields_.clear();
    buttons_.clear();
    mode_buttons_.clear();
    game_mode_ = protocol::GameMode::SQUAD;  // Reset to default

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

    // Game Mode selector - Label
    auto mode_label = std::make_unique<UILabel>(center_x, start_y + 280, "Game Mode:", 20);
    mode_label->set_alignment(UILabel::Alignment::CENTER);
    mode_label->set_color(engine::Color{200, 200, 200, 255});
    labels_.push_back(std::move(mode_label));

    // Game Mode buttons (DUO/TRIO/SQUAD)
    float mode_button_y = start_y + 315;
    float button_width = 120.0f;
    float button_height = 45.0f;
    float button_spacing = 10.0f;
    float total_width = button_width * 3 + button_spacing * 2;
    float start_x = center_x - total_width / 2.0f;

    // DUO button (2 players)
    auto duo_btn = std::make_unique<UIButton>(start_x, mode_button_y, button_width, button_height, "DUO (2)");
    duo_btn->set_on_click([this]() {
        game_mode_ = protocol::GameMode::DUO;
        std::cout << "[CreateRoomScreen] Selected DUO mode (2 players)\n";
    });
    mode_buttons_.push_back(std::move(duo_btn));

    // TRIO button (3 players)
    auto trio_btn = std::make_unique<UIButton>(start_x + button_width + button_spacing, mode_button_y,
                                                button_width, button_height, "TRIO (3)");
    trio_btn->set_on_click([this]() {
        game_mode_ = protocol::GameMode::TRIO;
        std::cout << "[CreateRoomScreen] Selected TRIO mode (3 players)\n";
    });
    mode_buttons_.push_back(std::move(trio_btn));

    // SQUAD button (4 players)
    auto squad_btn = std::make_unique<UIButton>(start_x + (button_width + button_spacing) * 2, mode_button_y,
                                                 button_width, button_height, "SQUAD (4)");
    squad_btn->set_on_click([this]() {
        game_mode_ = protocol::GameMode::SQUAD;
        std::cout << "[CreateRoomScreen] Selected SQUAD mode (4 players)\n";
    });
    mode_buttons_.push_back(std::move(squad_btn));

    // Create and Back buttons
    float action_button_width = 200.0f;
    float action_button_height = 50.0f;

    auto create_btn = std::make_unique<UIButton>(
        center_x - action_button_width - 10, start_y + 390, action_button_width, action_button_height, "Create");
    create_btn->set_on_click([this]() {
        std::string room_name = fields_[0]->get_text();
        std::string password = fields_[1]->get_text();

        if (room_name.empty()) {
            room_name = "Room";
        }

        uint8_t max_players = get_configured_max_players();

        network_client_.send_create_room(room_name, password,
                                         game_mode_,
                                         protocol::Difficulty::NORMAL, max_players);

        if (on_room_created_) {
            on_room_created_(game_mode_, max_players);
        }
    });
    buttons_.push_back(std::move(create_btn));

    // Back button
    auto back_btn = std::make_unique<UIButton>(
        center_x + 10, start_y + 390, action_button_width, action_button_height, "Back");
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
        for (auto& button : mode_buttons_) {
            button->update(graphics, input);
        }
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

    // Draw mode buttons
    for (auto& button : mode_buttons_) {
        button->draw(graphics);
    }

    for (auto& button : buttons_) {
        button->draw(graphics);
    }
}

}  // namespace rtype::client
