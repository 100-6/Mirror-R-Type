/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ChatOverlay - In-game chat overlay for multiplayer communication
*/

#include "ui/ChatOverlay.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace rtype::client {

ChatOverlay::ChatOverlay(float screen_width, float screen_height)
    : visible_(false)
    , screen_width_(screen_width)
    , screen_height_(screen_height)
    , x_(10.0f)
    , y_(screen_height - 310.0f)  // Bottom of screen
    , width_(500.0f)
    , height_(300.0f)
    , last_update_time_(std::chrono::steady_clock::now())
{
    input_field_ = std::make_unique<rtype::UITextField>(
        x_ + 10.0f,
        y_ + height_ - 40.0f,
        width_ - 20.0f,
        30.0f,
        "Type a message..."
    );
    input_field_->set_max_length(127);  // Match protocol max
}

void ChatOverlay::toggle()
{
    visible_ = !visible_;
    if (visible_) {
        input_field_->set_focused(true);
        open_animation_ = 1.0f;  // Show immediately
        clear_unread();
    } else {
        input_field_->set_focused(false);
        input_field_->set_text("");
        open_animation_ = 0.0f;  // Hide immediately when closing
    }
}

void ChatOverlay::set_visible(bool visible)
{
    visible_ = visible;
    input_field_->set_focused(visible);
    if (visible) {
        open_animation_ = 1.0f;  // Show immediately
        clear_unread();
    } else {
        input_field_->set_text("");
        open_animation_ = 0.0f;  // Hide immediately when closing
    }
}

void ChatOverlay::add_message(uint32_t sender_id, const std::string& sender_name,
                               const std::string& message)
{
    std::cout << "[ChatOverlay] add_message called: " << sender_name << ": " << message << "\n";
    std::cout << "[ChatOverlay] Current message count before add: " << messages_.size() << "\n";

    ChatMessage msg;
    msg.sender_id = sender_id;
    msg.sender_name = sender_name;
    msg.message = message;
    msg.timestamp = std::chrono::system_clock::now();
    msg.name_color = get_player_color(sender_id);

    messages_.push_back(msg);
    if (messages_.size() > MAX_MESSAGES)
        messages_.pop_front();

    std::cout << "[ChatOverlay] Message count after add: " << messages_.size() << "\n";

    // Auto-scroll to bottom
    if (scroll_offset_ < 2) {
        scroll_offset_ = 0;
    }

    // Increment unread if chat is closed
    if (!visible_) {
        unread_count_++;
        std::cout << "[ChatOverlay] Unread count: " << unread_count_ << "\n";
    }
}

float ChatOverlay::get_delta_time()
{
    auto now = std::chrono::steady_clock::now();
    float delta = std::chrono::duration<float>(now - last_update_time_).count();
    last_update_time_ = now;
    return std::min(delta, 0.1f);
}

engine::Color ChatOverlay::get_player_color(uint32_t player_id)
{
    // Generate a unique color based on player ID
    static const engine::Color colors[] = {
        {100, 200, 255, 255},  // Light blue
        {255, 150, 100, 255},  // Orange
        {150, 255, 150, 255},  // Light green
        {255, 200, 100, 255},  // Yellow
        {200, 150, 255, 255},  // Purple
        {255, 150, 200, 255},  // Pink
        {150, 255, 255, 255},  // Cyan
        {255, 255, 150, 255},  // Light yellow
    };
    return colors[player_id % 8];
}

void ChatOverlay::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input)
{
    if (!visible_)
        return;

    float dt = get_delta_time();

    // Animate opening
    if (open_animation_ < 1.0f) {
        open_animation_ = std::min(1.0f, open_animation_ + dt * ANIMATION_SPEED);
    }

    // Handle F1 to close chat - use is_key_just_pressed for reliable single trigger
    if (input->is_key_just_pressed(engine::Key::F1)) {
        set_visible(false);
        return;
    }

    // Handle Enter to send message - use is_key_just_pressed for reliable single trigger
    if (input->is_key_just_pressed(engine::Key::Enter)) {
        send_message();
    }

    // Handle scrolling
    handle_scrolling(input);

    // Update input field
    input_field_->update(graphics, input);
}

void ChatOverlay::send_message()
{
    std::string text = input_field_->get_text();

    std::cout << "[ChatOverlay] send_message() called, text='" << text << "'\n";

    // Trim whitespace
    size_t start = text.find_first_not_of(" \t\n\r");
    size_t end = text.find_last_not_of(" \t\n\r");

    if (start == std::string::npos || end == std::string::npos) {
        input_field_->set_text("");
        return;
    }

    text = text.substr(start, end - start + 1);

    if (text.empty()) {
        input_field_->set_text("");
        return;
    }

    std::cout << "[ChatOverlay] Sending message: " << text << "\n";

    // Call the send callback
    if (on_send_) {
        on_send_(text);
    }

    input_field_->set_text("");
}

void ChatOverlay::handle_scrolling(engine::IInputPlugin* input)
{
    // Page Up / Page Down
    bool up_pressed = input->is_key_pressed(engine::Key::PageUp);
    bool down_pressed = input->is_key_pressed(engine::Key::PageDown);

    if (up_pressed && !was_up_pressed_) {
        scroll_offset_ = std::min(scroll_offset_ + 3,
            static_cast<int>(messages_.size()) - max_visible_messages_);
        if (scroll_offset_ < 0) scroll_offset_ = 0;
    }
    if (down_pressed && !was_down_pressed_) {
        scroll_offset_ = std::max(0, scroll_offset_ - 3);
    }

    was_up_pressed_ = up_pressed;
    was_down_pressed_ = down_pressed;

    // Mouse wheel
    float wheel = input->get_mouse_wheel_delta();
    if (wheel != 0.0f) {
        scroll_offset_ += static_cast<int>(wheel * 3);
        int max_scroll = static_cast<int>(messages_.size()) - max_visible_messages_;
        scroll_offset_ = std::clamp(scroll_offset_, 0, std::max(0, max_scroll));
    }
}

