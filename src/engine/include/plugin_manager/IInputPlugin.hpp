/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** IInputPlugin - Input plugin interface
*/

#pragma once

#include "IPlugin.hpp"
#include "CommonTypes.hpp"
#include <string>

namespace engine {

/**
 * @brief Keyboard key codes
 */
enum class Key {
    Unknown = -1,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Escape, LControl, LShift, LAlt, LSystem,
    RControl, RShift, RAlt, RSystem,
    Menu, LBracket, RBracket, Semicolon, Comma, Period, Quote, Slash, Backslash,
    Tilde, Equal, Hyphen, Space, Enter, Backspace, Tab,
    PageUp, PageDown, End, Home, Insert, Delete,
    Add, Subtract, Multiply, Divide,
    Left, Right, Up, Down,
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12, F13, F14, F15,
    Pause
};

/**
 * @brief Mouse button codes
 */
enum class MouseButton {
    Left,
    Right,
    Middle,
    XButton1,
    XButton2
};

/**
 * @brief Input plugin interface
 * 
 * This interface defines the contract for all input plugins.
 * Implementations can use SFML, SDL, or any other input library.
 */
class IInputPlugin : public IPlugin {
public:
    virtual ~IInputPlugin() = default;

    // Keyboard
    /**
     * @brief Check if a key is currently pressed
     * @param key Key to check
     * @return true if key is pressed
     */
    virtual bool is_key_pressed(Key key) const = 0;

    /**
     * @brief Check if a key was just pressed this frame
     * @param key Key to check
     * @return true if key was just pressed
     */
    virtual bool is_key_just_pressed(Key key) const = 0;

    /**
     * @brief Check if a key was just released this frame
     * @param key Key to check
     * @return true if key was just released
     */
    virtual bool is_key_just_released(Key key) const = 0;

    // Mouse
    /**
     * @brief Check if a mouse button is currently pressed
     * @param button Mouse button to check
     * @return true if button is pressed
     */
    virtual bool is_mouse_button_pressed(MouseButton button) const = 0;

    /**
     * @brief Check if a mouse button was just pressed this frame
     * @param button Mouse button to check
     * @return true if button was just pressed
     */
    virtual bool is_mouse_button_just_pressed(MouseButton button) const = 0;

    /**
     * @brief Check if a mouse button was just released this frame
     * @param button Mouse button to check
     * @return true if button was just released
     */
    virtual bool is_mouse_button_just_released(MouseButton button) const = 0;

    /**
     * @brief Get current mouse position relative to the window
     * @return Mouse position
     */
    virtual Vector2f get_mouse_position() const = 0;

    /**
     * @brief Get mouse wheel scroll delta
     * @return Scroll amount (positive = up, negative = down)
     */
    virtual float get_mouse_wheel_delta() const = 0;

    // Gamepad (optional support)
    /**
     * @brief Check if a gamepad is connected
     * @param gamepad_id Gamepad index (0-7)
     * @return true if gamepad is connected
     */
    virtual bool is_gamepad_connected(int gamepad_id) const = 0;

    /**
     * @brief Check if a gamepad button is pressed
     * @param gamepad_id Gamepad index
     * @param button Button index
     * @return true if button is pressed
     */
    virtual bool is_gamepad_button_pressed(int gamepad_id, int button) const = 0;

    /**
     * @brief Get gamepad axis value
     * @param gamepad_id Gamepad index
     * @param axis Axis index
     * @return Axis value (-1.0 to 1.0)
     */
    virtual float get_gamepad_axis(int gamepad_id, int axis) const = 0;

    // Update
    /**
     * @brief Update input state (should be called once per frame)
     * This method updates the "just pressed/released" states
     */
    virtual void update() = 0;

    /**
     * @brief Set the window handle for coordinate conversion
     * @param handle Pointer to the window handle (platform specific void*)
     */
    virtual void set_window_handle(void* handle) = 0;
};

}
