#include "screens/SkinScreen.hpp"
#include <iostream>
#include <cmath>
#include <cstdio> // For popen

// Cross-platform popen/pclose compatibility
#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace bagario {

SkinScreen::SkinScreen(LocalGameState& game_state, int screen_width, int screen_height)
    : BaseScreen(game_state, screen_width, screen_height) {
}

void SkinScreen::initialize() {
    rebuild_ui();
}

int SkinScreen::get_color_count(SkinPattern pattern) {
    switch (pattern) {
        case SkinPattern::SOLO: return 1;
        case SkinPattern::STRIPES: return 2;
        case SkinPattern::ZIGZAG: return 2;
        case SkinPattern::CIRCULAR: return 3;
        case SkinPattern::DOTS: return 2;
        default: return 0;
    }
}

void SkinScreen::rebuild_ui() {
    labels_.clear();
    pattern_buttons_.clear();

    float center_x = screen_width_ / 2.0f;
    float center_y = screen_height_ / 2.0f;
    float start_y = center_y - 400.0f;

    // Title
    auto title = std::make_unique<UILabel>(center_x, start_y, "CUSTOMIZE SKIN", 50);
    title->set_color(engine::Color{76, 175, 80, 255});
    title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(title));

    // Pattern section title
    auto pattern_title = std::make_unique<UILabel>(center_x, start_y + 50.0f, "Select Pattern", 28);
    pattern_title->set_color(engine::Color{200, 200, 200, 255});
    pattern_title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(pattern_title));

    // Pattern buttons (Row 1: Solo, Stripes, Zigzag)
    // Row 2: Circular, Dots, Image
    float btn_width = 140.0f; // Wider buttons
    float btn_height = 50.0f; // Taller
    float btn_spacing = 20.0f;

    // Calculate layout
    // Row 1: 3 buttons
    float row1_width = 3 * btn_width + 2 * btn_spacing;
    float start_x1 = center_x - row1_width / 2.0f;
    float row1_y = start_y + 90.0f;

    // Row 2: 3 buttons
    float row2_width = 3 * btn_width + 2 * btn_spacing;
    float start_x2 = center_x - row2_width / 2.0f;
    float row2_y = row1_y + btn_height + 15.0f; // Below row 1

    auto add_pattern_btn = [&](const std::string& text, SkinPattern p, float x, float y) {
        auto btn = std::make_unique<UIButton>(x, y, btn_width, btn_height, text);
        btn->set_on_click([this, p]() {
            game_state_.skin.pattern = p;
            update_pattern_buttons();
        });
        pattern_buttons_.push_back(std::move(btn));
    };

    add_pattern_btn("Solo", SkinPattern::SOLO, start_x1, row1_y);
    add_pattern_btn("Stripes", SkinPattern::STRIPES, start_x1 + btn_width + btn_spacing, row1_y);
    add_pattern_btn("Zigzag", SkinPattern::ZIGZAG, start_x1 + 2 * (btn_width + btn_spacing), row1_y);
    
    add_pattern_btn("Circular", SkinPattern::CIRCULAR, start_x2, row2_y);
    add_pattern_btn("Dots", SkinPattern::DOTS, start_x2 + btn_width + btn_spacing, row2_y);
    add_pattern_btn("Image", SkinPattern::IMAGE, start_x2 + 2 * (btn_width + btn_spacing), row2_y);

    update_pattern_buttons();

    // Image Input Field (Initially hidden or shown based on pattern)
    float input_width = 300.0f; // Slightly smaller to fit browse button
    float input_x = center_x - input_width / 2.0f - 40.0f;
    float input_y = start_y + 260.0f;
    image_input_ = std::make_unique<UITextField>(
        input_x, input_y, input_width, 60.0f, "Enter image path...");
    image_input_->set_max_length(512); // Allow long file paths
    image_input_->set_text(game_state_.skin.image_path);
    image_input_->set_on_change([this](const std::string& text) {
        game_state_.skin.image_path = text;
        preview_texture_ = engine::INVALID_HANDLE; // Force reload when path changes
    });

    // Browse Button
    browse_button_ = std::make_unique<UIButton>(
        input_x + input_width + 10.0f, input_y, 80.0f, 60.0f, "Browse"
    );
    browse_button_->set_on_click([this]() {
        // Simple Zenity file picker integration for Linux
        FILE* pipe = popen("zenity --file-selection --title=\"Select Image\" --file-filter=\"*.png *.jpg *.jpeg *.bmp\" 2>/dev/null", "r");
        if (pipe) {
            char buffer[1024];
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                std::string path = buffer;
                if (!path.empty() && path.back() == '\n') {
                    path.pop_back();
                }
                if (!path.empty()) {
                    game_state_.skin.image_path = path;
                    image_input_->set_text(path);
                    preview_texture_ = engine::INVALID_HANDLE; // Force reload
                }
            }
            pclose(pipe);
        }
    });

    // Color pickers (4x5 grid each, cell_size = 35)
    float cell_size = 35.0f;
    float picker_width = 5 * cell_size;
    float picker_height = 4 * cell_size;

    // Calculate total width of active pickers to center them dynamically?
    // For now, fixed positions but wider spacing
    float picker_y = start_y + 270.0f;
    float picker_spacing = 250.0f; // Wider spacing

    auto setup_picker = [&](std::unique_ptr<UIColorPicker>& picker, std::unique_ptr<UILabel>& label, float x, const std::string& text) {
        label = std::make_unique<UILabel>(x, picker_y - 30.0f, text, 24);
        label->set_color(engine::Color{200, 200, 200, 255});
        label->set_alignment(UILabel::Alignment::CENTER);

        picker = std::make_unique<UIColorPicker>(x - picker_width / 2.0f, picker_y, cell_size);
    };

    setup_picker(primary_picker_, primary_label_, center_x - picker_spacing, "Primary");
    setup_picker(secondary_picker_, secondary_label_, center_x, "Secondary");
    setup_picker(tertiary_picker_, tertiary_label_, center_x + picker_spacing, "Tertiary");

    // Initialize with current colors
    primary_picker_->set_selected_color(game_state_.skin.primary);
    primary_picker_->set_on_color_change([this](engine::Color c) { game_state_.skin.primary = c; });

    secondary_picker_->set_selected_color(game_state_.skin.secondary);
    secondary_picker_->set_on_color_change([this](engine::Color c) { game_state_.skin.secondary = c; });
    
    tertiary_picker_->set_selected_color(game_state_.skin.tertiary);
    tertiary_picker_->set_on_color_change([this](engine::Color c) { game_state_.skin.tertiary = c; });

    // Back button
    float back_width = 180.0f;
    float back_height = 55.0f;
    back_button_ = std::make_unique<UIButton>(
        center_x - back_width / 2.0f, start_y + 730.0f, back_width, back_height, "Back");
    back_button_->set_on_click([this]() {
        // Save user data (skin) before leaving
        game_state_.save_user();
        if (on_screen_change_) {
            on_screen_change_(GameScreen::WELCOME);
        }
    });
}

