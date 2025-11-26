/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** RaylibGraphicsPlugin - Raylib implementation of IGraphicsPlugin
*/

#pragma once

#include "plugin_manager/IGraphicsPlugin.hpp"
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

namespace engine {

/**
 * @brief Raylib implementation of the graphics plugin interface
 * 
 * This plugin uses Raylib for rendering graphics, loading textures,
 * and managing the game window. It provides a clean abstraction
 * over Raylib's API through the IGraphicsPlugin interface.
 */
class RaylibGraphicsPlugin : public IGraphicsPlugin {
public:
    RaylibGraphicsPlugin();
    ~RaylibGraphicsPlugin() override;

    // IPlugin interface
    const char* get_name() const override;
    const char* get_version() const override;
    bool initialize() override;
    void shutdown() override;
    bool is_initialized() const override;

    // Window management
    bool create_window(int width, int height, const char* title) override;
    void close_window() override;
    bool is_window_open() const override;
    void set_fullscreen(bool fullscreen) override;
    void set_vsync(bool enabled) override;

    // Rendering
    void clear(Color color) override;
    void display() override;

    // Drawing primitives
    void draw_sprite(const Sprite& sprite, Vector2f position) override;
    void draw_text(const std::string& text, Vector2f position, Color color,
                   FontHandle font_handle, int font_size) override;
    void draw_rectangle(const Rectangle& rect, Color color) override;
    void draw_rectangle_outline(const Rectangle& rect, Color color, float thickness) override;
    void draw_circle(Vector2f center, float radius, Color color) override;
    void draw_line(Vector2f start, Vector2f end, Color color, float thickness) override;

    // Resource loading
    TextureHandle load_texture(const std::string& path) override;
    void unload_texture(TextureHandle handle) override;
    Vector2f get_texture_size(TextureHandle handle) const override;
    FontHandle load_font(const std::string& path) override;
    void unload_font(FontHandle handle) override;

    // Camera/View
    void set_view(Vector2f center, Vector2f size) override;
    void reset_view() override;

private:
    struct TextureData {
        std::vector<uint8_t> data; // Opaque storage for Raylib texture
        Vector2f size;
    };

    struct FontData {
        std::vector<uint8_t> data; // Opaque storage for Raylib font
    };

    bool initialized_;
    bool window_open_;
    int window_width_;
    int window_height_;
    
    // Resource caches
    std::unordered_map<TextureHandle, TextureData> textures_;
    std::unordered_map<FontHandle, FontData> fonts_;
    
    // Handle generators
    TextureHandle next_texture_handle_;
    FontHandle next_font_handle_;
    
    // View state
    Vector2f view_center_;
    Vector2f view_size_;
    bool using_custom_view_;
};

}

