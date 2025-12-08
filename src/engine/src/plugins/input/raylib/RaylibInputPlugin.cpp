/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** RaylibInputPlugin - Implementation
*/

#include "plugins/input/raylib/RaylibInputPlugin.hpp"
#include <iostream>

RaylibInputPlugin::RaylibInputPlugin() {
    init_key_mapping();
}

bool RaylibInputPlugin::initialize() {
    std::cout << "RaylibInputPlugin: Initialization" << std::endl;
    initialized = true;
    return true;
}

void RaylibInputPlugin::shutdown() {
    std::cout << "RaylibInputPlugin: Shutdown" << std::endl;
    initialized = false;
}

void RaylibInputPlugin::init_key_mapping() {
    // Mappage des touches principales
    key_mapping[engine::Key::W] = KEY_W;
    key_mapping[engine::Key::A] = KEY_A;
    key_mapping[engine::Key::S] = KEY_S;
    key_mapping[engine::Key::D] = KEY_D;
    
    key_mapping[engine::Key::Up] = KEY_UP;
    key_mapping[engine::Key::Down] = KEY_DOWN;
    key_mapping[engine::Key::Left] = KEY_LEFT;
    key_mapping[engine::Key::Right] = KEY_RIGHT;
    
    key_mapping[engine::Key::Space] = KEY_SPACE;
    key_mapping[engine::Key::Enter] = KEY_ENTER;
    key_mapping[engine::Key::Escape] = KEY_ESCAPE;
    key_mapping[engine::Key::Tab] = KEY_TAB;
    key_mapping[engine::Key::Backspace] = KEY_BACKSPACE;
    
    // Lettres
    key_mapping[engine::Key::B] = KEY_B;
    key_mapping[engine::Key::C] = KEY_C;
    key_mapping[engine::Key::E] = KEY_E;
    key_mapping[engine::Key::F] = KEY_F;
    key_mapping[engine::Key::G] = KEY_G;
    key_mapping[engine::Key::H] = KEY_H;
    key_mapping[engine::Key::I] = KEY_I;
    key_mapping[engine::Key::J] = KEY_J;
    key_mapping[engine::Key::K] = KEY_K;
    key_mapping[engine::Key::L] = KEY_L;
    key_mapping[engine::Key::M] = KEY_M;
    key_mapping[engine::Key::N] = KEY_N;
    key_mapping[engine::Key::O] = KEY_O;
    key_mapping[engine::Key::P] = KEY_P;
    key_mapping[engine::Key::Q] = KEY_Q;
    key_mapping[engine::Key::R] = KEY_R;
    key_mapping[engine::Key::T] = KEY_T;
    key_mapping[engine::Key::U] = KEY_U;
    key_mapping[engine::Key::V] = KEY_V;
    key_mapping[engine::Key::X] = KEY_X;
    key_mapping[engine::Key::Y] = KEY_Y;
    key_mapping[engine::Key::Z] = KEY_Z;
    
    // Chiffres
    key_mapping[engine::Key::Num0] = KEY_ZERO;
    key_mapping[engine::Key::Num1] = KEY_ONE;
    key_mapping[engine::Key::Num2] = KEY_TWO;
    key_mapping[engine::Key::Num3] = KEY_THREE;
    key_mapping[engine::Key::Num4] = KEY_FOUR;
    key_mapping[engine::Key::Num5] = KEY_FIVE;
    key_mapping[engine::Key::Num6] = KEY_SIX;
    key_mapping[engine::Key::Num7] = KEY_SEVEN;
    key_mapping[engine::Key::Num8] = KEY_EIGHT;
    key_mapping[engine::Key::Num9] = KEY_NINE;
    
    // Modificateurs
    key_mapping[engine::Key::LShift] = KEY_LEFT_SHIFT;
    key_mapping[engine::Key::RShift] = KEY_RIGHT_SHIFT;
    key_mapping[engine::Key::LControl] = KEY_LEFT_CONTROL;
    key_mapping[engine::Key::RControl] = KEY_RIGHT_CONTROL;
    key_mapping[engine::Key::LAlt] = KEY_LEFT_ALT;
    key_mapping[engine::Key::RAlt] = KEY_RIGHT_ALT;
}

