#include "screens/createroom/CreateRoomInitializer.hpp"
#include "screens/createroom/CreateRoomConfig.hpp"
#include <iostream>

namespace rtype::client::createroom {

void Initializer::init_room_info_step(
    std::vector<std::unique_ptr<UILabel>>& labels,
    std::vector<std::unique_ptr<UITextField>>& fields,
    float screen_width
) {
    float center_x = screen_width / 2.0f;
    float content_start_y = Config::CONTENT_START_Y;

    // Large, modern text fields with rounded corners
    float field_width = 700.0f;   // Increased from 600
    float field_height = 85.0f;   // Increased from 70
    float vertical_spacing = 160.0f;  // Increased spacing

    // Room Name field
    auto name_label = std::make_unique<UILabel>(center_x - field_width / 2, content_start_y, "Room Name:", 32);
    name_label->set_color(engine::Color{140, 180, 255, 255});
    labels.push_back(std::move(name_label));

    auto name_field = std::make_unique<UITextField>(center_x - field_width / 2, content_start_y + 50, field_width, field_height, "My Room");
    fields.push_back(std::move(name_field));

    // Password field (optional)
    auto pass_label = std::make_unique<UILabel>(center_x - field_width / 2, content_start_y + vertical_spacing, "Password (optional):", 32);
    pass_label->set_color(engine::Color{140, 180, 255, 255});
    labels.push_back(std::move(pass_label));

    auto pass_field = std::make_unique<UITextField>(center_x - field_width / 2, content_start_y + vertical_spacing + 50, field_width, field_height, "");
    pass_field->set_password_mode(true);
    fields.push_back(std::move(pass_field));
}

void Initializer::init_map_selection_step(
    std::vector<std::unique_ptr<UIButton>>& map_buttons,
    rtype::client::MapId& map_id,
    float screen_width
) {
    float center_x = screen_width / 2.0f;
    float content_start_y = Config::CONTENT_START_Y;

    float map_button_width = Config::MAP_BUTTON_WIDTH;
    float map_button_height = Config::MAP_BUTTON_HEIGHT;
    float map_button_spacing = 50.0f;  // Increased spacing between buttons
    float map_button_y = content_start_y + Config::MAP_PREVIEW_HEIGHT + 40.0f;  // 40px below map previews
    float map_total_width = map_button_width * 3 + map_button_spacing * 2;
    float map_start_x = center_x - map_total_width / 2.0f;

    // Map 1 - Nebula Outpost
    auto map1_btn = std::make_unique<UIButton>(map_start_x, map_button_y, map_button_width, map_button_height, "Nebula Outpost");
    map1_btn->set_on_click([&map_id]() {
        map_id = rtype::client::MapId::NEBULA_OUTPOST;
        std::cout << "[CreateRoom] Selected Map: Nebula Outpost\n";
    });
    map_buttons.push_back(std::move(map1_btn));

    // Map 2 - Asteroid Belt
    auto map2_btn = std::make_unique<UIButton>(map_start_x + map_button_width + map_button_spacing, map_button_y,
                                                map_button_width, map_button_height, "Asteroid Belt");
    map2_btn->set_on_click([&map_id]() {
        map_id = rtype::client::MapId::ASTEROID_BELT;
        std::cout << "[CreateRoom] Selected Map: Asteroid Belt\n";
    });
    map_buttons.push_back(std::move(map2_btn));

    // Map 3 - Bydo Mothership
    auto map3_btn = std::make_unique<UIButton>(map_start_x + (map_button_width + map_button_spacing) * 2, map_button_y,
                                                map_button_width, map_button_height, "Bydo Mothership");
    map3_btn->set_on_click([&map_id]() {
        map_id = rtype::client::MapId::BYDO_MOTHERSHIP;
        std::cout << "[CreateRoom] Selected Map: Bydo Mothership\n";
    });
    map_buttons.push_back(std::move(map3_btn));
}

void Initializer::init_difficulty_step() {
    // Difficulty uses circular clickable images - no buttons needed
    // Click detection is handled in Updater
}

void Initializer::init_game_mode_step() {
    // Game mode uses circular clickable images - no buttons needed
    // Click detection is handled in Updater
}

void Initializer::init_navigation_buttons(
    std::vector<std::unique_ptr<UIButton>>& nav_buttons,
    std::function<void()> on_previous,
    std::function<void()> on_next,
    float screen_width,
    float screen_height
) {
    float center_x = screen_width / 2.0f;
    float nav_button_width = Config::NAV_BUTTON_WIDTH;
    float nav_button_height = Config::NAV_BUTTON_HEIGHT;
    float nav_y = screen_height - Config::NAV_BUTTON_Y_OFFSET;
    float nav_spacing = Config::NAV_BUTTON_SPACING;

    // Previous/Back button (left)
    auto prev_btn = std::make_unique<UIButton>(
        center_x - nav_button_width - nav_spacing / 2, nav_y, nav_button_width, nav_button_height, "Previous");
    prev_btn->set_on_click(on_previous);
    nav_buttons.push_back(std::move(prev_btn));

    // Next/Create button (right)
    auto next_btn = std::make_unique<UIButton>(
        center_x + nav_spacing / 2, nav_y, nav_button_width, nav_button_height, "Next");
    next_btn->set_on_click(on_next);
    nav_buttons.push_back(std::move(next_btn));
}

}  // namespace rtype::client::createroom
