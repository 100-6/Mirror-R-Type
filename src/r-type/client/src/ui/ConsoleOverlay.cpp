#include "ui/ConsoleOverlay.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

namespace rtype::client {

ConsoleOverlay::ConsoleOverlay(float screen_width, float screen_height)
    : visible_(false)
    , x_(10.0f)
    , y_(10.0f)
    , width_(screen_width - 20.0f)
    , height_(500.0f)  // Increased height for better visibility
    , command_history_index_(0)
    , scroll_offset_(0)
    , max_visible_lines_(0)
    , scroll_velocity_(0.0f)
    , is_mouse_over_console_(false)
    , is_dragging_scrollbar_(false)
    , scrollbar_drag_start_y_(0.0f)
    , scrollbar_drag_start_offset_(0)
    , open_animation_(0.0f)
    , last_update_time_(std::chrono::steady_clock::now())
{
    input_field_ = std::make_unique<rtype::UITextField>(
        x_ + 45.0f,  // Space for prompt icon ">"
        y_ + height_ - 43.0f,
        width_ - 65.0f,  // Leave space for margins
        30.0f,
        ""  // No placeholder, we'll show our own hint
    );
    input_field_->set_max_length(256);  // Increased for longer commands
}

void ConsoleOverlay::toggle()
{
    visible_ = !visible_;
    if (visible_) {
        input_field_->set_focused(true);
        open_animation_ = 0.0f;
        scroll_offset_ = 0;  // Reset scroll when opening
        add_info("================================================");
        add_info("  Console Admin - Type 'help' for commands");
        add_info("================================================");
    } else {
        input_field_->set_focused(false);
    }
}

void ConsoleOverlay::set_visible(bool visible)
{
    visible_ = visible;
    input_field_->set_focused(visible);
    if (visible) {
        open_animation_ = 0.0f;
        scroll_offset_ = 0;
    }
}

void ConsoleOverlay::add_message_internal(const std::string& message, engine::Color color, MessageType type)
{
    Message msg;
    msg.text = message;
    msg.color = color;
    msg.type = type;
    msg.timestamp = std::chrono::system_clock::now();
    msg.fade_in_progress = 0.0f;

    message_history_.push_back(msg);
    if (message_history_.size() > MAX_MESSAGES)
        message_history_.pop_front();

    // Auto-scroll to bottom when new message arrives (if not manually scrolled up)
    if (scroll_offset_ < 3) {
        scroll_offset_ = 0;
    }
}

std::vector<std::string> ConsoleOverlay::wrap_text(const std::string& text, float max_width, engine::IGraphicsPlugin* graphics)
{
    std::vector<std::string> lines;
    if (text.empty()) {
        lines.push_back("");
        return lines;
    }

    // Approximate character width (we use fixed-width rendering)
    const float char_width = 8.0f;  // Approximate width for font size 14
    const size_t max_chars = static_cast<size_t>(max_width / char_width);

    std::string current_line;
    std::istringstream stream(text);
    std::string word;

    while (stream >> word) {
        // If adding this word would exceed the limit
        if (current_line.length() + word.length() + 1 > max_chars) {
            if (!current_line.empty()) {
                lines.push_back(current_line);
                current_line.clear();
            }

            // If a single word is too long, break it up
            if (word.length() > max_chars) {
                while (word.length() > max_chars) {
                    lines.push_back(word.substr(0, max_chars));
                    word = word.substr(max_chars);
                }
                if (!word.empty()) {
                    current_line = word;
                }
            } else {
                current_line = word;
            }
        } else {
            if (!current_line.empty()) {
                current_line += " ";
            }
            current_line += word;
        }
    }

    if (!current_line.empty()) {
        lines.push_back(current_line);
    }

    if (lines.empty()) {
        lines.push_back("");
    }

    return lines;
}

void ConsoleOverlay::add_message(const std::string& message, engine::Color color)
{
    // Calculate max text width (console width - margins - timestamp - icon)
    float max_text_width = width_ - 170.0f;  // Leave space for timestamp (85px) + icon (40px) + margins (45px)

    // Wrap text if needed
    std::vector<std::string> lines = wrap_text(message, max_text_width, nullptr);

    // Add each line as a separate message
    for (const auto& line : lines) {
        add_message_internal(line, color, MessageType::NORMAL);
    }
}

void ConsoleOverlay::add_error(const std::string& error)
{
    float max_text_width = width_ - 170.0f;
    std::vector<std::string> lines = wrap_text(error, max_text_width, nullptr);

    for (const auto& line : lines) {
        add_message_internal(line, {255, 80, 80, 255}, MessageType::ERROR);
    }
}

void ConsoleOverlay::add_success(const std::string& message)
{
    float max_text_width = width_ - 170.0f;
    std::vector<std::string> lines = wrap_text(message, max_text_width, nullptr);

    for (const auto& line : lines) {
        add_message_internal(line, {80, 255, 120, 255}, MessageType::SUCCESS);
    }
}

void ConsoleOverlay::add_info(const std::string& info)
{
    float max_text_width = width_ - 170.0f;
    std::vector<std::string> lines = wrap_text(info, max_text_width, nullptr);

    for (const auto& line : lines) {
        add_message_internal(line, {120, 200, 255, 255}, MessageType::INFO);
    }
}

void ConsoleOverlay::add_warning(const std::string& warning)
{
    float max_text_width = width_ - 170.0f;
    std::vector<std::string> lines = wrap_text(warning, max_text_width, nullptr);

    for (const auto& line : lines) {
        add_message_internal(line, {255, 200, 80, 255}, MessageType::WARNING);
    }
}

void ConsoleOverlay::clear()
{
    message_history_.clear();
    scroll_offset_ = 0;
}

float ConsoleOverlay::get_delta_time()
{
    auto now = std::chrono::steady_clock::now();
    float delta = std::chrono::duration<float>(now - last_update_time_).count();
    last_update_time_ = now;
    return std::min(delta, 0.1f);  // Cap at 100ms to prevent huge jumps
}

void ConsoleOverlay::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input)
{
    if (!visible_)
        return;

    float delta_time = get_delta_time();

    // Smooth opening animation
    if (open_animation_ < 1.0f) {
        open_animation_ += delta_time * ANIMATION_SPEED;
        if (open_animation_ > 1.0f) open_animation_ = 1.0f;
    }

    // Update fade-in animation for messages
    for (auto& msg : message_history_) {
        if (msg.fade_in_progress < 1.0f) {
            msg.fade_in_progress += delta_time * 8.0f;  // Fast fade-in
            if (msg.fade_in_progress > 1.0f) msg.fade_in_progress = 1.0f;
        }
    }

    // Check if mouse is over console
    engine::Vector2f mouse_pos = input->get_mouse_position();
    is_mouse_over_console_ = (mouse_pos.x >= x_ && mouse_pos.x <= x_ + width_ &&
                             mouse_pos.y >= y_ && mouse_pos.y <= y_ + height_);

    input_field_->update(graphics, input);

    // Handle Enter key for command execution
    if (input->is_key_just_pressed(engine::Key::Enter)) {
        std::string command = input_field_->get_text();
        if (!command.empty())
            execute_command();
    }

    // Handle Escape to close console
    if (input->is_key_just_pressed(engine::Key::Escape)) {
        toggle();
        return;
    }

    handle_history_navigation(input);
    handle_scrolling(input);
    handle_scrollbar_interaction(input);

    // Smooth scroll deceleration
    if (std::abs(scroll_velocity_) > 0.01f) {
        scroll_offset_ += static_cast<int>(scroll_velocity_);
        scroll_velocity_ *= 0.85f;  // Deceleration
        clamp_scroll_offset();
    }
}

