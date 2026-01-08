#include "screens/CreateRoomScreen.hpp"
#include "NetworkClient.hpp"
#include "ScreenManager.hpp"
#include "systems/MapConfigLoader.hpp"
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
    map_buttons_.clear();
    difficulty_buttons_.clear();

    // Load available maps from index.json
    available_maps_ = rtype::MapConfigLoader::loadMapIndex("assets/maps/index.json");
    if (available_maps_.empty()) {
        std::cerr << "[CreateRoomScreen] No maps found in index.json!\n";
    } else {
        selected_map_id_ = available_maps_[0].id;
        selected_map_index_ = 0;
        std::cout << "[CreateRoomScreen] Loaded " << available_maps_.size() << " maps\n";
    }

    // Reset to defaults
    game_mode_ = protocol::GameMode::SQUAD;
    difficulty_ = protocol::Difficulty::NORMAL;

    float center_x = screen_width_ / 2.0f;
    float start_y = 80.0f;

    // Title
    auto title = std::make_unique<UILabel>(center_x, start_y, "Create Room", 36);
    title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(title));

    // Room Name field
    auto name_label = std::make_unique<UILabel>(center_x - 200, start_y + 70, "Room Name:", 18);
    labels_.push_back(std::move(name_label));

    auto name_field = std::make_unique<UITextField>(center_x - 200, start_y + 95, 400, 35, "My Room");
    fields_.push_back(std::move(name_field));

    // Password field (optional)
    auto pass_label = std::make_unique<UILabel>(center_x - 200, start_y + 145, "Password (optional):", 18);
    labels_.push_back(std::move(pass_label));

    auto pass_field = std::make_unique<UITextField>(center_x - 200, start_y + 170, 400, 35, "");
    pass_field->set_password_mode(true);
    fields_.push_back(std::move(pass_field));

    // ==================== MAP SELECTION ====================
    auto map_label = std::make_unique<UILabel>(center_x, start_y + 220, "Select Map:", 20);
    map_label->set_alignment(UILabel::Alignment::CENTER);
    map_label->set_color(engine::Color{100, 200, 255, 255});
    labels_.push_back(std::move(map_label));

    float map_button_y = start_y + 250;
    float map_button_width = 180.0f;
    float map_button_height = 40.0f;
    float map_button_spacing = 15.0f;
    
    // Dynamically create map buttons based on available maps
    size_t num_maps = available_maps_.size();
    if (num_maps > 0) {
        float map_total_width = map_button_width * static_cast<float>(num_maps) + map_button_spacing * static_cast<float>(num_maps - 1);
        float map_start_x = center_x - map_total_width / 2.0f;
        
        for (size_t i = 0; i < num_maps; ++i) {
            const auto& mapInfo = available_maps_[i];
            float btn_x = map_start_x + static_cast<float>(i) * (map_button_width + map_button_spacing);
            
            auto map_btn = std::make_unique<UIButton>(btn_x, map_button_y, map_button_width, map_button_height, mapInfo.name.c_str());
            
            // Capture index by value for the callback
            size_t map_index = i;
            map_btn->set_on_click([this, map_index]() {
                selected_map_index_ = map_index;
                selected_map_id_ = available_maps_[map_index].id;
                std::cout << "[CreateRoomScreen] Selected Map: " << available_maps_[map_index].name << "\n";
            });
            map_buttons_.push_back(std::move(map_btn));
        }
    }

    // ==================== DIFFICULTY SELECTION ====================
    auto diff_label = std::make_unique<UILabel>(center_x, start_y + 310, "Difficulty:", 20);
    diff_label->set_alignment(UILabel::Alignment::CENTER);
    diff_label->set_color(engine::Color{255, 200, 100, 255});
    labels_.push_back(std::move(diff_label));

    float diff_button_y = start_y + 340;
    float diff_button_width = 120.0f;
    float diff_button_height = 40.0f;
    float diff_button_spacing = 15.0f;
    float diff_total_width = diff_button_width * 3 + diff_button_spacing * 2;
    float diff_start_x = center_x - diff_total_width / 2.0f;

    // Easy
    auto easy_btn = std::make_unique<UIButton>(diff_start_x, diff_button_y, diff_button_width, diff_button_height, "Easy");
    easy_btn->set_on_click([this]() {
        difficulty_ = protocol::Difficulty::EASY;
        std::cout << "[CreateRoomScreen] Selected Difficulty: Easy\n";
    });
    difficulty_buttons_.push_back(std::move(easy_btn));

    // Normal
    auto normal_btn = std::make_unique<UIButton>(diff_start_x + diff_button_width + diff_button_spacing, diff_button_y,
                                                  diff_button_width, diff_button_height, "Normal");
    normal_btn->set_on_click([this]() {
        difficulty_ = protocol::Difficulty::NORMAL;
        std::cout << "[CreateRoomScreen] Selected Difficulty: Normal\n";
    });
    difficulty_buttons_.push_back(std::move(normal_btn));

    // Hard
    auto hard_btn = std::make_unique<UIButton>(diff_start_x + (diff_button_width + diff_button_spacing) * 2, diff_button_y,
                                                diff_button_width, diff_button_height, "Hard");
    hard_btn->set_on_click([this]() {
        difficulty_ = protocol::Difficulty::HARD;
        std::cout << "[CreateRoomScreen] Selected Difficulty: Hard\n";
    });
    difficulty_buttons_.push_back(std::move(hard_btn));

    // ==================== GAME MODE SELECTION ====================
    auto mode_label = std::make_unique<UILabel>(center_x, start_y + 400, "Game Mode:", 20);
    mode_label->set_alignment(UILabel::Alignment::CENTER);
    mode_label->set_color(engine::Color{200, 200, 200, 255});
    labels_.push_back(std::move(mode_label));

    float mode_button_y = start_y + 430;
    float button_width = 120.0f;
    float button_height = 40.0f;
    float button_spacing = 15.0f;
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

    // ==================== CREATE / BACK BUTTONS ====================
    float action_button_width = 200.0f;
    float action_button_height = 50.0f;
    float action_y = start_y + 500;

    auto create_btn = std::make_unique<UIButton>(
        center_x - action_button_width - 10, action_y, action_button_width, action_button_height, "Create");
    create_btn->set_on_click([this]() {
        std::string room_name = fields_[0]->get_text();
        std::string password = fields_[1]->get_text();

        if (room_name.empty()) {
            room_name = "Room";
        }

        uint8_t max_players = get_configured_max_players();
        uint16_t map_index = get_configured_map_index();

        std::string map_name = available_maps_.empty() ? "Unknown" : available_maps_[selected_map_index_].name;
        std::cout << "[CreateRoomScreen] Creating room on map: " << map_name << "\n";

        network_client_.send_create_room(room_name, password,
                                         game_mode_, difficulty_,
                                         map_index, max_players);

        if (on_room_created_) {
            on_room_created_(game_mode_, max_players);
        }
    });
    buttons_.push_back(std::move(create_btn));

    // Back button
    auto back_btn = std::make_unique<UIButton>(
        center_x + 10, action_y, action_button_width, action_button_height, "Back");
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
        // Update map selection buttons
        for (size_t i = 0; i < map_buttons_.size(); ++i) {
            bool is_selected = (selected_map_index_ == i);
            map_buttons_[i]->set_selected(is_selected);
            map_buttons_[i]->update(graphics, input);
        }

        // Update difficulty buttons
        for (size_t i = 0; i < difficulty_buttons_.size(); ++i) {
            bool is_selected = (static_cast<uint8_t>(difficulty_) == i + 1);
            difficulty_buttons_[i]->set_selected(is_selected);
            difficulty_buttons_[i]->update(graphics, input);
        }

        // Update game mode buttons
        for (size_t i = 0; i < mode_buttons_.size(); ++i) {
            bool is_selected = (static_cast<uint8_t>(game_mode_) == i + 1);
            mode_buttons_[i]->set_selected(is_selected);
            mode_buttons_[i]->update(graphics, input);
        }

        // Update action buttons
        for (auto& button : buttons_) {
            button->update(graphics, input);
        }
    }
}

void CreateRoomScreen::draw(engine::IGraphicsPlugin* graphics) {
    graphics->clear(engine::Color{20, 20, 30, 255});

    // Draw labels
    for (auto& label : labels_) {
        label->draw(graphics);
    }

    // Draw text fields
    for (auto& field : fields_) {
        field->draw(graphics);
    }

    // Draw map selection buttons
    for (auto& button : map_buttons_) {
        button->draw(graphics);
    }

    // Draw difficulty buttons
    for (auto& button : difficulty_buttons_) {
        button->draw(graphics);
    }

    // Draw game mode buttons
    for (auto& button : mode_buttons_) {
        button->draw(graphics);
    }

    // Draw action buttons
    for (auto& button : buttons_) {
        button->draw(graphics);
    }
}

}  // namespace rtype::client
