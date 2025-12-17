#include "InputHandler.hpp"

namespace rtype::client {

InputHandler::InputHandler(engine::IInputPlugin& input_plugin)
    : input_plugin_(input_plugin) {
}

uint16_t InputHandler::gather_input() const {
    uint16_t flags = 0;

    auto is_pressed = [this](engine::Key key) {
        return input_plugin_.is_key_pressed(key);
    };

    if (is_pressed(engine::Key::W) || is_pressed(engine::Key::Up))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_UP);
    if (is_pressed(engine::Key::S) || is_pressed(engine::Key::Down))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_DOWN);
    if (is_pressed(engine::Key::A) || is_pressed(engine::Key::Left))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_LEFT);
    if (is_pressed(engine::Key::D) || is_pressed(engine::Key::Right))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_RIGHT);
    if (is_pressed(engine::Key::Space))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_SHOOT);
    if (is_pressed(engine::Key::LShift) || is_pressed(engine::Key::RShift))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_CHARGE);
    if (is_pressed(engine::Key::LControl) || is_pressed(engine::Key::RControl))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_SPECIAL);
    if (is_pressed(engine::Key::E))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_SWITCH_WEAPON);

    return flags;
}

bool InputHandler::is_escape_pressed() const {
    return input_plugin_.is_key_pressed(engine::Key::Escape);
}

}