void ConsoleOverlay::handle_scrolling(engine::IInputPlugin* input)
{
    if (!is_mouse_over_console_)
        return;

    // Mouse wheel scrolling
    // Note: You may need to add get_mouse_wheel_delta() to IInputPlugin interface
    // For now, we'll use PageUp/PageDown as alternative

    if (input->is_key_pressed(engine::Key::PageUp)) {
        scroll_offset_ += 5;
        clamp_scroll_offset();
    }
    if (input->is_key_pressed(engine::Key::PageDown)) {
        scroll_offset_ -= 5;
        clamp_scroll_offset();
    }

    // Home/End for quick navigation
    if (input->is_key_just_pressed(engine::Key::Home)) {
        scroll_offset_ = get_max_scroll_offset();
    }
    if (input->is_key_just_pressed(engine::Key::End)) {
        scroll_offset_ = 0;
    }
}

void ConsoleOverlay::handle_scrollbar_interaction(engine::IInputPlugin* input)
{
    float scrollbar_x = x_ + width_ - 15.0f;
    float scrollbar_width = 12.0f;
    float scrollbar_track_y = y_ + 45.0f;
    float scrollbar_track_height = height_ - 110.0f;

    engine::Vector2f mouse_pos = input->get_mouse_position();
    bool mouse_down = input->is_mouse_button_pressed(engine::MouseButton::Left);

    // Calculate scrollbar handle position and size
    int total_messages = get_total_message_count();
    if (total_messages <= max_visible_lines_) {
        is_dragging_scrollbar_ = false;
        return;  // No need for scrollbar
    }

    float content_ratio = static_cast<float>(max_visible_lines_) / total_messages;
    float handle_height = std::max(30.0f, scrollbar_track_height * content_ratio);

    float scroll_range = total_messages - max_visible_lines_;
    float scroll_progress = scroll_range > 0 ?
        static_cast<float>(scroll_offset_) / scroll_range : 0.0f;

    float handle_y = scrollbar_track_y +
        (scrollbar_track_height - handle_height) * (1.0f - scroll_progress);

    // Check if mouse is over scrollbar handle
    bool mouse_over_handle = (mouse_pos.x >= scrollbar_x &&
                             mouse_pos.x <= scrollbar_x + scrollbar_width &&
                             mouse_pos.y >= handle_y &&
                             mouse_pos.y <= handle_y + handle_height);

    // Start dragging
    if (mouse_over_handle && mouse_down && !is_dragging_scrollbar_) {
        is_dragging_scrollbar_ = true;
        scrollbar_drag_start_y_ = mouse_pos.y;
        scrollbar_drag_start_offset_ = scroll_offset_;
    }

    // Dragging
    if (is_dragging_scrollbar_ && mouse_down) {
        float delta_y = scrollbar_drag_start_y_ - mouse_pos.y;
        float scroll_scale = scroll_range / (scrollbar_track_height - handle_height);
        scroll_offset_ = scrollbar_drag_start_offset_ + static_cast<int>(delta_y * scroll_scale);
        clamp_scroll_offset();
    }

    // Stop dragging
    if (!mouse_down) {
        is_dragging_scrollbar_ = false;
    }

    // Click on track (not handle) to jump
    if (!is_dragging_scrollbar_ && mouse_down &&
        mouse_pos.x >= scrollbar_x && mouse_pos.x <= scrollbar_x + scrollbar_width &&
        mouse_pos.y >= scrollbar_track_y && mouse_pos.y <= scrollbar_track_y + scrollbar_track_height) {

        if (mouse_pos.y < handle_y) {
            // Clicked above handle - scroll up
            scroll_offset_ += max_visible_lines_;
        } else if (mouse_pos.y > handle_y + handle_height) {
            // Clicked below handle - scroll down
            scroll_offset_ -= max_visible_lines_;
        }
        clamp_scroll_offset();
    }
}

