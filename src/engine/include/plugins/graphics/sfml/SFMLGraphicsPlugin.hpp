/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** SFMLGraphicsPlugin - SFML implementation of IGraphicsPlugin
*/

#pragma once

#include "plugin_manager/IGraphicsPlugin.hpp"
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

// Forward declarations for SFML types (avoid including SFML headers in public header)
namespace sf {
    class RenderWindow;
    class Texture;
    class Font;
    class View;
}

namespace engine {

/**
 * @brief SFML implementation of the graphics plugin interface
 * 
 * This plugin uses SFML for rendering graphics, loading textures,
 * and managing the game window. It provides a clean abstraction
 * over SFML's API through the IGraphicsPlugin interface.
 */
class SFMLGraphicsPlugin : public IGraphicsPlugin {
public:
    SFMLGraphicsPlugin();
    ~SFMLGraphicsPlugin() override;

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

    // Get default texture (pink/black checkerboard for missing textures)
    TextureHandle get_default_texture() const override;

    // Camera/View
    void set_view(Vector2f center, Vector2f size) override;
    void reset_view() override;

private:
    struct TextureData {
        std::unique_ptr<sf::Texture> texture;
        Vector2f size;
    };

    struct FontData {
        std::unique_ptr<sf::Font> font;
    };

    bool initialized_;
    std::unique_ptr<sf::RenderWindow> window_;
    int window_width_;
    int window_height_;
    std::string window_title_;
    bool is_fullscreen_;
    
    // Resource caches
    std::unordered_map<TextureHandle, TextureData> textures_;
    std::unordered_map<FontHandle, FontData> fonts_;
    
    // Handle generators
    TextureHandle next_texture_handle_;
    FontHandle next_font_handle_;

    // Default texture (pink/black checkerboard)
    TextureHandle default_texture_;

    // View state
    std::unique_ptr<sf::View> custom_view_;
    Vector2f view_center_;
    Vector2f view_size_;
    bool using_custom_view_;

    // Helper to create default checkerboard texture
    void create_default_texture();
};

}