int RaylibInputPlugin::to_raylib_key(engine::Key key) const {
    auto it = key_mapping.find(key);
    if (it != key_mapping.end()) {
        return it->second;
    }
    return KEY_NULL;
}

int RaylibInputPlugin::to_raylib_mouse_button(engine::MouseButton button) const {
    switch (button) {
        case engine::MouseButton::Left: return MOUSE_BUTTON_LEFT;
        case engine::MouseButton::Right: return MOUSE_BUTTON_RIGHT;
        case engine::MouseButton::Middle: return MOUSE_BUTTON_MIDDLE;
        case engine::MouseButton::XButton1: return MOUSE_BUTTON_SIDE;
        case engine::MouseButton::XButton2: return MOUSE_BUTTON_EXTRA;
        default: return MOUSE_BUTTON_LEFT;
    }
}

// ============== KEYBOARD ==============

bool RaylibInputPlugin::is_key_pressed(engine::Key key) const {
    int raylib_key = to_raylib_key(key);
    return raylib_key != KEY_NULL && IsKeyDown(raylib_key);
}

bool RaylibInputPlugin::is_key_just_pressed(engine::Key key) const {
    auto it = previous_key_state.find(key);
    bool was_pressed = (it != previous_key_state.end() && it->second);
    bool is_pressed_now = is_key_pressed(key);
    
    return is_pressed_now && !was_pressed;
}

bool RaylibInputPlugin::is_key_just_released(engine::Key key) const {
    auto it = previous_key_state.find(key);
    bool was_pressed = (it != previous_key_state.end() && it->second);
    bool is_pressed_now = is_key_pressed(key);
    
    return !is_pressed_now && was_pressed;
}

// ============== MOUSE ==============

bool RaylibInputPlugin::is_mouse_button_pressed(engine::MouseButton button) const {
    int raylib_button = to_raylib_mouse_button(button);
    return IsMouseButtonDown(raylib_button);
}

bool RaylibInputPlugin::is_mouse_button_just_pressed(engine::MouseButton button) const {
    auto it = previous_mouse_state.find(button);
    bool was_pressed = (it != previous_mouse_state.end() && it->second);
    bool is_pressed_now = is_mouse_button_pressed(button);
    
    return is_pressed_now && !was_pressed;
}

bool RaylibInputPlugin::is_mouse_button_just_released(engine::MouseButton button) const {
    auto it = previous_mouse_state.find(button);
    bool was_pressed = (it != previous_mouse_state.end() && it->second);
    bool is_pressed_now = is_mouse_button_pressed(button);
    
    return !is_pressed_now && was_pressed;
}

engine::Vector2f RaylibInputPlugin::get_mouse_position() const {
    Vector2 pos = GetMousePosition();
    return {pos.x, pos.y};
}

float RaylibInputPlugin::get_mouse_wheel_delta() const {
    return GetMouseWheelMove();
}

// ============== GAMEPAD ==============

bool RaylibInputPlugin::is_gamepad_connected(int gamepad_id) const {
    return IsGamepadAvailable(gamepad_id);
}

bool RaylibInputPlugin::is_gamepad_button_pressed(int gamepad_id, int button) const {
    return IsGamepadButtonDown(gamepad_id, button);
}

float RaylibInputPlugin::get_gamepad_axis(int gamepad_id, int axis) const {
    return GetGamepadAxisMovement(gamepad_id, axis);
}

// ============== UPDATE ==============

void RaylibInputPlugin::update() {
    // Mettre à jour l'état précédent des touches
    for (auto& pair : key_mapping) {
        previous_key_state[pair.first] = is_key_pressed(pair.first);
    }
    
    // Mettre à jour l'état précédent des boutons souris
    previous_mouse_state[engine::MouseButton::Left] = is_mouse_button_pressed(engine::MouseButton::Left);
    previous_mouse_state[engine::MouseButton::Right] = is_mouse_button_pressed(engine::MouseButton::Right);
    previous_mouse_state[engine::MouseButton::Middle] = is_mouse_button_pressed(engine::MouseButton::Middle);
}

// ============== PLUGIN FACTORY ==============

extern "C" {
    engine::IInputPlugin* create_input_plugin() {
        return new RaylibInputPlugin();
    }

    void destroy_input_plugin(engine::IInputPlugin* plugin) {
        delete plugin;
    }
}
