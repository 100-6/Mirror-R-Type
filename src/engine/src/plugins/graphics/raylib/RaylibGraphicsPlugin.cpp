/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** RaylibGraphicsPlugin - Raylib implementation of IGraphicsPlugin
*/

#include "plugins/graphics/raylib/RaylibGraphicsPlugin.hpp"
#include "plugin_manager/PluginExport.hpp"
#include <raylib.h>
#include <stdexcept>
#include <cstring>
#include <iostream>

namespace engine {

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

// Shader source (GLSL 330 for Raylib)
static const char* RAYLIB_COLORBLIND_SHADER = R"(
#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform int mode; // 0: None, 1: Protanopia, 2: Deuteranopia, 3: Tritanopia

void main() {
    vec4 color = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    
    if (mode == 1) { // Protanopia (Red-blind)
        float r = 0.567 * color.r + 0.433 * color.g;
        float g = 0.558 * color.r + 0.442 * color.g;
        float b = 0.242 * color.g + 0.758 * color.b;
        color.r = r; color.g = g; color.b = b;
    } else if (mode == 2) { // Deuteranopia (Green-blind)
        float r = 0.625 * color.r + 0.375 * color.g;
        float g = 0.700 * color.r + 0.300 * color.g;
        float b = 0.300 * color.g + 0.700 * color.b;
        color.r = r; color.g = g; color.b = b;
    } else if (mode == 3) { // Tritanopia (Blue-blind)
        float r = 0.950 * color.r + 0.050 * color.g;
        float g = 0.433 * color.r + 0.567 * color.g;
        float b = 0.475 * color.g + 0.525 * color.b;
        color.r = r; color.g = g; color.b = b;
    }
    
    finalColor = color;
}
)";

// Constructor
RaylibGraphicsPlugin::RaylibGraphicsPlugin()
    : initialized_(false)
    , window_open_(false)
    , window_width_(0)
    , window_height_(0)
    , next_texture_handle_(1) // Start at 1, 0 is INVALID_HANDLE
    , default_texture_(INVALID_HANDLE)
    , next_font_handle_(1)
    , view_center_{0.0f, 0.0f}
    , view_size_{0.0f, 0.0f}
    , using_custom_view_(false)
    , current_colorblind_mode_(ColorBlindMode::None)
    , shaders_ready_(false) {
}

// Destructor
RaylibGraphicsPlugin::~RaylibGraphicsPlugin() {
    shutdown();
}

void RaylibGraphicsPlugin::init_shaders() {
    // Load shader from memory
    // Raylib LoadShaderFromMemory takes vsCode and fsCode. 0 or NULL for default VS.
    Shader shader = LoadShaderFromMemory(0, RAYLIB_COLORBLIND_SHADER);
    
    if (shader.id != 0) {
        store_in_vector(shader_storage_, shader);
        shaders_ready_ = true;
        std::cout << "RaylibGraphicsPlugin: Colorblind shader loaded successfully" << std::endl;
        
        // Get uniform location
        int modeLoc = GetShaderLocation(shader, "mode");
        int mode = static_cast<int>(current_colorblind_mode_);
        SetShaderValue(shader, modeLoc, &mode, SHADER_UNIFORM_INT);
    } else {
        std::cerr << "RaylibGraphicsPlugin: Failed to load colorblind shader" << std::endl;
    }
}

