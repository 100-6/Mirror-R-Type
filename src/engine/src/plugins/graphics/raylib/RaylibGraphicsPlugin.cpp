/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** RaylibGraphicsPlugin - Raylib implementation of IGraphicsPlugin
*/

#include "plugins/graphics/raylib/RaylibGraphicsPlugin.hpp"
#include <raylib.h>
#include <stdexcept>
#include <cstring>

namespace rtype {

// Helper functions to convert between rtype and Raylib types
static ::Color to_raylib_color(const Color& color) {
    return ::Color{color.r, color.g, color.b, color.a};
}

static Vector2 to_raylib_vector2(const Vector2f& vec) {
    return Vector2{vec.x, vec.y};
}

// Helper to store/retrieve Raylib structures in opaque storage
template<typename T>
static void store_in_vector(std::vector<uint8_t>& vec, const T& obj) {
    vec.resize(sizeof(T));
    std::memcpy(vec.data(), &obj, sizeof(T));
}

template<typename T>
static T* get_from_vector(std::vector<uint8_t>& vec) {
    if (vec.size() < sizeof(T)) return nullptr;
    return reinterpret_cast<T*>(vec.data());
}

template<typename T>
static const T* get_from_vector(const std::vector<uint8_t>& vec) {
    if (vec.size() < sizeof(T)) return nullptr;
    return reinterpret_cast<const T*>(vec.data());
}

// Constructor
RaylibGraphicsPlugin::RaylibGraphicsPlugin()
    : initialized_(false)
    , window_open_(false)
    , window_width_(0)
    , window_height_(0)
    , next_texture_handle_(1) // Start at 1, 0 is INVALID_HANDLE
    , next_font_handle_(1)
    , view_center_{0.0f, 0.0f}
    , view_size_{0.0f, 0.0f}
    , using_custom_view_(false) {
}

// Destructor
RaylibGraphicsPlugin::~RaylibGraphicsPlugin() {
    shutdown();
}

// IPlugin interface
const char* RaylibGraphicsPlugin::get_name() const {
    return "Raylib Graphics Plugin";
}

const char* RaylibGraphicsPlugin::get_version() const {
    return "1.0.0";
}

bool RaylibGraphicsPlugin::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Raylib doesn't need explicit initialization beyond window creation
    initialized_ = true;
    return true;
}

void RaylibGraphicsPlugin::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Unload all resources
    textures_.clear();
    fonts_.clear();
    
    // Close window if open
    if (window_open_) {
        CloseWindow();
        window_open_ = false;
    }
    
    initialized_ = false;
}

bool RaylibGraphicsPlugin::is_initialized() const {
    return initialized_;
}

// Window management
bool RaylibGraphicsPlugin::create_window(int width, int height, const char* title) {
    if (window_open_) {
        return true; // Window already created
    }
    
    window_width_ = width;
    window_height_ = height;
    
    InitWindow(width, height, title);
    
    if (!IsWindowReady()) {
        return false;
    }
    
    window_open_ = true;
    
    // Set default view
    view_center_ = Vector2f(static_cast<float>(width) / 2.0f, static_cast<float>(height) / 2.0f);
    view_size_ = Vector2f(static_cast<float>(width), static_cast<float>(height));
    
    return true;
}

void RaylibGraphicsPlugin::close_window() {
    if (window_open_) {
        CloseWindow();
        window_open_ = false;
    }
}

bool RaylibGraphicsPlugin::is_window_open() const {
    return window_open_ && !WindowShouldClose();
}

void RaylibGraphicsPlugin::set_fullscreen(bool fullscreen) {
    if (!window_open_) {
        return;
    }
    
    if (fullscreen && !IsWindowFullscreen()) {
        ToggleFullscreen();
    } else if (!fullscreen && IsWindowFullscreen()) {
        ToggleFullscreen();
    }
}

void RaylibGraphicsPlugin::set_vsync(bool enabled) {
    if (enabled) {
        SetTargetFPS(60);
    } else {
        SetTargetFPS(0);
    }
}

// Rendering
void RaylibGraphicsPlugin::clear(Color color) {
    if (!window_open_) {
        return;
    }
    
    BeginDrawing();
    ClearBackground(to_raylib_color(color));
}

void RaylibGraphicsPlugin::display() {
    if (!window_open_) {
        return;
    }
    
    EndDrawing();
}

