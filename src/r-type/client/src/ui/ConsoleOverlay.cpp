#include "ui/ConsoleOverlay.hpp"
#include <iostream>

namespace rtype::client {

ConsoleOverlay::ConsoleOverlay(float screen_width, float screen_height)
    : visible_(false)
    , x_(10.0f)
    , y_(10.0f)
    , width_(screen_width - 20.0f)
    , height_(400.0f)
    , command_history_index_(0)
{
    input_field_ = std::make_unique<rtype::UITextField>(
        x_ + 10.0f,
        y_ + height_ - 45.0f,
        width_ - 20.0f,
        35.0f,
        "> Enter admin command..."
    );
    input_field_->set_max_length(128);
}

void ConsoleOverlay::toggle()
{
    visible_ = !visible_;
    if (visible_) {
        input_field_->set_focused(true);
        add_info("Console opened. Type 'help' for commands.");
    } else
        input_field_->set_focused(false);
}

void ConsoleOverlay::set_visible(bool visible)
{
    visible_ = visible;
    input_field_->set_focused(visible);
}

void ConsoleOverlay::add_message(const std::string& message, engine::Color color)
{
    message_history_.push_back({message, color});
    if (message_history_.size() > MAX_MESSAGES)
        message_history_.pop_front();
}

void ConsoleOverlay::add_error(const std::string& error)
{
    add_message(error, {255, 60, 60, 255});  // Red
}

void ConsoleOverlay::add_success(const std::string& message)
{
    add_message(message, {60, 255, 60, 255});  // Green
}

void ConsoleOverlay::add_info(const std::string& info)
{
    add_message(info, {100, 200, 255, 255});  // Cyan
}

void ConsoleOverlay::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input)
{
    if (!visible_)
        return;
    input_field_->update(graphics, input);

    if (input->is_key_just_pressed(engine::Key::Enter)) {
        std::string command = input_field_->get_text();
        if (!command.empty())
            execute_command();
    }
    if (input->is_key_just_pressed(engine::Key::Escape)) {
        toggle();
        return;
    }
    handle_history_navigation(input);
}

void ConsoleOverlay::draw(engine::IGraphicsPlugin* graphics)
{
    if (!visible_)
        return;

    graphics->draw_rectangle(engine::Rectangle{x_, y_, width_, height_}, {10, 10, 20, 240});
    graphics->draw_rectangle_outline(engine::Rectangle{x_, y_, width_, height_}, {100, 150, 255, 255}, 3);
    graphics->draw_rectangle(engine::Rectangle{x_, y_, width_, 30.0f}, {20, 40, 80, 255});
    graphics->draw_text("ADMIN CONSOLE (Tab to close)",
                       engine::Vector2f{x_ + 10.0f, y_ + 7.0f},
                       {200, 220, 255, 255},
                       engine::INVALID_HANDLE,
                       16);
    float input_area_y = y_ + height_ - 55.0f;
    graphics->draw_rectangle(engine::Rectangle{x_ + 5.0f, input_area_y, width_ - 10.0f, 2.0f}, {100, 150, 255, 180});
    float line_height = 18.0f;
    float line_y = input_area_y - 10.0f;
    for (auto it = message_history_.rbegin();
         it != message_history_.rend() && line_y > (y_ + 35.0f);
         ++it) {
        graphics->draw_text(it->text,
                           engine::Vector2f{x_ + 15.0f, line_y},
                           it->color,
                           engine::INVALID_HANDLE,
                           14);
        line_y -= line_height;
    }
    input_field_->draw(graphics);
}

void ConsoleOverlay::execute_command()
{
    std::string command = input_field_->get_text();

    add_message("> " + command, {255, 255, 100, 255});
    command_history_.push_front(command);
    if (command_history_.size() > MAX_COMMAND_HISTORY)
        command_history_.pop_back();
    command_history_index_ = 0;
    if (on_command_)
        on_command_(command);
    input_field_->set_text("");
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

}