void RaylibGraphicsPlugin::set_colorblind_mode(ColorBlindMode mode) {
    current_colorblind_mode_ = mode;
    
    if (shaders_ready_ && !shader_storage_.empty()) {
        Shader* shader = get_from_vector<Shader>(shader_storage_);
        if (shader && shader->id != 0) {
            int modeLoc = GetShaderLocation(*shader, "mode");
            int modeInt = static_cast<int>(mode);
            SetShaderValue(*shader, modeLoc, &modeInt, SHADER_UNIFORM_INT);
        }
    }
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

    // Must unload resources BEFORE CloseWindow() destroys the OpenGL context
    // Unloading after CloseWindow() would cause undefined behavior

    // Unload all fonts first (fonts contain CPU-allocated memory: recs, glyphs)
    for (auto& [handle, data] : fonts_) {
        ::Font* font = get_from_vector<::Font>(data.data);
        if (font && font->texture.id != 0) {
            UnloadFont(*font);
            font->texture.id = 0;  // Mark as unloaded to prevent double-free
        }
    }
    fonts_.clear();

    // Unload all textures (GPU resources)
    for (auto& [handle, data] : textures_) {
        Texture2D* texture = get_from_vector<Texture2D>(data.data);
        if (texture && texture->id != 0) {
            UnloadTexture(*texture);
            texture->id = 0;  // Mark as unloaded to prevent double-free
        }
    }
    textures_.clear();

    // Unload render texture
    if (!render_texture_storage_.empty()) {
        RenderTexture2D* target = get_from_vector<RenderTexture2D>(render_texture_storage_);
        if (target && target->id != 0) {
            UnloadRenderTexture(*target);
            target->id = 0;
        }
        render_texture_storage_.clear();
    }

    // Unload shader
    if (!shader_storage_.empty()) {
        Shader* shader = get_from_vector<Shader>(shader_storage_);
        if (shader && shader->id != 0) {
            UnloadShader(*shader);
            shader->id = 0;
        }
        shader_storage_.clear();
    }
    shaders_ready_ = false;

    // Now close the window (destroys OpenGL context)
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

    // Create default pink/black checkerboard texture
    create_default_texture();

    // Initialize shaders and render texture for post-processing
    init_shaders();
    
    RenderTexture2D target = LoadRenderTexture(width, height);
    if (target.id != 0) {
        store_in_vector(render_texture_storage_, target);
    } else {
        std::cerr << "RaylibGraphicsPlugin: Failed to create render texture" << std::endl;
    }

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
    
    RenderTexture2D* target = get_from_vector<RenderTexture2D>(render_texture_storage_);
    
    if (target && target->id != 0) {
        BeginTextureMode(*target);
        ClearBackground(to_raylib_color(color));
        // EndTextureMode is called in display()
    } else {
        BeginDrawing();
        ClearBackground(to_raylib_color(color));
    }
}