int ConsoleOverlay::get_max_scroll_offset() const
{
    return std::max(0, get_total_message_count() - max_visible_lines_);
}

void ConsoleOverlay::clamp_scroll_offset()
{
    int max_offset = get_max_scroll_offset();
    scroll_offset_ = std::max(0, std::min(scroll_offset_, max_offset));
}

void ConsoleOverlay::draw(engine::IGraphicsPlugin* graphics)
{
    if (!visible_)
        return;

    // Apply animation scaling
    float anim_scale = open_animation_;
    if (anim_scale < 0.01f) return;  // Don't render if animation just started

    draw_background(graphics);
    draw_header(graphics);
    draw_messages(graphics);
    draw_scrollbar(graphics);
    draw_input_area(graphics);
    input_field_->draw(graphics);
}

void ConsoleOverlay::draw_background(engine::IGraphicsPlugin* graphics)
{
    // Main background with transparency
    graphics->draw_rectangle(
        engine::Rectangle{x_, y_, width_, height_},
        {8, 10, 16, 245}  // Very dark blue-black
    );

    // Top gradient overlay for depth
    for (int i = 0; i < 20; ++i) {
        float alpha = 30.0f * (1.0f - i / 20.0f);
        graphics->draw_rectangle(
            engine::Rectangle{x_, y_ + i, width_, 1.0f},
            {20, 30, 50, static_cast<uint8_t>(alpha)}
        );
    }

    // Border with glow effect
    graphics->draw_rectangle_outline(
        engine::Rectangle{x_, y_, width_, height_},
        {80, 150, 255, 255},
        2.0f
    );

    // Subtle outer glow
    graphics->draw_rectangle_outline(
        engine::Rectangle{x_ - 1.0f, y_ - 1.0f, width_ + 2.0f, height_ + 2.0f},
        {80, 150, 255, 100},
        1.0f
    );
}