void ChatOverlay::draw(engine::IGraphicsPlugin* graphics)
{
    static int draw_log_counter = 0;
    if (draw_log_counter++ % 300 == 0) {  // Log every ~5 seconds at 60fps
        std::cout << "[ChatOverlay::draw] visible_=" << visible_
                  << " open_animation_=" << open_animation_
                  << " messages_.size()=" << messages_.size() << "\n";
    }

    if (!visible_ && open_animation_ <= 0.0f)
        return;

    float alpha_mult = open_animation_;

    draw_background(graphics);
    draw_messages(graphics);
    draw_input_area(graphics);
}

void ChatOverlay::draw_background(engine::IGraphicsPlugin* graphics)
{
    // Use full opacity for better visibility
    uint8_t alpha = static_cast<uint8_t>(255 * open_animation_);

    // Main background - darker and fully opaque
    engine::Rectangle bg = {x_, y_, width_, height_};
    graphics->draw_rectangle(bg, {15, 18, 28, alpha});

    // Border
    graphics->draw_rectangle_outline(bg, {80, 150, 255, alpha}, 2.0f);

    // Header
    engine::Rectangle header = {x_, y_, width_, 30.0f};
    graphics->draw_rectangle(header, {30, 40, 60, alpha});

    // Header text
    graphics->draw_text("CHAT", {x_ + 10.0f, y_ + 5.0f},
        {200, 220, 255, alpha},
        engine::INVALID_HANDLE, 18);
}

void ChatOverlay::draw_messages(engine::IGraphicsPlugin* graphics)
{
    float message_area_y = y_ + 35.0f;
    float message_area_height = height_ - 80.0f;
    float line_height = 22.0f;

    max_visible_messages_ = static_cast<int>(message_area_height / line_height);

    if (messages_.empty())
        return;

    int start_idx = static_cast<int>(messages_.size()) - 1 - scroll_offset_;
    int count = 0;

    for (int i = start_idx; i >= 0 && count < max_visible_messages_; i--, count++) {
        const auto& msg = messages_[i];

        float msg_y = message_area_y + message_area_height - (count + 1) * line_height;

        // Draw player name
        float name_width = msg.sender_name.length() * 8.0f + 10.0f;
        graphics->draw_text(msg.sender_name + ":",
            {x_ + 10.0f, msg_y},
            {msg.name_color.r, msg.name_color.g, msg.name_color.b,
             static_cast<uint8_t>(255 * open_animation_)},
            engine::INVALID_HANDLE, 16);

        // Draw message
        graphics->draw_text(msg.message,
            {x_ + 15.0f + name_width, msg_y},
            {220, 220, 220, static_cast<uint8_t>(255 * open_animation_)},
            engine::INVALID_HANDLE, 16);
    }

    // Scroll indicator
    if (messages_.size() > static_cast<size_t>(max_visible_messages_)) {
        int total = static_cast<int>(messages_.size());
        int max_scroll = total - max_visible_messages_;

        if (max_scroll > 0) {
            float scroll_bar_height = message_area_height * 0.8f;
            float scroll_pos = 1.0f - (static_cast<float>(scroll_offset_) / max_scroll);
            float thumb_height = scroll_bar_height * (static_cast<float>(max_visible_messages_) / total);
            float thumb_y = message_area_y + 10.0f + (scroll_bar_height - thumb_height) * (1.0f - scroll_pos);

            // Track
            graphics->draw_rectangle(
                {x_ + width_ - 12.0f, message_area_y + 10.0f, 4.0f, scroll_bar_height},
                {60, 70, 90, static_cast<uint8_t>(150 * open_animation_)});

            // Thumb
            graphics->draw_rectangle(
                {x_ + width_ - 12.0f, thumb_y, 4.0f, thumb_height},
                {100, 150, 255, static_cast<uint8_t>(200 * open_animation_)});
        }
    }
}

void ChatOverlay::draw_input_area(engine::IGraphicsPlugin* graphics)
{
    // Input background - fully opaque
    uint8_t alpha = static_cast<uint8_t>(255 * open_animation_);
    engine::Rectangle input_bg = {x_ + 5.0f, y_ + height_ - 45.0f, width_ - 10.0f, 40.0f};
    graphics->draw_rectangle(input_bg, {25, 30, 45, alpha});

    // Draw the text field
    input_field_->draw(graphics);

    // Hint text
    graphics->draw_text("Press ENTER to send, F1 to close",
        {x_ + width_ - 270.0f, y_ + height_ - 12.0f},
        {150, 150, 170, static_cast<uint8_t>(180 * open_animation_)},
        engine::INVALID_HANDLE, 12);
}

void ChatOverlay::draw_notification_badge(engine::IGraphicsPlugin* graphics)
{
    if (visible_ || unread_count_ <= 0)
        return;

    // Small text at bottom-left near chat position
    std::string text = "T: " + std::to_string(unread_count_) + " msg";
    graphics->draw_text(text, {15.0f, screen_height_ - 25.0f}, {150, 180, 220, 200}, engine::INVALID_HANDLE, 14);
}

}
