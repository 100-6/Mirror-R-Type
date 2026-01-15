/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** SFMLInputPlugin - SFML 3.0 Implementation
*/

#include "plugins/input/sfml/SFMLInputPlugin.hpp"
#include "plugin_manager/PluginExport.hpp"
#include <SFML/Window.hpp>
#include <iostream>

SFMLInputPlugin::SFMLInputPlugin() {
    init_key_mapping();
}

bool SFMLInputPlugin::initialize() {
    std::cout << "SFMLInputPlugin: Initialization" << std::endl;
    initialized_ = true;
    return true;
}

void SFMLInputPlugin::shutdown() {
    std::cout << "SFMLInputPlugin: Shutdown" << std::endl;
    initialized_ = false;
}

void SFMLInputPlugin::init_key_mapping() {
    // SFML 3.0: Keyboard keys are now sf::Keyboard::Key::*
    // Main control keys
    key_mapping_[engine::Key::W] = static_cast<int>(sf::Keyboard::Key::W);
    key_mapping_[engine::Key::A] = static_cast<int>(sf::Keyboard::Key::A);
    key_mapping_[engine::Key::S] = static_cast<int>(sf::Keyboard::Key::S);
    key_mapping_[engine::Key::D] = static_cast<int>(sf::Keyboard::Key::D);
    
    key_mapping_[engine::Key::Up] = static_cast<int>(sf::Keyboard::Key::Up);
    key_mapping_[engine::Key::Down] = static_cast<int>(sf::Keyboard::Key::Down);
    key_mapping_[engine::Key::Left] = static_cast<int>(sf::Keyboard::Key::Left);
    key_mapping_[engine::Key::Right] = static_cast<int>(sf::Keyboard::Key::Right);
    
    key_mapping_[engine::Key::Space] = static_cast<int>(sf::Keyboard::Key::Space);
    key_mapping_[engine::Key::Enter] = static_cast<int>(sf::Keyboard::Key::Enter);
    key_mapping_[engine::Key::Escape] = static_cast<int>(sf::Keyboard::Key::Escape);
    key_mapping_[engine::Key::Tab] = static_cast<int>(sf::Keyboard::Key::Tab);
    key_mapping_[engine::Key::Backspace] = static_cast<int>(sf::Keyboard::Key::Backspace);
    
    // Letters
    key_mapping_[engine::Key::B] = static_cast<int>(sf::Keyboard::Key::B);
    key_mapping_[engine::Key::C] = static_cast<int>(sf::Keyboard::Key::C);
    key_mapping_[engine::Key::E] = static_cast<int>(sf::Keyboard::Key::E);
    key_mapping_[engine::Key::F] = static_cast<int>(sf::Keyboard::Key::F);
    key_mapping_[engine::Key::G] = static_cast<int>(sf::Keyboard::Key::G);
    key_mapping_[engine::Key::H] = static_cast<int>(sf::Keyboard::Key::H);
    key_mapping_[engine::Key::I] = static_cast<int>(sf::Keyboard::Key::I);
    key_mapping_[engine::Key::J] = static_cast<int>(sf::Keyboard::Key::J);
    key_mapping_[engine::Key::K] = static_cast<int>(sf::Keyboard::Key::K);
    key_mapping_[engine::Key::L] = static_cast<int>(sf::Keyboard::Key::L);
    key_mapping_[engine::Key::M] = static_cast<int>(sf::Keyboard::Key::M);
    key_mapping_[engine::Key::N] = static_cast<int>(sf::Keyboard::Key::N);
    key_mapping_[engine::Key::O] = static_cast<int>(sf::Keyboard::Key::O);
    key_mapping_[engine::Key::P] = static_cast<int>(sf::Keyboard::Key::P);
    key_mapping_[engine::Key::Q] = static_cast<int>(sf::Keyboard::Key::Q);
    key_mapping_[engine::Key::R] = static_cast<int>(sf::Keyboard::Key::R);
    key_mapping_[engine::Key::T] = static_cast<int>(sf::Keyboard::Key::T);
    key_mapping_[engine::Key::U] = static_cast<int>(sf::Keyboard::Key::U);
    key_mapping_[engine::Key::V] = static_cast<int>(sf::Keyboard::Key::V);
    key_mapping_[engine::Key::X] = static_cast<int>(sf::Keyboard::Key::X);
    key_mapping_[engine::Key::Y] = static_cast<int>(sf::Keyboard::Key::Y);
    key_mapping_[engine::Key::Z] = static_cast<int>(sf::Keyboard::Key::Z);
    
    // Numbers
    key_mapping_[engine::Key::Num0] = static_cast<int>(sf::Keyboard::Key::Num0);
    key_mapping_[engine::Key::Num1] = static_cast<int>(sf::Keyboard::Key::Num1);
    key_mapping_[engine::Key::Num2] = static_cast<int>(sf::Keyboard::Key::Num2);
    key_mapping_[engine::Key::Num3] = static_cast<int>(sf::Keyboard::Key::Num3);
    key_mapping_[engine::Key::Num4] = static_cast<int>(sf::Keyboard::Key::Num4);
    key_mapping_[engine::Key::Num5] = static_cast<int>(sf::Keyboard::Key::Num5);
    key_mapping_[engine::Key::Num6] = static_cast<int>(sf::Keyboard::Key::Num6);
    key_mapping_[engine::Key::Num7] = static_cast<int>(sf::Keyboard::Key::Num7);
    key_mapping_[engine::Key::Num8] = static_cast<int>(sf::Keyboard::Key::Num8);
    key_mapping_[engine::Key::Num9] = static_cast<int>(sf::Keyboard::Key::Num9);
    
    // Modifiers
    key_mapping_[engine::Key::LShift] = static_cast<int>(sf::Keyboard::Key::LShift);
    key_mapping_[engine::Key::RShift] = static_cast<int>(sf::Keyboard::Key::RShift);
    key_mapping_[engine::Key::LControl] = static_cast<int>(sf::Keyboard::Key::LControl);
    key_mapping_[engine::Key::RControl] = static_cast<int>(sf::Keyboard::Key::RControl);
    key_mapping_[engine::Key::LAlt] = static_cast<int>(sf::Keyboard::Key::LAlt);
    key_mapping_[engine::Key::RAlt] = static_cast<int>(sf::Keyboard::Key::RAlt);
    
    // Function keys
    key_mapping_[engine::Key::F1] = static_cast<int>(sf::Keyboard::Key::F1);
    key_mapping_[engine::Key::F2] = static_cast<int>(sf::Keyboard::Key::F2);
    key_mapping_[engine::Key::F3] = static_cast<int>(sf::Keyboard::Key::F3);
    key_mapping_[engine::Key::F4] = static_cast<int>(sf::Keyboard::Key::F4);
    key_mapping_[engine::Key::F5] = static_cast<int>(sf::Keyboard::Key::F5);
    key_mapping_[engine::Key::F6] = static_cast<int>(sf::Keyboard::Key::F6);
    key_mapping_[engine::Key::F7] = static_cast<int>(sf::Keyboard::Key::F7);
    key_mapping_[engine::Key::F8] = static_cast<int>(sf::Keyboard::Key::F8);
    key_mapping_[engine::Key::F9] = static_cast<int>(sf::Keyboard::Key::F9);
    key_mapping_[engine::Key::F10] = static_cast<int>(sf::Keyboard::Key::F10);
    key_mapping_[engine::Key::F11] = static_cast<int>(sf::Keyboard::Key::F11);
    key_mapping_[engine::Key::F12] = static_cast<int>(sf::Keyboard::Key::F12);
}