void ConsoleOverlay::draw_header(engine::IGraphicsPlugin* graphics)
{
    float header_height = 38.0f;

    // Header background gradient
    graphics->draw_rectangle(
        engine::Rectangle{x_, y_, width_, header_height},
        {15, 25, 45, 255}
    );

    // Header bottom border
    graphics->draw_rectangle(
        engine::Rectangle{x_, y_ + header_height - 2.0f, width_, 2.0f},
        {80, 150, 255, 200}
    );

    // Title with icon
    graphics->draw_text(
        ">> ADMIN CONSOLE",
        engine::Vector2f{x_ + 15.0f, y_ + 10.0f},
        {180, 220, 255, 255},
        engine::INVALID_HANDLE,
        18
    );
}

void ConsoleOverlay::draw_messages(engine::IGraphicsPlugin* graphics)
{
    float message_area_y = y_ + 45.0f;
    float message_area_height = height_ - 110.0f;  // Space for input area
    float line_height = 20.0f;

    max_visible_lines_ = static_cast<int>(message_area_height / line_height);

    // Message rendering area - start above input area
    float current_y = y_ + height_ - 70.0f;  // Adjusted to not overlap input area

    int total_messages = get_total_message_count();
    int start_index = std::max(0, total_messages - max_visible_lines_ - scroll_offset_);
    int end_index = std::min(total_messages, start_index + max_visible_lines_);

    // Draw messages from bottom to top
    for (int i = end_index - 1; i >= start_index; --i) {
        const Message& msg = message_history_[i];

        if (current_y < message_area_y) break;  // Out of visible area

        // Apply fade-in animation
        engine::Color msg_color = msg.color;
        msg_color.a = static_cast<uint8_t>(msg_color.a * msg.fade_in_progress);

        // Draw timestamp
        std::string timestamp = get_timestamp_string(msg.timestamp);
        graphics->draw_text(
            timestamp,
            engine::Vector2f{x_ + 15.0f, current_y},
            {100, 120, 150, static_cast<uint8_t>(180 * msg.fade_in_progress)},
            engine::INVALID_HANDLE,
            12
        );

        // Draw message icon
        std::string icon = get_message_icon(msg.type);
        float icon_x = x_ + 85.0f;
        graphics->draw_text(
            icon,
            engine::Vector2f{icon_x, current_y},
            msg_color,
            engine::INVALID_HANDLE,
            14
        );

        // Draw message text (more space for wider ASCII icons)
        float text_x = icon_x + 40.0f;
        graphics->draw_text(
            msg.text,
            engine::Vector2f{text_x, current_y},
            msg_color,
            engine::INVALID_HANDLE,
            14
        );

        current_y -= line_height;
    }
}

void ConsoleOverlay::draw_scrollbar(engine::IGraphicsPlugin* graphics)
{
    int total_messages = get_total_message_count();
    if (total_messages <= max_visible_lines_)
        return;  // No scrollbar needed

    float scrollbar_x = x_ + width_ - 15.0f;
    float scrollbar_width = 12.0f;
    float scrollbar_track_y = y_ + 45.0f;
    float scrollbar_track_height = height_ - 110.0f;

    // Draw track
    graphics->draw_rectangle(
        engine::Rectangle{scrollbar_x, scrollbar_track_y, scrollbar_width, scrollbar_track_height},
        {20, 25, 35, 200}
    );

    // Calculate handle position and size
    float content_ratio = static_cast<float>(max_visible_lines_) / total_messages;
    float handle_height = std::max(30.0f, scrollbar_track_height * content_ratio);

    float scroll_range = static_cast<float>(total_messages - max_visible_lines_);
    float scroll_progress = scroll_range > 0 ?
        static_cast<float>(scroll_offset_) / scroll_range : 0.0f;

    float handle_y = scrollbar_track_y +
        (scrollbar_track_height - handle_height) * (1.0f - scroll_progress);

    // Draw handle
    engine::Color handle_color = is_dragging_scrollbar_ ?
        engine::Color{120, 180, 255, 255} :
        engine::Color{80, 150, 255, 220};

    graphics->draw_rectangle(
        engine::Rectangle{scrollbar_x + 2.0f, handle_y, scrollbar_width - 4.0f, handle_height},
        handle_color
    );

    // Handle border
    graphics->draw_rectangle_outline(
        engine::Rectangle{scrollbar_x + 2.0f, handle_y, scrollbar_width - 4.0f, handle_height},
        {150, 200, 255, 150},
        1.0f
    );
}

