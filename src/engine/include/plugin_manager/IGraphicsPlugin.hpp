/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** IGraphicsPlugin - Graphics plugin interface
*/

#pragma once

#include "IPlugin.hpp"
#include "CommonTypes.hpp"
#include "PluginExport.hpp"
#include <string>

namespace engine {

/**
 * @brief Sprite information structure
 */
struct Sprite {
    TextureHandle texture_handle = INVALID_HANDLE;
    Vector2f size{0.0f, 0.0f};
    Vector2f origin{0.0f, 0.0f};
    float rotation = 0.0f;
    Color tint = Color::White;
};

/**
 * @brief Rectangle structure
 */
struct Rectangle {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    Rectangle() = default;
    Rectangle(float x, float y, float w, float h) 
        : x(x), y(y), width(w), height(h) {}
};

/**
 * @brief Graphics plugin interface
 * 
 * This interface defines the contract for all graphics plugins.
 * Implementations can use SFML, Raylib, SDL, or any other graphics library.
 */
class IGraphicsPlugin : public IPlugin {
public:
    virtual ~IGraphicsPlugin() = default;

    // Window management
    /**
     * @brief Create a window
     * @param width Window width in pixels
     * @param height Window height in pixels
     * @param title Window title
     * @return true if window was created successfully
     */
    virtual bool create_window(int width, int height, const char* title) = 0;

    /**
     * @brief Close the window
     */
    virtual void close_window() = 0;

    /**
     * @brief Check if the window is open
     * @return true if window is open
     */
    virtual bool is_window_open() const = 0;

    /**
     * @brief Set window to fullscreen or windowed mode
     * @param fullscreen true for fullscreen, false for windowed
     */
    virtual void set_fullscreen(bool fullscreen) = 0;

    /**
     * @brief Enable or disable vertical synchronization
     * @param enabled true to enable vsync
     */
    virtual void set_vsync(bool enabled) = 0;

    // Rendering
    /**
     * @brief Clear the window with a color
     * @param color Clear color
     */
    virtual void clear(Color color) = 0;

    /**
     * @brief Display the rendered frame
     */
    virtual void display() = 0;

    // Drawing primitives
    /**
     * @brief Draw a sprite
     * @param sprite Sprite information
     * @param position Position to draw at
     */
    virtual void draw_sprite(const Sprite& sprite, Vector2f position) = 0;

    /**
     * @brief Draw text
     * @param text Text to draw
     * @param position Position to draw at
     * @param color Text color
     * @param font_handle Font to use (INVALID_HANDLE for default font)
     * @param font_size Font size in pixels
     */
    virtual void draw_text(const std::string& text, Vector2f position, Color color,
                          FontHandle font_handle = INVALID_HANDLE, int font_size = 30) = 0;

    /**
     * @brief Draw a filled rectangle
     * @param rect Rectangle to draw
     * @param color Fill color
     */
    virtual void draw_rectangle(const Rectangle& rect, Color color) = 0;

    /**
     * @brief Draw a rectangle outline
     * @param rect Rectangle to draw
     * @param color Outline color
     * @param thickness Line thickness
     */
    virtual void draw_rectangle_outline(const Rectangle& rect, Color color, float thickness = 1.0f) = 0;

    /**
     * @brief Draw a circle
     * @param center Circle center position
     * @param radius Circle radius
     * @param color Fill color
     */
    virtual void draw_circle(Vector2f center, float radius, Color color) = 0;

    /**
     * @brief Draw a line
     * @param start Start position
     * @param end End position
     * @param color Line color
     * @param thickness Line thickness
     */
    virtual void draw_line(Vector2f start, Vector2f end, Color color, float thickness = 1.0f) = 0;

    // Resource loading
    /**
     * @brief Load a texture from file
     * @param path Path to the texture file
     * @return Texture handle, or INVALID_HANDLE on failure
     */
    virtual TextureHandle load_texture(const std::string& path) = 0;

    /**
     * @brief Unload a texture
     * @param handle Texture handle to unload
     */
    virtual void unload_texture(TextureHandle handle) = 0;

    /**
     * @brief Get texture dimensions
     * @param handle Texture handle
     * @return Texture size, or {0, 0} if invalid
     */
    virtual Vector2f get_texture_size(TextureHandle handle) const = 0;

    /**
     * @brief Get default texture (pink/black checkerboard for missing textures)
     * @return Default texture handle, or INVALID_HANDLE if not available
     */
    virtual TextureHandle get_default_texture() const = 0;

    /**
     * @brief Load a font from file
     * @param path Path to the font file
     * @return Font handle, or INVALID_HANDLE on failure
     */
    virtual FontHandle load_font(const std::string& path) = 0;

    /**
     * @brief Unload a font
     * @param handle Font handle to unload
     */
    virtual void unload_font(FontHandle handle) = 0;

    // Camera/View
    /**
     * @brief Set the camera view
     * @param center Center of the view
     * @param size Size of the view
     */
    virtual void set_view(Vector2f center, Vector2f size) = 0;

    /**
     * @brief Reset the view to the default (window size)
     */
    virtual void reset_view() = 0;
};

}

// Forward declaration for C linkage
namespace engine {
    class IGraphicsPlugin;
}

// Plugin factory function signatures
extern "C" {
    /**
     * @brief Factory function to create a graphics plugin instance
     * @return Pointer to the created plugin
     */
    PLUGIN_API engine::IGraphicsPlugin* create_graphics_plugin();

    /**
     * @brief Destroy a graphics plugin instance
     * @param plugin Plugin to destroy
     */
    PLUGIN_API void destroy_graphics_plugin(engine::IGraphicsPlugin* plugin);
}