void RaylibGraphicsPlugin::display() {
    if (!window_open_) {
        return;
    }
    
    RenderTexture2D* target = get_from_vector<RenderTexture2D>(render_texture_storage_);
    
    if (target && target->id != 0) {
        EndTextureMode();
        
        BeginDrawing();
        ClearBackground(::BLACK);
        
        Shader* shader = nullptr;
        if (shaders_ready_ && !shader_storage_.empty() && current_colorblind_mode_ != ColorBlindMode::None) {
            shader = get_from_vector<Shader>(shader_storage_);
        }
        
        if (shader && shader->id != 0) {
            BeginShaderMode(*shader);
        }
        
        // Draw the render texture to the screen
        // Note: Render textures in Raylib are vertically flipped (OpenGL coordinates)
        ::Rectangle sourceRec = { 0.0f, 0.0f, (float)target->texture.width, -(float)target->texture.height };
        ::Rectangle destRec = { 0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight() };
        Vector2 origin = { 0.0f, 0.0f };
        DrawTexturePro(target->texture, sourceRec, destRec, origin, 0.0f, ::WHITE);
        
        if (shader && shader->id != 0) {
            EndShaderMode();
        }
        
        EndDrawing();
    } else {
        EndDrawing();
    }
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
    
    // Define source rectangle
    // Si source_rect est défini, l'utiliser (pour spritesheets)
    // Sinon, utiliser la texture complète
    ::Rectangle source;
    if (sprite.source_rect.is_valid()) {
        source = {
            sprite.source_rect.x,
            sprite.source_rect.y,
            sprite.source_rect.width,
            sprite.source_rect.height
        };
    } else {
        source = {
            0.0f,
            0.0f,
            static_cast<float>(texture->width),
            static_cast<float>(texture->height)
        };
    }
    
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

TextureHandle RaylibGraphicsPlugin::load_texture_from_memory(const uint8_t* data, size_t size) {
    if (!initialized_) {
        throw std::runtime_error("Plugin not initialized");
    }

    if (data == nullptr || size == 0) {
        throw std::runtime_error("Invalid texture data");
    }

    // Load image from memory - Raylib auto-detects format (PNG, JPG, etc.)
    // We pass ".png" as extension hint but Raylib will detect the actual format
    Image image = LoadImageFromMemory(".png", data, static_cast<int>(size));

    if (image.data == nullptr) {
        throw std::runtime_error("Failed to load image from memory");
    }

    // Convert image to texture
    Texture2D rl_texture = LoadTextureFromImage(image);

    // Free image data (texture is now in GPU memory)
    UnloadImage(image);

    if (rl_texture.id == 0) {
        throw std::runtime_error("Failed to create texture from image");
    }

    // Create texture data
    TextureData tex_data;
    store_in_vector(tex_data.data, rl_texture);
    tex_data.size = Vector2f(
        static_cast<float>(rl_texture.width),
        static_cast<float>(rl_texture.height)
    );

    // Generate handle
    TextureHandle handle = next_texture_handle_++;

    // Store in cache
    textures_[handle] = std::move(tex_data);

    return handle;
}

void RaylibGraphicsPlugin::unload_texture(TextureHandle handle) {
    auto it = textures_.find(handle);
    if (it != textures_.end()) {
        Texture2D* texture = get_from_vector<Texture2D>(it->second.data);
        if (texture && texture->id != 0) {
            UnloadTexture(*texture);
            texture->id = 0;  // Mark as unloaded
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
        if (font && font->texture.id != 0) {
            UnloadFont(*font);
            font->texture.id = 0;  // Mark as unloaded
        }
        fonts_.erase(it);
    }
}

float RaylibGraphicsPlugin::measure_text(const std::string& text, int font_size,
                                          FontHandle font_handle) const {
    if (font_handle == INVALID_HANDLE) {
        return static_cast<float>(MeasureText(text.c_str(), font_size));
    }
    auto it = fonts_.find(font_handle);
    if (it != fonts_.end()) {
        const ::Font* font = get_from_vector<::Font>(it->second.data);
        if (font) {
            Vector2 size = MeasureTextEx(*font, text.c_str(),
                                          static_cast<float>(font_size), 1.0f);
            return size.x;
        }
    }
    return static_cast<float>(MeasureText(text.c_str(), font_size));
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

// Default texture management
void RaylibGraphicsPlugin::create_default_texture() {
    std::cout << "Creating default pink/black checkerboard texture..." << std::endl;

    // Create a 32x32 pink and black checkerboard pattern
    const int size = 32;
    const int check_size = 8; // Size of each checker square

    // Create image with checkerboard pattern
    Image checked = GenImageChecked(size, size, check_size, check_size,
                                    ::Color{255, 0, 255, 255},  // Pink (magenta)
                                    ::Color{0, 0, 0, 255});      // Black

    std::cout << "  Image created: " << checked.width << "x" << checked.height << std::endl;

    // Load texture from image
    Texture2D rl_texture = LoadTextureFromImage(checked);

    std::cout << "  Texture loaded with ID: " << rl_texture.id << std::endl;

    // Free image data (texture is now in GPU memory)
    UnloadImage(checked);

    if (rl_texture.id == 0) {
        // Failed to create default texture - this shouldn't happen
        std::cerr << "  ERROR: Failed to create default texture" << std::endl;
        default_texture_ = INVALID_HANDLE;
        return;
    }

    // Create texture data
    TextureData data;
    store_in_vector(data.data, rl_texture);
    data.size = Vector2f(static_cast<float>(size), static_cast<float>(size));

    // Generate handle
    default_texture_ = next_texture_handle_++;

    std::cout << "  Default texture handle: " << default_texture_ << std::endl;

    // Store in cache
    textures_[default_texture_] = std::move(data);

    std::cout << "  Default texture created successfully!" << std::endl;
}

TextureHandle RaylibGraphicsPlugin::get_default_texture() const {
    return default_texture_;
}

void* RaylibGraphicsPlugin::get_window_handle() const {
    return nullptr;
}

void RaylibGraphicsPlugin::begin_blend_mode(int mode) {
    // Map engine blend modes to Raylib blend modes
    // 0=ALPHA (default), 1=ADDITIVE, 2=MULTIPLIED, 3=ADD_COLORS, 4=SUBTRACT_COLORS
    BeginBlendMode(mode);
}

void RaylibGraphicsPlugin::end_blend_mode() {
    EndBlendMode();
}

}

// Plugin factory functions
extern "C" {
    PLUGIN_API ::engine::IGraphicsPlugin* create_graphics_plugin() {
        return new ::engine::RaylibGraphicsPlugin();
    }

    PLUGIN_API void destroy_graphics_plugin(::engine::IGraphicsPlugin* plugin) {
        delete plugin;
    }
}

