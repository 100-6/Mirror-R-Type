#include "KeyBindings.hpp"
#include <fstream>
#include <sstream>

namespace rtype::client {

KeyBindings::KeyBindings() {
    init_defaults();
}

void KeyBindings::init_defaults() {
    // Movement
    primary_bindings_[GameAction::MOVE_UP] = engine::Key::W;
    alt_bindings_[GameAction::MOVE_UP] = engine::Key::Up;

    primary_bindings_[GameAction::MOVE_DOWN] = engine::Key::S;
    alt_bindings_[GameAction::MOVE_DOWN] = engine::Key::Down;

    primary_bindings_[GameAction::MOVE_LEFT] = engine::Key::A;
    alt_bindings_[GameAction::MOVE_LEFT] = engine::Key::Left;

    primary_bindings_[GameAction::MOVE_RIGHT] = engine::Key::D;
    alt_bindings_[GameAction::MOVE_RIGHT] = engine::Key::Right;

    // Actions
    primary_bindings_[GameAction::SHOOT] = engine::Key::Space;
    alt_bindings_[GameAction::SHOOT] = engine::Key::Unknown;

    primary_bindings_[GameAction::CHARGE] = engine::Key::LShift;
    alt_bindings_[GameAction::CHARGE] = engine::Key::RShift;

    primary_bindings_[GameAction::SPECIAL] = engine::Key::LControl;
    alt_bindings_[GameAction::SPECIAL] = engine::Key::RControl;

    // SWITCH_WEAPON removed - weapon is now determined by player level

    // Debug
    primary_bindings_[GameAction::TOGGLE_HITBOX] = engine::Key::H;
    alt_bindings_[GameAction::TOGGLE_HITBOX] = engine::Key::Unknown;

    primary_bindings_[GameAction::TOGGLE_NETWORK_DEBUG] = engine::Key::F3;
    alt_bindings_[GameAction::TOGGLE_NETWORK_DEBUG] = engine::Key::Unknown;
}

engine::Key KeyBindings::get_key(GameAction action) const {
    auto it = primary_bindings_.find(action);
    return it != primary_bindings_.end() ? it->second : engine::Key::Unknown;
}

engine::Key KeyBindings::get_alt_key(GameAction action) const {
    auto it = alt_bindings_.find(action);
    return it != alt_bindings_.end() ? it->second : engine::Key::Unknown;
}

void KeyBindings::set_key(GameAction action, engine::Key key) {
    primary_bindings_[action] = key;
}

void KeyBindings::set_alt_key(GameAction action, engine::Key key) {
    alt_bindings_[action] = key;
}

void KeyBindings::reset_to_defaults() {
    primary_bindings_.clear();
    alt_bindings_.clear();
    init_defaults();
}

std::string KeyBindings::get_key_name(engine::Key key) {
    switch (key) {
        case engine::Key::A: return "A";
        case engine::Key::B: return "B";
        case engine::Key::C: return "C";
        case engine::Key::D: return "D";
        case engine::Key::E: return "E";
        case engine::Key::F: return "F";
        case engine::Key::G: return "G";
        case engine::Key::H: return "H";
        case engine::Key::I: return "I";
        case engine::Key::J: return "J";
        case engine::Key::K: return "K";
        case engine::Key::L: return "L";
        case engine::Key::M: return "M";
        case engine::Key::N: return "N";
        case engine::Key::O: return "O";
        case engine::Key::P: return "P";
        case engine::Key::Q: return "Q";
        case engine::Key::R: return "R";
        case engine::Key::S: return "S";
        case engine::Key::T: return "T";
        case engine::Key::U: return "U";
        case engine::Key::V: return "V";
        case engine::Key::W: return "W";
        case engine::Key::X: return "X";
        case engine::Key::Y: return "Y";
        case engine::Key::Z: return "Z";
        case engine::Key::Num0: return "0";
        case engine::Key::Num1: return "1";
        case engine::Key::Num2: return "2";
        case engine::Key::Num3: return "3";
        case engine::Key::Num4: return "4";
        case engine::Key::Num5: return "5";
        case engine::Key::Num6: return "6";
        case engine::Key::Num7: return "7";
        case engine::Key::Num8: return "8";
        case engine::Key::Num9: return "9";
        case engine::Key::Space: return "Space";
        case engine::Key::Enter: return "Enter";
        case engine::Key::Escape: return "Escape";
        case engine::Key::Backspace: return "Backspace";
        case engine::Key::Tab: return "Tab";
        case engine::Key::LControl: return "L-Ctrl";
        case engine::Key::RControl: return "R-Ctrl";
        case engine::Key::LShift: return "L-Shift";
        case engine::Key::RShift: return "R-Shift";
        case engine::Key::LAlt: return "L-Alt";
        case engine::Key::RAlt: return "R-Alt";
        case engine::Key::Up: return "Up";
        case engine::Key::Down: return "Down";
        case engine::Key::Left: return "Left";
        case engine::Key::Right: return "Right";
        case engine::Key::F1: return "F1";
        case engine::Key::F2: return "F2";
        case engine::Key::F3: return "F3";
        case engine::Key::F4: return "F4";
        case engine::Key::F5: return "F5";
        case engine::Key::F6: return "F6";
        case engine::Key::F7: return "F7";
        case engine::Key::F8: return "F8";
        case engine::Key::F9: return "F9";
        case engine::Key::F10: return "F10";
        case engine::Key::F11: return "F11";
        case engine::Key::F12: return "F12";
        case engine::Key::Unknown: return "None";
        default: return "Unknown";
    }
}

std::string KeyBindings::get_action_name(GameAction action) {
    switch (action) {
        case GameAction::MOVE_UP: return "Move Up";
        case GameAction::MOVE_DOWN: return "Move Down";
        case GameAction::MOVE_LEFT: return "Move Left";
        case GameAction::MOVE_RIGHT: return "Move Right";
        case GameAction::SHOOT: return "Shoot";
        case GameAction::CHARGE: return "Charge";
        case GameAction::SPECIAL: return "Special";
        case GameAction::TOGGLE_HITBOX: return "Toggle Hitbox";
        case GameAction::TOGGLE_NETWORK_DEBUG: return "Network Debug";
        default: return "Unknown";
    }
}

bool KeyBindings::load_from_file(const std::string& filepath) {
    // TODO: Implement file loading
    return false;
}

bool KeyBindings::save_to_file(const std::string& filepath) const {
    // TODO: Implement file saving
    return false;
}

}
