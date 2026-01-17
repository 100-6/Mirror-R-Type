#pragma once

#include <cstdint>
#include "plugin_manager/IInputPlugin.hpp"
#include "protocol/PacketTypes.hpp"
#include "KeyBindings.hpp"

namespace rtype::client {

/**
 * @brief Handles player input collection and conversion to network format
 */
class InputHandler {
public:
    explicit InputHandler(engine::IInputPlugin& input_plugin, KeyBindings* key_bindings = nullptr);

    /**
     * @brief Set key bindings (optional, uses defaults if not set)
     */
    void set_key_bindings(KeyBindings* key_bindings);

    /**
     * @brief Gather current input state and convert to network flags
     * @return Input flags ready to be sent to server
     */
    uint16_t gather_input() const;

    /**
     * @brief Check if escape key is pressed
     */
    bool is_escape_pressed() const;

    /**
     * @brief Check if hitbox debug toggle key (H) was just pressed
     */
    bool is_hitbox_toggle_pressed() const;

    /**
     * @brief Check if network debug toggle key (F3) was just pressed
     */
    bool is_network_debug_toggle_pressed() const;

    /**
     * @brief Check if scoreboard key (Tab) is pressed
     */
    bool is_scoreboard_pressed() const;

private:
    engine::IInputPlugin& input_plugin_;
    KeyBindings* key_bindings_;
    KeyBindings default_bindings_;  // Fallback if none provided

    bool is_action_pressed(GameAction action) const;
};

}
