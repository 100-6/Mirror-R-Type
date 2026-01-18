#include "screens/createroom/CreateRoomRenderer.hpp"
#include "screens/createroom/CreateRoomConfig.hpp"
#include "ui/UILabel.hpp"

namespace rtype::client::createroom {

// ============================================================================
// TEXTURE PACK
// ============================================================================

void TexturePack::load(engine::IGraphicsPlugin* graphics) {
    if (loaded) return;

    // Load background texture
    menu_background = graphics->load_texture(Config::MENU_BACKGROUND);

    // Load map textures
    map_nebula = graphics->load_texture(Config::MAP_NEBULA);
    map_asteroid = graphics->load_texture(Config::MAP_ASTEROID);
    map_bydo = graphics->load_texture(Config::MAP_BYDO);

    // Load difficulty textures
    difficulty_easy = graphics->load_texture(Config::DIFFICULTY_EASY);
    difficulty_medium = graphics->load_texture(Config::DIFFICULTY_MEDIUM);
    difficulty_hard = graphics->load_texture(Config::DIFFICULTY_HARD);

    // Load game mode textures
    gamemode_duo = graphics->load_texture(Config::GAMEMODE_DUO);
    gamemode_trio = graphics->load_texture(Config::GAMEMODE_TRIO);
    gamemode_squad = graphics->load_texture(Config::GAMEMODE_SQUAD);

    loaded = true;
}

// ============================================================================
// RENDERER
// ============================================================================

void Renderer::draw_background(
    engine::IGraphicsPlugin* graphics,
    const TexturePack& textures,
    int screen_width,
    int screen_height
) {
    // Clear with dark background as fallback
    graphics->clear(Config::BG_DARK());

    // Draw menu background image stretched to fill screen
    engine::Sprite background_sprite;
    background_sprite.texture_handle = textures.menu_background;
    background_sprite.size = {static_cast<float>(screen_width), static_cast<float>(screen_height)};
    background_sprite.origin = {0.0f, 0.0f};
    background_sprite.tint = {255, 255, 255, 255};

    graphics->draw_sprite(background_sprite, {0.0f, 0.0f});
}

void Renderer::draw_stepper(
    engine::IGraphicsPlugin* graphics,
    int screen_width,
    int current_step_index,
    const char* step_title,
    int total_steps
) {
    float center_x = screen_width / 2.0f;
    float stepper_y = Config::STEPPER_Y;

    // R-Type Evolution style - larger circular nodes
    float node_radius = 22.0f;
    float connector_width = 140.0f;
    float connector_height = 4.0f;

    float total_width = (node_radius * 2) * total_steps + connector_width * (total_steps - 1);
    float start_x = center_x - total_width / 2.0f + node_radius;

    // Draw connectors between nodes first (behind the nodes)
    for (int i = 0; i < total_steps - 1; ++i) {
        float node_x = start_x + i * (node_radius * 2 + connector_width);
        float connector_x = node_x + node_radius;
        float connector_y = stepper_y + node_radius - connector_height / 2.0f;

        // Determine connector color based on progress
        engine::Color connector_color;
        if (i < current_step_index) {
            // Completed - bright purple gradient
            connector_color = {140, 100, 255, 255};
        } else {
            // Future - dim gray
            connector_color = {60, 60, 80, 180};
        }

        // Draw connector line
        engine::Rectangle connector{connector_x, connector_y, connector_width, connector_height};
        graphics->draw_rectangle(connector, connector_color);

        // Add glow to completed connectors
        if (i < current_step_index) {
            engine::Rectangle glow_top{connector_x, connector_y - 2, connector_width, 1.0f};
            graphics->draw_rectangle(glow_top, {180, 140, 255, 150});
            engine::Rectangle glow_bottom{connector_x, connector_y + connector_height + 1, connector_width, 1.0f};
            graphics->draw_rectangle(glow_bottom, {180, 140, 255, 150});
        }
    }

    // Draw step nodes
    for (int i = 0; i < total_steps; ++i) {
        float node_x = start_x + i * (node_radius * 2 + connector_width);
        float node_y = stepper_y + node_radius;

        bool is_completed = i < current_step_index;
        bool is_current = i == current_step_index;
        bool is_future = i > current_step_index;

        // Draw shadow
        engine::Vector2f shadow_center{node_x + 3, node_y + 3};
        graphics->draw_circle(shadow_center, node_radius, {0, 0, 0, 150});

        // Draw outer glow for current step
        if (is_current) {
            for (int j = 4; j > 0; j--) {
                float glow_radius = node_radius + 8.0f * j;
                engine::Color glow_color{160, 100, 255, static_cast<unsigned char>(50 / j)};
                graphics->draw_circle({node_x, node_y}, glow_radius, glow_color);
            }
        }

        // Draw outer ring/border
        engine::Color border_color;
        float border_radius = node_radius + 3.0f;
        if (is_completed) {
            border_color = {140, 100, 255, 255};  // Purple for completed
        } else if (is_current) {
            border_color = {180, 120, 255, 255};  // Bright purple for current
        } else {
            border_color = {70, 70, 90, 200};     // Dark gray for future
        }
        graphics->draw_circle({node_x, node_y}, border_radius, border_color);

        // Draw inner circle background
        engine::Color bg_color;
        if (is_completed) {
            bg_color = {80, 50, 120, 255};        // Dark purple filled
        } else if (is_current) {
            bg_color = {100, 60, 160, 255};       // Medium purple filled
        } else {
            bg_color = {30, 30, 45, 255};         // Very dark gray
        }
        graphics->draw_circle({node_x, node_y}, node_radius, bg_color);

        // Draw inner glow/gradient effect
        if (is_completed || is_current) {
            engine::Color inner_glow = is_current ?
                engine::Color{140, 90, 200, 120} :
                engine::Color{110, 70, 160, 100};
            graphics->draw_circle({node_x, node_y - node_radius / 3.0f}, node_radius * 0.6f, inner_glow);
        }

        // Draw check mark for completed steps or number
        std::string step_text;
        engine::Color text_color;
        int font_size = 24;

        if (is_completed) {
            step_text = "*";  // Simple V for checkmark (always recognized)
            text_color = {200, 180, 255, 255};
            font_size = 28;
        } else {
            step_text = std::to_string(i + 1);
            text_color = is_current ?
                engine::Color{230, 210, 255, 255} :
                engine::Color{120, 120, 150, 200};
        }

        // Center the text in the circle
        float text_offset_x = step_text == "V" ? 10.0f : (step_text.length() > 1 ? 14.0f : 9.0f);
        engine::Vector2f text_pos{node_x - text_offset_x, node_y - font_size / 2.0f + 2};
        graphics->draw_text(step_text, text_pos, text_color, engine::INVALID_HANDLE, font_size);
    }

    // Draw current step title with enhanced styling and more padding
    auto title_label = UILabel(center_x, stepper_y + node_radius * 2 + 100, step_title, 42);
    title_label.set_alignment(UILabel::Alignment::CENTER);
    title_label.set_color(engine::Color{200, 170, 255, 255});
    title_label.draw(graphics);

    // Draw step counter text below title with more spacing
    std::string counter_text = "Step " + std::to_string(current_step_index + 1) + " of " + std::to_string(total_steps);
    auto counter_label = UILabel(center_x, stepper_y + node_radius * 2 + 155, counter_text.c_str(), 22);
    counter_label.set_alignment(UILabel::Alignment::CENTER);
    counter_label.set_color(engine::Color{150, 130, 200, 200});
    counter_label.draw(graphics);
}



void Renderer::draw_circular_image(
    engine::IGraphicsPlugin* graphics,
    engine::TextureHandle texture,
    float x, float y,
    float size,
    bool is_selected
) {
    // Calculate circle center
    float circle_x = x + size / 2.0f;
    float circle_y = y + size / 2.0f;
    float radius = size / 2.0f;

    // Draw shadow
    engine::Vector2f shadow_center{circle_x + 6, circle_y + 6};
    graphics->draw_circle(shadow_center, radius, Config::SHADOW());

    // Draw glow effect if selected
    if (is_selected) {
        for (int j = 4; j > 0; j--) {
            float expand = 12.0f * j;
            engine::Vector2f glow_center{circle_x, circle_y};
            engine::Color glow_color{150, 100, 255, static_cast<unsigned char>(60 / j)};
            graphics->draw_circle(glow_center, radius + expand, glow_color);
        }
    }

    // Draw border circle
    engine::Vector2f border_center{circle_x, circle_y};
    engine::Color border_color = is_selected ? Config::SELECTED_BORDER() : Config::NORMAL_BORDER();
    graphics->draw_circle(border_center, radius + 4, border_color);

    // Draw image
    engine::Sprite sprite;
    sprite.texture_handle = texture;
    sprite.size = {size, size};
    sprite.source_rect = {};

    engine::Vector2f position{x, y};
    graphics->draw_sprite(sprite, position);
}

void Renderer::draw_map_selection(
    engine::IGraphicsPlugin* graphics,
    const std::vector<engine::TextureHandle>& map_thumbnails,
    int screen_width,
    size_t selected_map_index
) {
    // Use same circular image style as difficulty/gamemode
    float size = Config::DIFFICULTY_CIRCLE_SIZE;  // Same size as difficulty
    float spacing = Config::DIFFICULTY_CIRCLE_SPACING;
    float center_x = screen_width / 2.0f;

    size_t num_maps = map_thumbnails.size();
    if (num_maps == 0) return;

    float total_width = size * num_maps + spacing * (num_maps - 1);
    float start_x = center_x - total_width / 2.0f;

    for (size_t i = 0; i < num_maps; ++i) {
        float x = start_x + i * (size + spacing);
        float y = Config::CONTENT_START_Y;
        bool is_selected = (selected_map_index == i);

        draw_circular_image(graphics, map_thumbnails[i], x, y, size, is_selected);
    }
}

void Renderer::draw_difficulty_selection(
    engine::IGraphicsPlugin* graphics,
    const TexturePack& textures,
    int screen_width,
    protocol::Difficulty selected_difficulty
) {
    float size = Config::DIFFICULTY_CIRCLE_SIZE;
    float spacing = Config::DIFFICULTY_CIRCLE_SPACING;
    float center_x = screen_width / 2.0f;
    float total_width = size * 3 + spacing * 2;
    float start_x = center_x - total_width / 2.0f;

    engine::TextureHandle diff_textures[] = {textures.difficulty_easy, textures.difficulty_medium, textures.difficulty_hard};
    protocol::Difficulty diff_values[] = {protocol::Difficulty::EASY, protocol::Difficulty::NORMAL, protocol::Difficulty::HARD};

    for (int i = 0; i < 3; ++i) {
        float x = start_x + i * (size + spacing);
        float y = Config::CONTENT_START_Y;
        bool is_selected = (selected_difficulty == diff_values[i]);

        draw_circular_image(graphics, diff_textures[i], x, y, size, is_selected);
    }
}

void Renderer::draw_gamemode_selection(
    engine::IGraphicsPlugin* graphics,
    const TexturePack& textures,
    int screen_width,
    protocol::GameMode selected_mode
) {
    float size = Config::GAMEMODE_CIRCLE_SIZE;
    float spacing = Config::GAMEMODE_CIRCLE_SPACING;
    float center_x = screen_width / 2.0f;
    float total_width = size * 3 + spacing * 2;
    float start_x = center_x - total_width / 2.0f;

    engine::TextureHandle mode_textures[] = {textures.gamemode_duo, textures.gamemode_trio, textures.gamemode_squad};
    protocol::GameMode mode_values[] = {protocol::GameMode::DUO, protocol::GameMode::TRIO, protocol::GameMode::SQUAD};

    for (int i = 0; i < 3; ++i) {
        float x = start_x + i * (size + spacing);
        float y = Config::CONTENT_START_Y;
        bool is_selected = (selected_mode == mode_values[i]);

        draw_circular_image(graphics, mode_textures[i], x, y, size, is_selected);
    }
}

void Renderer::draw_navigation_buttons(
    engine::IGraphicsPlugin* graphics,
    int screen_width,
    int screen_height,
    bool show_previous,
    bool show_next,
    bool previous_hovered,
    bool next_hovered,
    float button_radius,
    float button_spacing,
    float button_y_offset
) {
    // Use dynamic values from edit mode
    float center_x = screen_width / 2.0f;
    float button_y = screen_height - button_y_offset;

    // Previous button (left arrow)
    if (show_previous) {
        float prev_x = center_x - button_spacing;
        float prev_y = button_y;

        // Draw shadow
        engine::Vector2f shadow_center{prev_x + 3, prev_y + 3};
        graphics->draw_circle(shadow_center, button_radius, {0, 0, 0, 150});

        // Draw glow if hovered
        if (previous_hovered) {
            for (int j = 4; j > 0; j--) {
                float glow_radius = button_radius + 8.0f * j;
                engine::Color glow_color{180, 120, 255, static_cast<unsigned char>(50 / j)};
                graphics->draw_circle({prev_x, prev_y}, glow_radius, glow_color);
            }
        }

        // Draw outer border
        engine::Color border_color = previous_hovered ?
            engine::Color{180, 120, 255, 255} :  // Bright purple when hovered
            engine::Color{140, 100, 255, 255};   // Normal purple
        graphics->draw_circle({prev_x, prev_y}, button_radius + 3, border_color);

        // Draw inner circle background
        engine::Color bg_color = previous_hovered ?
            engine::Color{100, 60, 160, 255} :   // Purple hover
            engine::Color{80, 50, 120, 255};     // Dark purple
        graphics->draw_circle({prev_x, prev_y}, button_radius, bg_color);

        // Draw inner glow
        if (previous_hovered) {
            engine::Color inner_glow{140, 90, 200, 120};
            graphics->draw_circle({prev_x, prev_y - button_radius / 3.0f}, button_radius * 0.6f, inner_glow);
        }

        // Draw left arrow (<)
        engine::Color arrow_color{230, 210, 255, 255};
        float arrow_size = 28.0f;  // Increased from 20 to 28 for better visibility
        float arrow_thickness = 4.5f;  // Increased from 3.5 for bolder lines

        // Declare all arrow points - centered in the circle
        engine::Vector2f prev_arrow_top_start{prev_x + arrow_size / 4.0f, prev_y - arrow_size / 2.0f};
        engine::Vector2f prev_arrow_top_end{prev_x - arrow_size / 4.0f, prev_y};
        engine::Vector2f prev_arrow_bottom_start{prev_x - arrow_size / 4.0f, prev_y};
        engine::Vector2f prev_arrow_bottom_end{prev_x + arrow_size / 4.0f, prev_y + arrow_size / 2.0f};

        // Draw left arrow lines
        graphics->draw_line(prev_arrow_top_start, prev_arrow_top_end, arrow_color, arrow_thickness);
        graphics->draw_line(prev_arrow_bottom_start, prev_arrow_bottom_end, arrow_color, arrow_thickness);
    }

    // Next button (right arrow)
    if (show_next) {
        float next_x = center_x + button_spacing;
        float next_y = button_y;

        // Draw shadow
        engine::Vector2f shadow_center{next_x + 3, next_y + 3};
        graphics->draw_circle(shadow_center, button_radius, {0, 0, 0, 150});

        // Draw glow if hovered
        if (next_hovered) {
            for (int j = 4; j > 0; j--) {
                float glow_radius = button_radius + 8.0f * j;
                engine::Color glow_color{180, 120, 255, static_cast<unsigned char>(50 / j)};
                graphics->draw_circle({next_x, next_y}, glow_radius, glow_color);
            }
        }

        // Draw outer border
        engine::Color border_color = next_hovered ?
            engine::Color{180, 120, 255, 255} :  // Bright purple when hovered
            engine::Color{140, 100, 255, 255};   // Normal purple
        graphics->draw_circle({next_x, next_y}, button_radius + 3, border_color);

        // Draw inner circle background
        engine::Color bg_color = next_hovered ?
            engine::Color{100, 60, 160, 255} :   // Purple hover
            engine::Color{80, 50, 120, 255};     // Dark purple
        graphics->draw_circle({next_x, next_y}, button_radius, bg_color);

        // Draw inner glow
        if (next_hovered) {
            engine::Color inner_glow{140, 90, 200, 120};
            graphics->draw_circle({next_x, next_y - button_radius / 3.0f}, button_radius * 0.6f, inner_glow);
        }

        // Draw right arrow (>)
        engine::Color arrow_color{230, 210, 255, 255};
        float arrow_size = 28.0f;  // Increased from 20 to 28 for better visibility
        float arrow_thickness = 4.5f;  // Increased from 3.5 for bolder lines

        // Declare all arrow points - centered in the circle
        engine::Vector2f next_arrow_top_start{next_x - arrow_size / 4.0f, next_y - arrow_size / 2.0f};
        engine::Vector2f next_arrow_top_end{next_x + arrow_size / 4.0f, next_y};
        engine::Vector2f next_arrow_bottom_start{next_x + arrow_size / 4.0f, next_y};
        engine::Vector2f next_arrow_bottom_end{next_x - arrow_size / 4.0f, next_y + arrow_size / 2.0f};

        // Draw right arrow lines
        graphics->draw_line(next_arrow_top_start, next_arrow_top_end, arrow_color, arrow_thickness);
        graphics->draw_line(next_arrow_bottom_start, next_arrow_bottom_end, arrow_color, arrow_thickness);
    }
}

}  // namespace rtype::client::createroom
