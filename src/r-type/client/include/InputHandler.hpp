#pragma once

#include <cstdint>
#include "plugin_manager/IInputPlugin.hpp"
#include "protocol/PacketTypes.hpp"

namespace rtype::client {

/**
 * @brief Handles player input collection and conversion to network format
 */
class InputHandler {
public:
    explicit InputHandler(engine::IInputPlugin& input_plugin);

    /**
     * @brief Gather current input state and convert to network flags
     * @return Input flags ready to be sent to server
     */
    uint16_t gather_input() const;

    /**
     * @brief Check if escape key is pressed
     */
    bool is_escape_pressed() const;

private:
    engine::IInputPlugin& input_plugin_;
};

}
