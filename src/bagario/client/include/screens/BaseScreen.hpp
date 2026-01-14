#pragma once

#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "LocalGameState.hpp"
#include <vector>
#include <memory>

namespace bagario {

/**
 * @brief Base class for all screens
 */
class BaseScreen {
public:
    BaseScreen(LocalGameState& game_state, int screen_width, int screen_height)
        : game_state_(game_state)
        , screen_width_(screen_width)
        , screen_height_(screen_height) {}

    virtual ~BaseScreen() = default;

    virtual void initialize() = 0;
    virtual void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) = 0;
    virtual void draw(engine::IGraphicsPlugin* graphics) = 0;
    virtual void on_enter() {}
    virtual void on_exit() {}

protected:
    LocalGameState& game_state_;
    int screen_width_;
    int screen_height_;
};

}  // namespace bagario