void SkinScreen::update_pattern_buttons() {
    if (pattern_buttons_.size() >= 6) {
        pattern_buttons_[0]->set_selected(game_state_.skin.pattern == SkinPattern::SOLO);
        pattern_buttons_[1]->set_selected(game_state_.skin.pattern == SkinPattern::STRIPES);
        pattern_buttons_[2]->set_selected(game_state_.skin.pattern == SkinPattern::ZIGZAG);
        pattern_buttons_[3]->set_selected(game_state_.skin.pattern == SkinPattern::CIRCULAR);
        pattern_buttons_[4]->set_selected(game_state_.skin.pattern == SkinPattern::DOTS);
        pattern_buttons_[5]->set_selected(game_state_.skin.pattern == SkinPattern::IMAGE);
    }
}

void SkinScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    for (auto& btn : pattern_buttons_) {
        btn->update(graphics, input);
    }

    // Update color pickers based on pattern
    // Update color pickers based on pattern
    bool is_image = (game_state_.skin.pattern == SkinPattern::IMAGE);

    if (!is_image) {
        int colors = get_color_count(game_state_.skin.pattern);

        if (colors >= 1 && primary_picker_) primary_picker_->update(graphics, input);
        if (colors >= 2 && secondary_picker_) secondary_picker_->update(graphics, input);
        if (colors >= 3 && tertiary_picker_) tertiary_picker_->update(graphics, input);
    } else {
        if (image_input_) {
            image_input_->update(graphics, input);
            if (browse_button_) browse_button_->update(graphics, input);
            
            // Try to load texture if path changed and valid
            if (preview_texture_ == engine::INVALID_HANDLE && !game_state_.skin.image_path.empty()) {
                try {
                    preview_texture_ = graphics->load_texture(game_state_.skin.image_path);
                } catch (...) {
                    // Ignore errors, preview will remain invalid
                    preview_texture_ = engine::INVALID_HANDLE;
                }
            }
        }
    }

    if (back_button_) {
        back_button_->update(graphics, input);
    }
}