int SFMLInputPlugin::to_sfml_key(engine::Key key) const {
    auto it = key_mapping_.find(key);
    if (it != key_mapping_.end()) {
        return it->second;
    }
    return static_cast<int>(sf::Keyboard::Key::Unknown);
}

int SFMLInputPlugin::to_sfml_mouse_button(engine::MouseButton button) const {
    // SFML 3.0: Mouse buttons are now sf::Mouse::Button::*
    switch (button) {
        case engine::MouseButton::Left: return static_cast<int>(sf::Mouse::Button::Left);
        case engine::MouseButton::Right: return static_cast<int>(sf::Mouse::Button::Right);
        case engine::MouseButton::Middle: return static_cast<int>(sf::Mouse::Button::Middle);
        case engine::MouseButton::XButton1: return static_cast<int>(sf::Mouse::Button::Extra1);
        case engine::MouseButton::XButton2: return static_cast<int>(sf::Mouse::Button::Extra2);
        default: return static_cast<int>(sf::Mouse::Button::Left);
    }
}

// ============== KEYBOARD ==============

bool SFMLInputPlugin::is_key_pressed(engine::Key key) const {
    int sfml_key = to_sfml_key(key);
    return sfml_key != static_cast<int>(sf::Keyboard::Key::Unknown) && 
           sf::Keyboard::isKeyPressed(static_cast<sf::Keyboard::Key>(sfml_key));
}

bool SFMLInputPlugin::is_key_just_pressed(engine::Key key) const {
    auto it = previous_key_state_.find(key);
    bool was_pressed = (it != previous_key_state_.end() && it->second);
    bool is_pressed_now = is_key_pressed(key);
    
    return is_pressed_now && !was_pressed;
}