// Drawing primitives
void RaylibGraphicsPlugin::draw_sprite(const Sprite& sprite, Vector2f position) {
    if (!window_open_ || sprite.texture_handle == INVALID_HANDLE) {
        return;
    }
    
    auto it = textures_.find(sprite.texture_handle);
    if (it == textures_.end()) {
        return;
    }
    
    const Texture2D* texture = get_from_vector<Texture2D>(it->second.data);
    if (!texture) {
        return;
    }
    
    // Apply custom view transformation if needed
    Vector2f draw_pos = position;
    if (using_custom_view_) {
        // Offset by view
        draw_pos.x = position.x - view_center_.x + view_size_.x / 2.0f;
        draw_pos.y = position.y - view_center_.y + view_size_.y / 2.0f;
    }
    
    // Define source rectangle (full texture)
    ::Rectangle source = {
        0.0f,
        0.0f,
        static_cast<float>(texture->width),
        static_cast<float>(texture->height)
    };
    
    // Define destination rectangle
    ::Rectangle dest = {
        draw_pos.x,
        draw_pos.y,
        sprite.size.x,
        sprite.size.y
    };
    
    // Origin for rotation
    Vector2 origin = to_raylib_vector2(sprite.origin);
    
    // Draw with rotation and tint
    DrawTexturePro(
        *texture,
        source,
        dest,
        origin,
        sprite.rotation,
        to_raylib_color(sprite.tint)
    );
}

void RaylibGraphicsPlugin::draw_text(const std::string& text, Vector2f position, Color color,
                                     FontHandle font_handle, int font_size) {
    if (!window_open_) {
        return;
    }
    
    // Apply custom view transformation if needed
    Vector2f draw_pos = position;
    if (using_custom_view_) {
        draw_pos.x = position.x - view_center_.x + view_size_.x / 2.0f;
        draw_pos.y = position.y - view_center_.y + view_size_.y / 2.0f;
    }
    
    if (font_handle == INVALID_HANDLE) {
        // Use default font
        DrawText(
            text.c_str(),
            static_cast<int>(draw_pos.x),
            static_cast<int>(draw_pos.y),
            font_size,
            to_raylib_color(color)
        );
    } else {
        // Use custom font
        auto it = fonts_.find(font_handle);
        if (it != fonts_.end()) {
            const ::Font* font = get_from_vector<::Font>(it->second.data);
            if (font) {
                DrawTextEx(
                    *font,
                    text.c_str(),
                    to_raylib_vector2(draw_pos),
                    static_cast<float>(font_size),
                    1.0f,
                    to_raylib_color(color)
                );
            }
        }
    }
}

void RaylibGraphicsPlugin::draw_rectangle(const Rectangle& rect, Color color) {
    if (!window_open_) {
        return;
    }
    
    // Apply custom view transformation if needed
    Rectangle draw_rect = rect;
    if (using_custom_view_) {
        draw_rect.x = rect.x - view_center_.x + view_size_.x / 2.0f;
        draw_rect.y = rect.y - view_center_.y + view_size_.y / 2.0f;
    }
    
    ::Rectangle rl_rect = {
        draw_rect.x,
        draw_rect.y,
        draw_rect.width,
        draw_rect.height
    };
    
    DrawRectangleRec(rl_rect, to_raylib_color(color));
}

void RaylibGraphicsPlugin::draw_rectangle_outline(const Rectangle& rect, Color color, float thickness) {
    if (!window_open_) {
        return;
    }
    
    // Apply custom view transformation if needed
    Rectangle draw_rect = rect;
    if (using_custom_view_) {
        draw_rect.x = rect.x - view_center_.x + view_size_.x / 2.0f;
        draw_rect.y = rect.y - view_center_.y + view_size_.y / 2.0f;
    }
    
    ::Rectangle rl_rect = {
        draw_rect.x,
        draw_rect.y,
        draw_rect.width,
        draw_rect.height
    };
    
    DrawRectangleLinesEx(rl_rect, thickness, to_raylib_color(color));
}

void RaylibGraphicsPlugin::draw_circle(Vector2f center, float radius, Color color) {
    if (!window_open_) {
        return;
    }
    
    // Apply custom view transformation if needed
    Vector2f draw_center = center;
    if (using_custom_view_) {
        draw_center.x = center.x - view_center_.x + view_size_.x / 2.0f;
        draw_center.y = center.y - view_center_.y + view_size_.y / 2.0f;
    }
    
    DrawCircleV(to_raylib_vector2(draw_center), radius, to_raylib_color(color));
}

