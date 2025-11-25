/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Mock plugins for testing without real graphics/input libraries
*/

#pragma once

#include "../plugin_manager/include/IGraphicsPlugin.hpp"
#include "../plugin_manager/include/IInputPlugin.hpp"
#include <iostream>
#include <unordered_map>
#include <chrono>

namespace rtype {

/**
 * @brief Mock graphics plugin for console-based testing
 */
class MockGraphicsPlugin : public IGraphicsPlugin {
public:
    MockGraphicsPlugin() : window_open_(false), frame_count_(0) {}

    // IPlugin interface
    const char* get_name() const override { return "MockGraphicsPlugin"; }
    const char* get_version() const override { return "1.0.0"; }
    bool initialize() override { 
        initialized_ = true;
        return true; 
    }
    void shutdown() override { 
        initialized_ = false;
    }
    bool is_initialized() const override { return initialized_; }

    // Window management
    bool create_window(int width, int height, const char* title) override {
        std::cout << "[MockGraphics] Window created: " << width << "x" << height 
                  << " - " << title << std::endl;
        window_open_ = true;
        return true;
    }

    void close_window() override {
        std::cout << "[MockGraphics] Window closed" << std::endl;
        window_open_ = false;
    }

    bool is_window_open() const override { return window_open_; }
    void set_fullscreen(bool fullscreen) override { (void)fullscreen; }
    void set_vsync(bool enabled) override { (void)enabled; }

    // Rendering
    void clear(Color color) override { 
        (void)color;
        draw_calls_ = 0;
    }

    void display() override {
        frame_count_++;
        if (frame_count_ % 60 == 0) {
            std::cout << "[MockGraphics] Frame " << frame_count_ 
                      << " - Drew " << draw_calls_ << " objects" << std::endl;
        }
    }

    // Drawing primitives
    void draw_sprite(const Sprite& sprite, Vector2f position) override {
        (void)sprite;
        (void)position;
        draw_calls_++;
    }

    void draw_text(const std::string& text, Vector2f position, Color color,
                   FontHandle font_handle, int font_size) override {
        (void)text; (void)position; (void)color; (void)font_handle; (void)font_size;
        draw_calls_++;
    }

    void draw_rectangle(const Rectangle& rect, Color color) override {
        (void)rect;
        (void)color;
        draw_calls_++;
    }

    void draw_rectangle_outline(const Rectangle& rect, Color color, float thickness) override {
        (void)rect; (void)color; (void)thickness;
        draw_calls_++;
    }

    void draw_circle(Vector2f center, float radius, Color color) override {
        (void)center; (void)radius; (void)color;
        draw_calls_++;
    }

    void draw_line(Vector2f start, Vector2f end, Color color, float thickness) override {
        (void)start; (void)end; (void)color; (void)thickness;
        draw_calls_++;
    }

    // Resource loading (mock)
    TextureHandle load_texture(const std::string& path) override {
        (void)path;
        return ++next_handle_;
    }

    void unload_texture(TextureHandle handle) override { (void)handle; }
    Vector2f get_texture_size(TextureHandle handle) const override { 
        (void)handle;
        return {32.0f, 32.0f}; 
    }

    FontHandle load_font(const std::string& path) override {
        (void)path;
        return ++next_handle_;
    }

    void unload_font(FontHandle handle) override { (void)handle; }

    // Camera/View
    void set_view(Vector2f center, Vector2f size) override { (void)center; (void)size; }
    void reset_view() override {}

private:
    bool window_open_;
    bool initialized_{false};
    uint32_t frame_count_;
    uint32_t draw_calls_;
    uint32_t next_handle_{1};
};

/**
 * @brief Mock input plugin for automated testing
 */
class MockInputPlugin : public IInputPlugin {
public:
    MockInputPlugin() {}

    // IPlugin interface
    const char* get_name() const override { return "MockInputPlugin"; }
    const char* get_version() const override { return "1.0.0"; }
    bool initialize() override { 
        initialized_ = true;
        return true; 
    }
    void shutdown() override { 
        initialized_ = false;
    }
    bool is_initialized() const override { return initialized_; }

    // Keyboard
    bool is_key_pressed(Key key) const override {
        auto it = key_states_.find(key);
        return it != key_states_.end() && it->second;
    }

    bool is_key_just_pressed(Key key) const override {
        (void)key;
        return false;
    }

    bool is_key_just_released(Key key) const override {
        (void)key;
        return false;
    }

    // Mouse
    bool is_mouse_button_pressed(MouseButton button) const override {
        (void)button;
        return false;
    }

    bool is_mouse_button_just_pressed(MouseButton button) const override {
        (void)button;
        return false;
    }

    bool is_mouse_button_just_released(MouseButton button) const override {
        (void)button;
        return false;
    }

    Vector2f get_mouse_position() const override {
        return {0.0f, 0.0f};
    }

    float get_mouse_wheel_delta() const override {
        return 0.0f;
    }

    // Gamepad
    bool is_gamepad_connected(int gamepad_id) const override {
        (void)gamepad_id;
        return false;
    }

    bool is_gamepad_button_pressed(int gamepad_id, int button) const override {
        (void)gamepad_id; (void)button;
        return false;
    }

    float get_gamepad_axis(int gamepad_id, int axis) const override {
        (void)gamepad_id; (void)axis;
        return 0.0f;
    }

    void update() override {
        // Simulate automated input for testing
        // Can be extended to simulate key presses
    }

    // Helper for testing
    void simulate_key_press(Key key, bool pressed) {
        key_states_[key] = pressed;
    }

private:
    bool initialized_{false};
    std::unordered_map<Key, bool> key_states_;
};

} // namespace rtype
