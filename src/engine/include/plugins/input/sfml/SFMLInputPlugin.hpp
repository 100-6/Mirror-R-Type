/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** SFMLInputPlugin - SFML implementation of input plugin
*/

#ifndef SFMLINPUTPLUGIN_HPP_
#define SFMLINPUTPLUGIN_HPP_

#include "plugin_manager/IInputPlugin.hpp"
#include <unordered_map>

// Forward declaration
namespace sf {
    class Window;
}

/**
 * @brief SFML implementation of the input plugin
 * 
 * This plugin reads keyboard/mouse/gamepad input via SFML and provides
 * the IInputPlugin interface for the engine.
 */
class SFMLInputPlugin : public engine::IInputPlugin {
private:
    // Mapping of engine::Key to SFML Keyboard::Key
    std::unordered_map<engine::Key, int> key_mapping_;
    
    // Previous frame state for just_pressed/just_released detection
    std::unordered_map<engine::Key, bool> previous_key_state_;
    std::unordered_map<engine::MouseButton, bool> previous_mouse_state_;
    
    // Initialization state
    bool initialized_ = false;
    
    // Mouse wheel delta (accumulated per frame)
    float mouse_wheel_delta_ = 0.0f;

    /**
     * @brief Initialize the key mapping
     */
    void init_key_mapping();

    /**
     * @brief Convert engine::Key to SFML Keyboard key code
     */
    int to_sfml_key(engine::Key key) const;

    /**
     * @brief Convert engine::MouseButton to SFML Mouse button code
     */
    int to_sfml_mouse_button(engine::MouseButton button) const;

public:
    SFMLInputPlugin();
    virtual ~SFMLInputPlugin() = default;

    // IPlugin implementation
    bool initialize() override;
    void shutdown() override;
    bool is_initialized() const override { return initialized_; }
    const char* get_name() const override { return "SFMLInputPlugin"; }
    const char* get_version() const override { return "1.0.0"; }

    // IInputPlugin - Keyboard
    bool is_key_pressed(engine::Key key) const override;
    bool is_key_just_pressed(engine::Key key) const override;
    bool is_key_just_released(engine::Key key) const override;

    // IInputPlugin - Mouse
    bool is_mouse_button_pressed(engine::MouseButton button) const override;
    bool is_mouse_button_just_pressed(engine::MouseButton button) const override;
    bool is_mouse_button_just_released(engine::MouseButton button) const override;
    engine::Vector2f get_mouse_position() const override;
    float get_mouse_wheel_delta() const override;

    // IInputPlugin - Gamepad
    bool is_gamepad_connected(int gamepad_id) const override;
    bool is_gamepad_button_pressed(int gamepad_id, int button) const override;
    float get_gamepad_axis(int gamepad_id, int axis) const override;

    // Update
    void update() override;
};

#endif /* !SFMLINPUTPLUGIN_HPP_ */
