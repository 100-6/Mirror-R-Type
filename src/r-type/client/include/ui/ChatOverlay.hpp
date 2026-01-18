/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ChatOverlay - In-game chat overlay for multiplayer communication
*/

#pragma once

#include "ui/UITextField.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include <vector>
#include <deque>
#include <string>
#include <functional>
#include <memory>
#include <chrono>

namespace rtype::client {

/**
 * @brief In-game chat overlay for multiplayer communication
 *
 * Features:
 * - Toggle with T key
 * - Non-blocking (game continues while chat is open)
 * - Message history with player names
 * - Notification badge for unread messages
 * - Smooth animations
 */
class ChatOverlay {
public:
    ChatOverlay(float screen_width, float screen_height);

    // Visibility
    void toggle();
    void set_visible(bool visible);
    bool is_visible() const { return visible_; }

    // Messages
    struct ChatMessage {
        uint32_t sender_id;
        std::string sender_name;
        std::string message;
        std::chrono::system_clock::time_point timestamp;
        engine::Color name_color;
    };

    void add_message(uint32_t sender_id, const std::string& sender_name,
                     const std::string& message);

    // Send callback
    using SendCallback = std::function<void(const std::string&)>;
    void set_send_callback(SendCallback callback) { on_send_ = callback; }

    // Notification system
    int get_unread_count() const { return unread_count_; }
    void clear_unread() { unread_count_ = 0; }

    // Update and render
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void draw(engine::IGraphicsPlugin* graphics);

    // Draw notification badge (call even when chat is closed)
    void draw_notification_badge(engine::IGraphicsPlugin* graphics);

private:
    bool visible_;
    float screen_width_, screen_height_;
    float x_, y_, width_, height_;

    // Messages
    std::deque<ChatMessage> messages_;
    static constexpr size_t MAX_MESSAGES = 100;

    // Input field
    std::unique_ptr<rtype::UITextField> input_field_;

    // Callback
    SendCallback on_send_;

    // Notification
    int unread_count_ = 0;

    // Scrolling
    int scroll_offset_ = 0;
    int max_visible_messages_ = 8;

    // Animation
    float open_animation_ = 0.0f;
    static constexpr float ANIMATION_SPEED = 8.0f;

    // Input state for scrolling
    bool was_up_pressed_ = false;
    bool was_down_pressed_ = false;

    // Delta time
    std::chrono::steady_clock::time_point last_update_time_;
    float get_delta_time();

    // Helpers
    void send_message();
    void handle_scrolling(engine::IInputPlugin* input);
    engine::Color get_player_color(uint32_t player_id);

    // Rendering helpers
    void draw_background(engine::IGraphicsPlugin* graphics);
    void draw_messages(engine::IGraphicsPlugin* graphics);
    void draw_input_area(engine::IGraphicsPlugin* graphics);
};

}