void SkinScreen::draw_grid_background(engine::IGraphicsPlugin* graphics) {
    graphics->clear(engine::Color{20, 25, 30, 255});

    engine::Color grid_color{40, 45, 55, 100};
    float grid_spacing = 50.0f;

    for (float x = 0; x < screen_width_; x += grid_spacing) {
        graphics->draw_line({x, 0}, {x, static_cast<float>(screen_height_)}, grid_color, 1.0f);
    }
    for (float y = 0; y < screen_height_; y += grid_spacing) {
        graphics->draw_line({0, y}, {static_cast<float>(screen_width_), y}, grid_color, 1.0f);
    }
}


void SkinScreen::draw_preview(engine::IGraphicsPlugin* graphics) {
    float center_x = screen_width_ / 2.0f;
    float start_y = screen_height_ / 2.0f - 400.0f;  // Match rebuild_ui start_y
    float center_y = start_y + 580.0f;  // Position preview below color pickers
    float radius = 55.0f;
    float r_sq = radius * radius;

    // Preview label
    graphics->draw_text("Preview", {center_x - 35.0f, center_y - radius - 30.0f},
                        engine::Color{200, 200, 200, 255}, engine::INVALID_HANDLE, 22);

    switch (game_state_.skin.pattern) {
        case SkinPattern::SOLO:
            graphics->draw_circle({center_x, center_y}, radius, game_state_.skin.primary);
            break;

        case SkinPattern::STRIPES: {
            // Draw background circle
            graphics->draw_circle({center_x, center_y}, radius, game_state_.skin.primary);

            // Draw stripes clipped to circle using circle equation: x² + y² = r²
            // For a stripe at x_offset, the y range is: y = ±sqrt(r² - x²)
            float stripe_width = 8.0f;
            float stripe_spacing = 18.0f;
            float half_w = stripe_width / 2.0f;

            for (float x_off = -radius + stripe_spacing / 2.0f; x_off < radius; x_off += stripe_spacing) {
                // Calculate clipping for the stripe's outer edges
                float left_x = x_off - half_w;
                float right_x = x_off + half_w;

                // Find the limiting x (furthest from center)
                float limit_x = std::max(std::abs(left_x), std::abs(right_x));
                if (limit_x >= radius) continue;

                // Calculate y extent at the limiting x
                float y_extent = std::sqrt(r_sq - limit_x * limit_x);

                // Draw the clipped stripe
                graphics->draw_rectangle(
                    engine::Rectangle{center_x + x_off - half_w, center_y - y_extent, stripe_width, y_extent * 2.0f},
                    game_state_.skin.secondary
                );
            }
            break;
        }

        case SkinPattern::ZIGZAG: {
            // Draw background circle
            graphics->draw_circle({center_x, center_y}, radius, game_state_.skin.primary);

            // Draw zigzag - only draw segments fully inside the circle
            float amplitude = 10.0f;
            float line_spacing = 16.0f;
            float step = 2.0f;
            float inner_r_sq = (radius - 2.0f) * (radius - 2.0f); // Slightly smaller to account for line thickness

            for (float y_off = -radius + line_spacing / 2.0f; y_off < radius; y_off += line_spacing) {
                for (float x = -radius; x <= radius; x += step) {
                    float y0 = y_off + amplitude * std::sin((x - step) * 0.25f);
                    float y1 = y_off + amplitude * std::sin(x * 0.25f);

                    // Check if both endpoints are inside the circle
                    float d0_sq = (x - step) * (x - step) + y0 * y0;
                    float d1_sq = x * x + y1 * y1;

                    if (d0_sq < inner_r_sq && d1_sq < inner_r_sq) {
                        graphics->draw_line(
                            {center_x + x - step, center_y + y0},
                            {center_x + x, center_y + y1},
                            game_state_.skin.secondary, 3.0f
                        );
                    }
                }
            }
            break;
        }

        case SkinPattern::CIRCULAR:
            // Concentric circles - naturally contained
            graphics->draw_circle({center_x, center_y}, radius, game_state_.skin.primary);
            graphics->draw_circle({center_x, center_y}, radius * 0.7f, game_state_.skin.secondary);
            graphics->draw_circle({center_x, center_y}, radius * 0.4f, game_state_.skin.tertiary);
            break;

        case SkinPattern::DOTS: {
            // Draw background circle
            graphics->draw_circle({center_x, center_y}, radius, game_state_.skin.primary);

            // Draw dots only if they fit entirely inside the circle
            float dot_radius = 5.0f;
            float dot_spacing = 18.0f;
            float max_dist = radius - dot_radius; // Dot center must be this far from edge
            float max_dist_sq = max_dist * max_dist;

            for (float dy = -radius + dot_spacing / 2.0f; dy < radius; dy += dot_spacing) {
                for (float dx = -radius + dot_spacing / 2.0f; dx < radius; dx += dot_spacing) {
                    if (dx * dx + dy * dy <= max_dist_sq) {
                        graphics->draw_circle({center_x + dx, center_y + dy}, dot_radius, game_state_.skin.secondary);
                    }
                }
            }
            break;
        }

        case SkinPattern::IMAGE: {
            if (preview_texture_ != engine::INVALID_HANDLE) {
                engine::Vector2f tex_size = graphics->get_texture_size(preview_texture_);
                if (tex_size.x > 0 && tex_size.y > 0) {
                    // Calculate scale to cover the circle
                    float scale = std::max(radius * 2.0f / tex_size.x, radius * 2.0f / tex_size.y);
                    float scaled_w = tex_size.x * scale;
                    float scaled_h = tex_size.y * scale;

                    // Draw image in horizontal slices, clipped to circle (same logic as STRIPES)
                    // For each y, calculate x_extent = sqrt(r² - y²)
                    float slice_height = 1.0f;

                    for (float y = -radius; y < radius; y += slice_height) {
                        // Calculate x extent at this y using circle equation
                        float y_sq = y * y;
                        if (y_sq >= r_sq) continue;
                        float x_extent = std::sqrt(r_sq - y_sq);

                        // Calculate source rectangle in texture coordinates
                        // Map from circle coords (-radius to +radius) to texture coords (0 to tex_size)
                        float tex_center_x = tex_size.x / 2.0f;
                        float tex_center_y = tex_size.y / 2.0f;

                        float src_x = tex_center_x + (-x_extent) / scale;
                        float src_y = tex_center_y + y / scale;
                        float src_w = (x_extent * 2.0f) / scale;
                        float src_h = slice_height / scale;

                        // Clamp to texture bounds
                        if (src_x < 0) { src_w += src_x; src_x = 0; }
                        if (src_y < 0) { src_h += src_y; src_y = 0; }
                        if (src_x + src_w > tex_size.x) src_w = tex_size.x - src_x;
                        if (src_y + src_h > tex_size.y) src_h = tex_size.y - src_y;

                        if (src_w <= 0 || src_h <= 0) continue;

                        engine::Sprite slice;
                        slice.texture_handle = preview_texture_;
                        slice.source_rect.x = src_x;
                        slice.source_rect.y = src_y;
                        slice.source_rect.width = src_w;
                        slice.source_rect.height = src_h;
                        slice.size = {x_extent * 2.0f, slice_height};
                        slice.origin = {0, 0};
                        slice.tint = engine::Color::White;

                        graphics->draw_sprite(slice, {center_x - x_extent, center_y + y});
                    }
                }
            } else {
                graphics->draw_circle({center_x, center_y}, radius, engine::Color{100, 100, 100, 255});
                graphics->draw_text("?", {center_x - 10, center_y - 15}, engine::Color::White, engine::INVALID_HANDLE, 40);
            }
            break;
        }
    }

    // Draw subtle outline
    engine::Color outline{255, 255, 255, 80};
    for (float angle = 0; angle < 360; angle += 2.0f) {
        float rad = angle * 3.14159f / 180.0f;
        graphics->draw_circle({center_x + radius * std::cos(rad), center_y + radius * std::sin(rad)}, 1.0f, outline);
    }
}

