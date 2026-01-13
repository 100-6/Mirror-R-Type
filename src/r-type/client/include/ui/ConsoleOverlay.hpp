#pragma once

#include "ui/UITextField.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include <vector>
#include <deque>
#include <string>
#include <functional>
#include <memory>

namespace rtype::client {

/**
 * @brief In-game admin console overlay
 *
 * Provides UI for:
 * - Text input for commands
 * - Message history display
 * - Command history (up/down arrows)
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

    // Command callback
    using CommandCallback = std::function<void(const std::string&)>;
    void set_command_callback(CommandCallback callback) { on_command_ = callback; }

    // Update and render
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void draw(engine::IGraphicsPlugin* graphics);

private:
    bool visible_;
    float x_, y_, width_, height_;

    // Message history
    struct Message {
        std::string text;
        engine::Color color;
    };
    std::deque<Message> message_history_;
    static constexpr size_t MAX_MESSAGES = 20;

    // Input field
    std::unique_ptr<rtype::UITextField> input_field_;

    // Command history
    std::deque<std::string> command_history_;
    size_t command_history_index_;
    static constexpr size_t MAX_COMMAND_HISTORY = 50;

    // Callback
    CommandCallback on_command_;

    // Input handling
    void execute_command();
    void handle_history_navigation(engine::IInputPlugin* input);

    // Key tracking for history navigation
    bool was_up_pressed_ = false;
    bool was_down_pressed_ = false;
};

}