void RaylibGraphicsPlugin::draw_line(Vector2f start, Vector2f end, Color color, float thickness) {
    if (!window_open_) {
        return;
    }
    
    // Apply custom view transformation if needed
    Vector2f draw_start = start;
    Vector2f draw_end = end;
    if (using_custom_view_) {
        draw_start.x = start.x - view_center_.x + view_size_.x / 2.0f;
        draw_start.y = start.y - view_center_.y + view_size_.y / 2.0f;
        draw_end.x = end.x - view_center_.x + view_size_.x / 2.0f;
        draw_end.y = end.y - view_center_.y + view_size_.y / 2.0f;
    }
    
    DrawLineEx(
        to_raylib_vector2(draw_start),
        to_raylib_vector2(draw_end),
        thickness,
        to_raylib_color(color)
    );
}

// Resource loading
TextureHandle RaylibGraphicsPlugin::load_texture(const std::string& path) {
    if (!initialized_) {
        throw std::runtime_error("Plugin not initialized");
    }
    
    // Load texture using Raylib
    Texture2D rl_texture = LoadTexture(path.c_str());
    
    if (rl_texture.id == 0) {
        throw std::runtime_error("Failed to load texture: " + path);
    }
    
    // Create texture data
    TextureData data;
    store_in_vector(data.data, rl_texture);
    data.size = Vector2f(
        static_cast<float>(rl_texture.width),
        static_cast<float>(rl_texture.height)
    );
    
    // Generate handle
    TextureHandle handle = next_texture_handle_++;
    
    // Store in cache
    textures_[handle] = std::move(data);
    
    return handle;
}

void RaylibGraphicsPlugin::unload_texture(TextureHandle handle) {
    auto it = textures_.find(handle);
    if (it != textures_.end()) {
        Texture2D* texture = get_from_vector<Texture2D>(it->second.data);
        if (texture) {
            UnloadTexture(*texture);
        }
        textures_.erase(it);
    }
}

Vector2f RaylibGraphicsPlugin::get_texture_size(TextureHandle handle) const {
    auto it = textures_.find(handle);
    if (it != textures_.end()) {
        return it->second.size;
    }
    return Vector2f(0.0f, 0.0f);
}

FontHandle RaylibGraphicsPlugin::load_font(const std::string& path) {
    if (!initialized_) {
        throw std::runtime_error("Plugin not initialized");
    }
    
    // Load font using Raylib
    ::Font rl_font = LoadFont(path.c_str());
    
    if (rl_font.texture.id == 0) {
        throw std::runtime_error("Failed to load font: " + path);
    }
    
    // Create font data
    FontData data;
    store_in_vector(data.data, rl_font);
    
    // Generate handle
    FontHandle handle = next_font_handle_++;
    
    // Store in cache
    fonts_[handle] = std::move(data);
    
    return handle;
}

void RaylibGraphicsPlugin::unload_font(FontHandle handle) {
    auto it = fonts_.find(handle);
    if (it != fonts_.end()) {
        ::Font* font = get_from_vector<::Font>(it->second.data);
        if (font) {
            UnloadFont(*font);
        }
        fonts_.erase(it);
    }
}

// Camera/View
void RaylibGraphicsPlugin::set_view(Vector2f center, Vector2f size) {
    view_center_ = center;
    view_size_ = size;
    using_custom_view_ = true;
}

void RaylibGraphicsPlugin::reset_view() {
    if (window_open_) {
        view_center_ = Vector2f(
            static_cast<float>(window_width_) / 2.0f,
            static_cast<float>(window_height_) / 2.0f
        );
        view_size_ = Vector2f(
            static_cast<float>(window_width_),
            static_cast<float>(window_height_)
        );
    }
    using_custom_view_ = false;
}

} // namespace rtype

// Plugin factory functions
extern "C" {
    rtype::IGraphicsPlugin* create_graphics_plugin() {
        return new rtype::RaylibGraphicsPlugin();
    }
    
    void destroy_graphics_plugin(rtype::IGraphicsPlugin* plugin) {
        delete plugin;
    }
}

