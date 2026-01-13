#include "screens/createroom/CreateRoomInput.hpp"
#include "screens/createroom/CreateRoomConfig.hpp"
#include <cmath>
#include <iostream>

namespace rtype::client::createroom {

bool InputHandler::is_point_in_circle(
    float mouse_x, float mouse_y,
    float circle_x, float circle_y,
    float radius
) {
    float dx = mouse_x - circle_x;
    float dy = mouse_y - circle_y;
    float distance = std::sqrt(dx * dx + dy * dy);
    return distance <= radius;
}

bool InputHandler::handle_map_click(
    engine::IInputPlugin* input,
    float screen_width,
    size_t& current_map_index,
    size_t num_maps
) {
    float center_x = screen_width / 2.0f;
    float size = Config::DIFFICULTY_CIRCLE_SIZE;  // Same size as difficulty
    float spacing = Config::DIFFICULTY_CIRCLE_SPACING;

    // Support up to 3 maps
    size_t maps_to_show = std::min(num_maps, static_cast<size_t>(3));
    if (maps_to_show == 0) return false;

    float total_width = size * maps_to_show + spacing * (maps_to_show - 1);
    float start_x = center_x - total_width / 2.0f;

    engine::Vector2f mouse_pos = input->get_mouse_position();
    bool mouse_clicked = input->is_mouse_button_pressed(engine::MouseButton::Left);

    if (!mouse_clicked) {
        return false;
    }

    for (size_t i = 0; i < maps_to_show; ++i) {
        float x = start_x + i * (size + spacing);
        float y = Config::CONTENT_START_Y;
        float circle_x = x + size / 2.0f;
        float circle_y = y + size / 2.0f;
        float radius = size / 2.0f;

        if (is_point_in_circle(mouse_pos.x, mouse_pos.y, circle_x, circle_y, radius)) {
            current_map_index = i;
            std::cout << "[CreateRoomInput] Selected Map Index: " << i << "\n";
            return true;
        }
    }

    return false;
}

bool InputHandler::handle_difficulty_click(
    engine::IInputPlugin* input,
    float screen_width,
    protocol::Difficulty& current_difficulty
) {
    float center_x = screen_width / 2.0f;
    float size = Config::DIFFICULTY_CIRCLE_SIZE;
    float spacing = Config::DIFFICULTY_CIRCLE_SPACING;
    float total_width = size * 3 + spacing * 2;
    float start_x = center_x - total_width / 2.0f;

    protocol::Difficulty difficulties[] = {
        protocol::Difficulty::EASY,
        protocol::Difficulty::NORMAL,
        protocol::Difficulty::HARD
    };

    engine::Vector2f mouse_pos = input->get_mouse_position();
    bool mouse_clicked = input->is_mouse_button_pressed(engine::MouseButton::Left);

    if (!mouse_clicked) {
        return false;
    }

    for (int i = 0; i < 3; ++i) {
        float x = start_x + i * (size + spacing);
        float y = Config::CONTENT_START_Y;
        float circle_x = x + size / 2.0f;
        float circle_y = y + size / 2.0f;
        float radius = size / 2.0f;

        if (is_point_in_circle(mouse_pos.x, mouse_pos.y, circle_x, circle_y, radius)) {
            current_difficulty = difficulties[i];
            std::cout << "[CreateRoomInput] Selected Difficulty: " << static_cast<int>(current_difficulty) << "\n";
            return true;
        }
    }

    return false;
}

bool InputHandler::handle_gamemode_click(
    engine::IInputPlugin* input,
    float screen_width,
    protocol::GameMode& current_mode
) {
    float center_x = screen_width / 2.0f;
    float size = Config::GAMEMODE_CIRCLE_SIZE;
    float spacing = Config::GAMEMODE_CIRCLE_SPACING;
    float total_width = size * 3 + spacing * 2;
    float start_x = center_x - total_width / 2.0f;

    protocol::GameMode modes[] = {
        protocol::GameMode::DUO,
        protocol::GameMode::TRIO,
        protocol::GameMode::SQUAD
    };

    engine::Vector2f mouse_pos = input->get_mouse_position();
    bool mouse_clicked = input->is_mouse_button_pressed(engine::MouseButton::Left);

    if (!mouse_clicked) {
        return false;
    }

    for (int i = 0; i < 3; ++i) {
        float x = start_x + i * (size + spacing);
        float y = Config::CONTENT_START_Y;
        float circle_x = x + size / 2.0f;
        float circle_y = y + size / 2.0f;
        float radius = size / 2.0f;

        if (is_point_in_circle(mouse_pos.x, mouse_pos.y, circle_x, circle_y, radius)) {
            current_mode = modes[i];
            std::cout << "[CreateRoomInput] Selected GameMode: " << static_cast<int>(current_mode) << "\n";
            return true;
        }
    }

    return false;
}

}  // namespace rtype::client::createroom