bool SFMLInputPlugin::is_key_just_released(engine::Key key) const {
    auto it = previous_key_state_.find(key);
    bool was_pressed = (it != previous_key_state_.end() && it->second);
    bool is_pressed_now = is_key_pressed(key);
    
    return !is_pressed_now && was_pressed;
}

// ============== MOUSE ==============

bool SFMLInputPlugin::is_mouse_button_pressed(engine::MouseButton button) const {
    int sfml_button = to_sfml_mouse_button(button);
    return sf::Mouse::isButtonPressed(static_cast<sf::Mouse::Button>(sfml_button));
}

bool SFMLInputPlugin::is_mouse_button_just_pressed(engine::MouseButton button) const {
    auto it = previous_mouse_state_.find(button);
    bool was_pressed = (it != previous_mouse_state_.end() && it->second);
    bool is_pressed_now = is_mouse_button_pressed(button);
    
    return is_pressed_now && !was_pressed;
}

bool SFMLInputPlugin::is_mouse_button_just_released(engine::MouseButton button) const {
    auto it = previous_mouse_state_.find(button);
    bool was_pressed = (it != previous_mouse_state_.end() && it->second);
    bool is_pressed_now = is_mouse_button_pressed(button);
    
    return !is_pressed_now && was_pressed;
}

engine::Vector2f SFMLInputPlugin::get_mouse_position() const {
    if (window_handle_) {
        sf::Vector2i pos = sf::Mouse::getPosition(*window_handle_);
        return {static_cast<float>(pos.x), static_cast<float>(pos.y)};
    }

    // Fallback to desktop coords if window not found
    sf::Vector2i pos = sf::Mouse::getPosition();
    return {static_cast<float>(pos.x), static_cast<float>(pos.y)};
}

float SFMLInputPlugin::get_mouse_wheel_delta() const {
    return mouse_wheel_delta_;
}

// ============== GAMEPAD ==============

bool SFMLInputPlugin::is_gamepad_connected(int gamepad_id) const {
    return sf::Joystick::isConnected(static_cast<unsigned int>(gamepad_id));
}

bool SFMLInputPlugin::is_gamepad_button_pressed(int gamepad_id, int button) const {
    return sf::Joystick::isButtonPressed(
        static_cast<unsigned int>(gamepad_id),
        static_cast<unsigned int>(button)
    );
}

float SFMLInputPlugin::get_gamepad_axis(int gamepad_id, int axis) const {
    return sf::Joystick::getAxisPosition(
        static_cast<unsigned int>(gamepad_id),
        static_cast<sf::Joystick::Axis>(axis)
    ) / 100.0f;  // SFML returns -100 to 100, normalize to -1 to 1
}

// ============== UPDATE ==============

void SFMLInputPlugin::update() {
    // Update previous key state
    for (const auto& pair : key_mapping_) {
        previous_key_state_[pair.first] = is_key_pressed(pair.first);
    }
    
    // Update previous mouse button state
    previous_mouse_state_[engine::MouseButton::Left] = is_mouse_button_pressed(engine::MouseButton::Left);
    previous_mouse_state_[engine::MouseButton::Right] = is_mouse_button_pressed(engine::MouseButton::Right);
    previous_mouse_state_[engine::MouseButton::Middle] = is_mouse_button_pressed(engine::MouseButton::Middle);
    previous_mouse_state_[engine::MouseButton::XButton1] = is_mouse_button_pressed(engine::MouseButton::XButton1);
    previous_mouse_state_[engine::MouseButton::XButton2] = is_mouse_button_pressed(engine::MouseButton::XButton2);
    
    // Update joystick state (SFML requires this for accurate readings)
    sf::Joystick::update();
    
    // Reset mouse wheel delta (should be accumulated via event polling in graphics plugin)
    // Reset mouse wheel delta (should be accumulated via event polling in graphics plugin)
    mouse_wheel_delta_ = 0.0f;
}

void SFMLInputPlugin::set_window_handle(void* handle) {
    if (handle) {
        window_handle_ = static_cast<sf::Window*>(handle);
        std::cout << "SFMLInputPlugin: Window handle set successfully" << std::endl;
    }
}

bool SFMLInputPlugin::has_focus() const {
    if (window_handle_) {
        return window_handle_->hasFocus();
    }
    return true;  // Default to true if no window handle
}

// ============== PLUGIN FACTORY ==============

extern "C" {
    PLUGIN_API engine::IInputPlugin* create_input_plugin() {
        return new SFMLInputPlugin();
    }

    PLUGIN_API void destroy_input_plugin(engine::IInputPlugin* plugin) {
        delete plugin;
    }
}
