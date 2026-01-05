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

    // Title
    auto title = std::make_unique<UILabel>(center_x, center_y - 80, "Enter Room Password", 24);
    title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(title));

    // Password field
    password_field_ = std::make_unique<UITextField>(center_x - 150, center_y - 20, 300, 40, "Password");
    password_field_->set_password_mode(true);

    // Join button
    auto join_btn = std::make_unique<UIButton>(center_x - 155, center_y + 40, 150, 40, "Join");
    join_btn->set_on_click([this]() {
        if (on_join_) {
            std::string password = password_field_->get_text();
            on_join_(pending_room_id_, password);
        }
        hide();
    });
    buttons_.push_back(std::move(join_btn));

    // Cancel button
    auto cancel_btn = std::make_unique<UIButton>(center_x + 5, center_y + 40, 150, 40, "Cancel");
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

    // Draw semi-transparent overlay
    engine::Rectangle overlay{0, 0, static_cast<float>(screen_width_), static_cast<float>(screen_height_)};
    graphics->draw_rectangle(overlay, engine::Color{0, 0, 0, 180});

    // Draw dialog background
    float dialog_width = 400.0f;
    float dialog_height = 250.0f;
    float dialog_x = (screen_width_ - dialog_width) / 2.0f;
    float dialog_y = (screen_height_ - dialog_height) / 2.0f;

    engine::Rectangle dialog_bg{dialog_x, dialog_y, dialog_width, dialog_height};
    graphics->draw_rectangle(dialog_bg, engine::Color{40, 40, 50, 255});
    graphics->draw_rectangle_outline(dialog_bg, engine::Color{100, 150, 255, 255}, 3.0f);

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