void SkinScreen::draw(engine::IGraphicsPlugin* graphics) {
    draw_grid_background(graphics);

    // Draw labels
    for (auto& label : labels_) {
        label->draw(graphics);
    }

    // Draw pattern buttons
    for (auto& btn : pattern_buttons_) {
        btn->draw(graphics);
    }

    // Draw color pickers OR Image input based on pattern
    bool is_image = (game_state_.skin.pattern == SkinPattern::IMAGE);

    if (!is_image) {
        int colors = get_color_count(game_state_.skin.pattern);

        if (colors >= 1) {
            if (primary_label_) primary_label_->draw(graphics);
            if (primary_picker_) primary_picker_->draw(graphics);
        }
        if (colors >= 2) {
            if (secondary_label_) secondary_label_->draw(graphics);
            if (secondary_picker_) secondary_picker_->draw(graphics);
        }
        if (colors >= 3) {
            if (tertiary_label_) tertiary_label_->draw(graphics);
            if (tertiary_picker_) tertiary_picker_->draw(graphics);
        }
    } else {
        if (image_input_) {
            image_input_->draw(graphics);
            if (browse_button_) browse_button_->draw(graphics);
        }
    }

    // Draw preview
    draw_preview(graphics);

    // Draw back button
    if (back_button_) {
        back_button_->draw(graphics);
    }
}

}  // namespace bagario
