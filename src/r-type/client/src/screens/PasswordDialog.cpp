#include "screens/PasswordDialog.hpp"

namespace rtype::client {

PasswordDialog::PasswordDialog(int screen_width, int screen_height)
    : screen_width_(screen_width)
    , screen_height_(screen_height) {
}

void PasswordDialog::initialize() {
    labels_.clear();
    buttons_.clear();

    float center_x = screen_width_ / 2.0f;
    float center_y = screen_height_ / 2.0f;

    // Title with elegant styling
    auto title = std::make_unique<UILabel>(center_x, center_y - 100, "Enter Room Password", 28);
    title->set_alignment(UILabel::Alignment::CENTER);
    title->set_color(engine::Color{220, 210, 255, 255});
    labels_.push_back(std::move(title));

    // Subtitle/instruction
    auto subtitle = std::make_unique<UILabel>(center_x, center_y - 65, "This room is protected", 16);
    subtitle->set_alignment(UILabel::Alignment::CENTER);
    subtitle->set_color(engine::Color{160, 150, 200, 200});
    labels_.push_back(std::move(subtitle));

    // Password field (larger and more centered)
    password_field_ = std::make_unique<UITextField>(center_x - 180, center_y - 10, 360, 45, "Password");
    password_field_->set_password_mode(true);

    // Join button (styled)
    auto join_btn = std::make_unique<UIButton>(center_x - 185, center_y + 70, 170, 50, "Join");
    join_btn->set_on_click([this]() {
        if (on_join_) {
            std::string password = password_field_->get_text();
            on_join_(pending_room_id_, password);
        }
        hide();
    });
    buttons_.push_back(std::move(join_btn));

    // Cancel button (styled)
    auto cancel_btn = std::make_unique<UIButton>(center_x + 15, center_y + 70, 170, 50, "Cancel");
    cancel_btn->set_on_click([this]() {
        if (on_cancel_) {
            on_cancel_();
        }
        hide();
    });
    buttons_.push_back(std::move(cancel_btn));
}

void PasswordDialog::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    if (!visible_) return;

    if (password_field_) {
        password_field_->update(graphics, input);
    }

    // Update buttons only if password field is not focused
    if (!password_field_ || !password_field_->is_focused()) {
        for (auto& button : buttons_) {
            button->update(graphics, input);
        }
    }
}

void PasswordDialog::draw(engine::IGraphicsPlugin* graphics) {
    if (!visible_) return;

    // Draw semi-transparent overlay with blur effect
    engine::Rectangle overlay{0, 0, static_cast<float>(screen_width_), static_cast<float>(screen_height_)};
    graphics->draw_rectangle(overlay, engine::Color{0, 0, 0, 200});

    // Dialog dimensions and position
    float dialog_width = 500.0f;
    float dialog_height = 320.0f;
    float dialog_x = (screen_width_ - dialog_width) / 2.0f;
    float dialog_y = (screen_height_ - dialog_height) / 2.0f;
    float corner_radius = 22.0f;

    // Enhanced shadow layers for depth
    for (int i = 0; i < 4; ++i) {
        float shadow_offset = 12.0f + i * 4.0f;
        float shadow_alpha = 40 - i * 8;
        engine::Rectangle shadow{dialog_x + shadow_offset, dialog_y + shadow_offset,
                                dialog_width, dialog_height};
        graphics->draw_rectangle(shadow, engine::Color{0, 0, 0, static_cast<uint8_t>(shadow_alpha)});
    }

    // Main dialog background with rounded corners
    // Draw main rectangle
    engine::Rectangle dialog_main{dialog_x + corner_radius, dialog_y,
                                  dialog_width - corner_radius * 2, dialog_height};
    graphics->draw_rectangle(dialog_main, engine::Color{28, 25, 40, 255});

    engine::Rectangle dialog_left{dialog_x, dialog_y + corner_radius,
                                  corner_radius, dialog_height - corner_radius * 2};
    graphics->draw_rectangle(dialog_left, engine::Color{28, 25, 40, 255});

    engine::Rectangle dialog_right{dialog_x + dialog_width - corner_radius, dialog_y + corner_radius,
                                   corner_radius, dialog_height - corner_radius * 2};
    graphics->draw_rectangle(dialog_right, engine::Color{28, 25, 40, 255});

    // Rounded corners
    graphics->draw_circle({dialog_x + corner_radius, dialog_y + corner_radius},
                         corner_radius, engine::Color{28, 25, 40, 255});
    graphics->draw_circle({dialog_x + dialog_width - corner_radius, dialog_y + corner_radius},
                         corner_radius, engine::Color{28, 25, 40, 255});
    graphics->draw_circle({dialog_x + corner_radius, dialog_y + dialog_height - corner_radius},
                         corner_radius, engine::Color{28, 25, 40, 255});
    graphics->draw_circle({dialog_x + dialog_width - corner_radius, dialog_y + dialog_height - corner_radius},
                         corner_radius, engine::Color{28, 25, 40, 255});

    // Gradient overlay for modern look
    engine::Rectangle gradient_overlay{dialog_x + corner_radius, dialog_y,
                                      dialog_width - corner_radius * 2, dialog_height / 2};
    graphics->draw_rectangle(gradient_overlay, engine::Color{45, 40, 65, 60});

    // Elegant purple border (stepper style)
    engine::Color border_color{120, 100, 200, 255};
    float border_width = 3.0f;

    // Border rectangles
    engine::Rectangle border_top{dialog_x + corner_radius, dialog_y,
                                dialog_width - corner_radius * 2, border_width};
    graphics->draw_rectangle(border_top, border_color);

    engine::Rectangle border_bottom{dialog_x + corner_radius, dialog_y + dialog_height - border_width,
                                   dialog_width - corner_radius * 2, border_width};
    graphics->draw_rectangle(border_bottom, border_color);

    engine::Rectangle border_left{dialog_x, dialog_y + corner_radius,
                                 border_width, dialog_height - corner_radius * 2};
    graphics->draw_rectangle(border_left, border_color);

    engine::Rectangle border_right{dialog_x + dialog_width - border_width, dialog_y + corner_radius,
                                  border_width, dialog_height - corner_radius * 2};
    graphics->draw_rectangle(border_right, border_color);

    // Accent glow line at top
    engine::Rectangle glow_line{dialog_x + 40, dialog_y + 60, dialog_width - 80, 2.0f};
    graphics->draw_rectangle(glow_line, engine::Color{140, 120, 220, 180});

    // Draw labels
    for (auto& label : labels_) {
        label->draw(graphics);
    }

    // Draw password field
    if (password_field_) {
        password_field_->draw(graphics);
    }

    // Draw buttons
    for (auto& button : buttons_) {
        button->draw(graphics);
    }
}

void PasswordDialog::show(uint32_t room_id) {
    pending_room_id_ = room_id;
    visible_ = true;
    if (password_field_) {
        password_field_->set_text("");
        password_field_->set_focused(true);
    }
}

void PasswordDialog::hide() {
    visible_ = false;
    pending_room_id_ = 0;
    if (password_field_) {
        password_field_->set_text("");
    }
}

}  // namespace rtype::client
