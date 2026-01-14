#include "screens/createroom/CreateRoomDrawer.hpp"
#include "screens/createroom/CreateRoomConfig.hpp"

namespace rtype::client::createroom {

void Drawer::draw_room_info_step(
    const std::vector<std::unique_ptr<UILabel>>& labels,
    const std::vector<std::unique_ptr<UITextField>>& fields,
    engine::IGraphicsPlugin* graphics
) {
    // Draw labels
    for (size_t i = 0; i < 2 && i < labels.size(); ++i) {
        labels[i]->draw(graphics);
    }
    // Draw text fields
    for (const auto& field : fields) {
        field->draw(graphics);
    }
}



void Drawer::draw_map_selection_step(
    const std::vector<engine::TextureHandle>& map_thumbnails,
    size_t selected_map_index,
    float screen_width,
    engine::IGraphicsPlugin* graphics
) {
    Renderer::draw_map_selection(graphics, map_thumbnails, screen_width, selected_map_index);
}

void Drawer::draw_difficulty_step(
    const TexturePack& textures,
    protocol::Difficulty selected_difficulty,
    float screen_width,
    engine::IGraphicsPlugin* graphics
) {
    Renderer::draw_difficulty_selection(graphics, textures, screen_width, selected_difficulty);
}

void Drawer::draw_game_mode_step(
    const TexturePack& textures,
    protocol::GameMode selected_mode,
    float screen_width,
    engine::IGraphicsPlugin* graphics
) {
    Renderer::draw_gamemode_selection(graphics, textures, screen_width, selected_mode);
}

void Drawer::draw_navigation_buttons(
    std::vector<std::unique_ptr<UIButton>>& nav_buttons,
    int current_step_index,
    int total_steps,
    engine::IGraphicsPlugin* graphics,
    int screen_width,
    int screen_height,
    float button_radius,
    float button_spacing,
    float button_y_offset,
    bool edit_mode
) {
    // Circular button parameters (now dynamic from edit mode)
    float center_x = screen_width / 2.0f;
    float button_y = screen_height - button_y_offset;

    // Update UIButton positions and sizes to match circular buttons for click detection
    float button_diameter = button_radius * 2.0f;

    if (nav_buttons.size() > 0) {
        // Previous button (left)
        float prev_x = center_x - button_spacing;
        float prev_y = button_y;
        nav_buttons[0]->set_position(
            prev_x - button_radius,  // Center the clickable area
            prev_y - button_radius
        );
        nav_buttons[0]->set_size(button_diameter, button_diameter);
    }

    if (nav_buttons.size() > 1) {
        // Next button (right)
        float next_x = center_x + button_spacing;
        float next_y = button_y;
        nav_buttons[1]->set_position(
            next_x - button_radius,  // Center the clickable area
            next_y - button_radius
        );
        nav_buttons[1]->set_size(button_diameter, button_diameter);
    }

    // Determine which buttons to show
    bool show_previous = true;  // Always show previous button (goes to menu on step 0)
    bool show_next = true;  // Always show next/create button

    // Check if buttons are hovered
    bool previous_hovered = show_previous && nav_buttons.size() > 0 && nav_buttons[0]->is_hovered();
    bool next_hovered = nav_buttons.size() > 1 && nav_buttons[1]->is_hovered();

    // Draw circular navigation buttons with arrows
    Renderer::draw_navigation_buttons(
        graphics,
        screen_width,
        screen_height,
        show_previous,
        show_next,
        previous_hovered,
        next_hovered,
        button_radius,
        button_spacing,
        button_y_offset
    );

    // Draw edit mode overlay
    if (edit_mode) {
        // Draw semi-transparent overlay with instructions
        engine::Rectangle overlay{0, 0, static_cast<float>(screen_width), 80.0f};
        graphics->draw_rectangle(overlay, {0, 0, 0, 200});

        // Draw instructions
        engine::Vector2f instr_pos{10, 10};
        graphics->draw_text("EDIT MODE: Arrows=Spacing | W/S=Vertical | +/-=Size | P=Print | E=Exit",
                          instr_pos, engine::Color{255, 255, 0, 255}, engine::INVALID_HANDLE, 20);

        // Draw current values
        engine::Vector2f values_pos{10, 40};
        std::string values_text = "Spacing: " + std::to_string(static_cast<int>(button_spacing)) +
                                 " | Y-Offset: " + std::to_string(static_cast<int>(button_y_offset)) +
                                 " | Radius: " + std::to_string(static_cast<int>(button_radius));
        graphics->draw_text(values_text, values_pos, engine::Color{255, 255, 255, 255}, engine::INVALID_HANDLE, 18);

        // Draw crosshairs on arrow centers for precision
        float prev_x = center_x - button_spacing;
        float next_x = center_x + button_spacing;

        // Previous arrow crosshair
        graphics->draw_line({prev_x - 10, button_y}, {prev_x + 10, button_y}, {255, 0, 0, 255}, 2.0f);
        graphics->draw_line({prev_x, button_y - 10}, {prev_x, button_y + 10}, {255, 0, 0, 255}, 2.0f);

        // Next arrow crosshair
        graphics->draw_line({next_x - 10, button_y}, {next_x + 10, button_y}, {255, 0, 0, 255}, 2.0f);
        graphics->draw_line({next_x, button_y - 10}, {next_x, button_y + 10}, {255, 0, 0, 255}, 2.0f);
    }
}

}  // namespace rtype::client::createroom
