#include "InputHandler.hpp"

namespace rtype::client {

InputHandler::InputHandler(engine::IInputPlugin& input_plugin, KeyBindings* key_bindings)
    : input_plugin_(input_plugin), key_bindings_(key_bindings) {
    if (!key_bindings_) {
        key_bindings_ = &default_bindings_;
    }
}

void InputHandler::set_key_bindings(KeyBindings* key_bindings) {
    key_bindings_ = key_bindings ? key_bindings : &default_bindings_;
}

bool InputHandler::is_action_pressed(GameAction action) const {
    engine::Key primary = key_bindings_->get_key(action);
    engine::Key alt = key_bindings_->get_alt_key(action);

    bool pressed = false;
    if (primary != engine::Key::Unknown) {
        pressed |= input_plugin_.is_key_pressed(primary);
    }
    if (alt != engine::Key::Unknown) {
        pressed |= input_plugin_.is_key_pressed(alt);
    }

    return pressed;
}

uint16_t InputHandler::gather_input() const {
    uint16_t flags = 0;

    if (is_action_pressed(GameAction::MOVE_UP))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_UP);
    if (is_action_pressed(GameAction::MOVE_DOWN))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_DOWN);
    if (is_action_pressed(GameAction::MOVE_LEFT))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_LEFT);
    if (is_action_pressed(GameAction::MOVE_RIGHT))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_RIGHT);
    if (is_action_pressed(GameAction::SHOOT))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_SHOOT);
    if (is_action_pressed(GameAction::CHARGE))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_CHARGE);
    if (is_action_pressed(GameAction::SPECIAL))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_SPECIAL);
    // SWITCH_WEAPON removed - weapon is now determined by player level

    return flags;
}

bool InputHandler::is_escape_pressed() const {
    return input_plugin_.is_key_pressed(engine::Key::Escape);
}

bool InputHandler::is_hitbox_toggle_pressed() const {
    return is_action_pressed(GameAction::TOGGLE_HITBOX);
}

bool InputHandler::is_network_debug_toggle_pressed() const {
    return is_action_pressed(GameAction::TOGGLE_NETWORK_DEBUG);
}

bool InputHandler::is_scoreboard_pressed() const {
    return is_action_pressed(GameAction::SHOW_SCOREBOARD);
}

}
