#pragma once

#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "ui/UIButton.hpp"
#include "ui/UILabel.hpp"
#include "ui/UITextField.hpp"
#include <vector>
#include <memory>

namespace rtype::client {

class NetworkClient;

/**
 * @brief Base class for all menu screens
 *
 * Provides common interface and utilities for screen management.
 * Each screen handles its own UI elements and logic.
 */
class BaseScreen {
public:
    BaseScreen(NetworkClient& network_client, int screen_width, int screen_height)
        : network_client_(network_client)
        , screen_width_(screen_width)
        , screen_height_(screen_height) {}

    virtual ~BaseScreen() = default;

    /**
     * @brief Initialize screen UI elements
     */
    virtual void initialize() = 0;

    /**
     * @brief Update screen logic and handle input
     */
    virtual void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) = 0;

    /**
     * @brief Draw the screen
     */
    virtual void draw(engine::IGraphicsPlugin* graphics) = 0;

    /**
     * @brief Called when entering this screen
     */
    virtual void on_enter() {}

    /**
     * @brief Called when leaving this screen
     */
    virtual void on_exit() {}

protected:
    NetworkClient& network_client_;
    int screen_width_;
    int screen_height_;
};

}  // namespace rtype::client