void ConsoleOverlay::draw_input_area(engine::IGraphicsPlugin* graphics)
{
    float input_area_y = y_ + height_ - 55.0f;
    float input_area_height = 50.0f;

    // Simple input area background
    graphics->draw_rectangle(
        engine::Rectangle{x_ + 5.0f, input_area_y, width_ - 10.0f, input_area_height},
        {12, 15, 22, 255}
    );

    // Simple top border line
    graphics->draw_rectangle(
        engine::Rectangle{x_ + 5.0f, input_area_y, width_ - 10.0f, 2.0f},
        {80, 150, 255, 180}
    );

    // Command prompt icon (simple, no box)
    float prompt_x = x_ + 15.0f;
    float prompt_y = y_ + height_ - 40.0f;

    graphics->draw_text(
        ">",
        engine::Vector2f{prompt_x, prompt_y},
        {120, 180, 255, 255},
        engine::INVALID_HANDLE,
        20
    );

    // Hint text if input is empty and not focused
    if (input_field_->get_text().empty() && !input_field_->is_focused()) {
        graphics->draw_text(
            "Type a command...",
            engine::Vector2f{x_ + 75.0f, prompt_y + 3.0f},
            {80, 100, 130, 160},
            engine::INVALID_HANDLE,
            14
        );
    }
}

std::string ConsoleOverlay::get_timestamp_string(const std::chrono::system_clock::time_point& time)
{
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        time.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();

    return ss.str();
}

std::string ConsoleOverlay::get_message_icon(MessageType type)
{
    switch (type) {
        case MessageType::ERROR:   return "[X]";
        case MessageType::SUCCESS: return "[OK]";
        case MessageType::INFO:    return "[i]";
        case MessageType::WARNING: return "[!]";
        case MessageType::COMMAND: return ">>>";
        default:                   return "[-]";
    }
}

void ConsoleOverlay::execute_command()
{
    std::string command = input_field_->get_text();

    // Add command to history with special formatting
    Message cmd_msg;
    cmd_msg.text = command;
    cmd_msg.color = {255, 255, 150, 255};  // Yellow for commands
    cmd_msg.type = MessageType::COMMAND;
    cmd_msg.timestamp = std::chrono::system_clock::now();
    cmd_msg.fade_in_progress = 0.0f;
    message_history_.push_back(cmd_msg);

    if (message_history_.size() > MAX_MESSAGES)
        message_history_.pop_front();

    // Add to command history
    command_history_.push_front(command);
    if (command_history_.size() > MAX_COMMAND_HISTORY)
        command_history_.pop_back();
    command_history_index_ = 0;

    // Execute callback
    if (on_command_)
        on_command_(command);

    input_field_->set_text("");
    scroll_offset_ = 0;  // Auto-scroll to show response
}

void ConsoleOverlay::handle_history_navigation(engine::IInputPlugin* input)
{
    bool up_pressed = input->is_key_pressed(engine::Key::Up);

    if (up_pressed && !was_up_pressed_) {
        if (command_history_index_ < command_history_.size()) {
            input_field_->set_text(command_history_[command_history_index_]);
            command_history_index_++;
        }
    }
    was_up_pressed_ = up_pressed;

    bool down_pressed = input->is_key_pressed(engine::Key::Down);
    if (down_pressed && !was_down_pressed_) {
        if (command_history_index_ > 0) {
            command_history_index_--;
            if (command_history_index_ > 0)
                input_field_->set_text(command_history_[command_history_index_ - 1]);
            else
                input_field_->set_text("");
        }
    }
    was_down_pressed_ = down_pressed;
}

}  // namespace rtype::client
