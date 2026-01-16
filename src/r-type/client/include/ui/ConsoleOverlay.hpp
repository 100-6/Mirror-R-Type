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
 * @brief In-game admin console overlay with modern design and scrolling
 *
 * Features:
 * - Text input for commands with auto-completion hints
 * - Scrollable message history with timestamps
 * - Visual scrollbar with mouse wheel support
 * - Command history navigation (up/down arrows)
 * - Message type icons and color coding
 * - Smooth animations and modern effects
 */
class ConsoleOverlay {
public:
    ConsoleOverlay(float screen_width, float screen_height);

    // Visibility
    void toggle();
    void set_visible(bool visible);
    bool is_visible() const { return visible_; }

    // Message display
    void add_message(const std::string& message,
                    engine::Color color = {255, 255, 255, 255});
    void add_error(const std::string& error);
    void add_success(const std::string& message);
    void add_info(const std::string& info);
    void add_warning(const std::string& warning);

    // Command callback
    using CommandCallback = std::function<void(const std::string&)>;
    void set_command_callback(CommandCallback callback) { on_command_ = callback; }

    // Update and render
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void draw(engine::IGraphicsPlugin* graphics);

private:
    bool visible_;
    float x_, y_, width_, height_;

    // Message history with timestamps
    enum class MessageType {
        NORMAL,
        ERROR,
        SUCCESS,
        INFO,
        WARNING,
        COMMAND
    };

    struct Message {
        std::string text;
        engine::Color color;
        MessageType type;
        std::chrono::system_clock::time_point timestamp;
        float fade_in_progress;  // 0.0 to 1.0 for smooth appearance
    };
    std::deque<Message> message_history_;
    static constexpr size_t MAX_MESSAGES = 200;  // Increased for scrolling

    // Input field
    std::unique_ptr<rtype::UITextField> input_field_;

    // Command history
    std::deque<std::string> command_history_;
    size_t command_history_index_;
    static constexpr size_t MAX_COMMAND_HISTORY = 50;

    // Callback
    CommandCallback on_command_;

    // Scrolling support
    int scroll_offset_;  // Number of lines scrolled
    int max_visible_lines_;
    float scroll_velocity_;  // For smooth scrolling
    bool is_mouse_over_console_;
    bool is_dragging_scrollbar_;
    float scrollbar_drag_start_y_;
    int scrollbar_drag_start_offset_;

    // Animation
    float open_animation_;  // 0.0 to 1.0
    static constexpr float ANIMATION_SPEED = 5.0f;

    // Input handling
    void execute_command();
    void handle_history_navigation(engine::IInputPlugin* input);
    void handle_scrolling(engine::IInputPlugin* input);
    void handle_scrollbar_interaction(engine::IInputPlugin* input);

    // Rendering helpers
    void draw_background(engine::IGraphicsPlugin* graphics);
    void draw_header(engine::IGraphicsPlugin* graphics);
    void draw_messages(engine::IGraphicsPlugin* graphics);
    void draw_scrollbar(engine::IGraphicsPlugin* graphics);
    void draw_input_area(engine::IGraphicsPlugin* graphics);
    std::string get_timestamp_string(const std::chrono::system_clock::time_point& time);
    std::string get_message_icon(MessageType type);

    // Scrolling helpers
    int get_total_message_count() const { return static_cast<int>(message_history_.size()); }
    int get_max_scroll_offset() const;
    void clamp_scroll_offset();

    // Key tracking for history navigation
    bool was_up_pressed_ = false;
    bool was_down_pressed_ = false;

    // Delta time for animations
    std::chrono::steady_clock::time_point last_update_time_;
    float get_delta_time();
};

}
