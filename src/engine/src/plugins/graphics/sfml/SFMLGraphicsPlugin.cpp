/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** SFMLGraphicsPlugin - SFML 3.0 implementation of IGraphicsPlugin
*/

#include "plugins/graphics/sfml/SFMLGraphicsPlugin.hpp"
#include "plugin_manager/PluginExport.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cmath>
#include <optional>

namespace engine {

// Helper functions to convert between engine and SFML types
static sf::Color to_sfml_color(const Color& color) {
    return sf::Color(color.r, color.g, color.b, color.a);
}

static sf::Vector2f to_sfml_vector2(const Vector2f& vec) {
    return sf::Vector2f(vec.x, vec.y);
}

// Constructor
SFMLGraphicsPlugin::SFMLGraphicsPlugin()
    : initialized_(false)
    , window_(nullptr)
    , window_width_(0)
    , window_height_(0)
    , is_fullscreen_(false)
    , next_texture_handle_(1)  // Start at 1, 0 is INVALID_HANDLE
    , next_font_handle_(1)
    , default_texture_(INVALID_HANDLE)
    , default_font_(INVALID_HANDLE)
    , custom_view_(nullptr)
    , view_center_{0.0f, 0.0f}
    , view_size_{0.0f, 0.0f}
    , using_custom_view_(false) {
}

// Destructor
SFMLGraphicsPlugin::~SFMLGraphicsPlugin() {
    shutdown();
}

// IPlugin interface
const char* SFMLGraphicsPlugin::get_name() const {
    return "SFML Graphics Plugin";
}

const char* SFMLGraphicsPlugin::get_version() const {
    return "3.0.0";
}

bool SFMLGraphicsPlugin::initialize() {
    if (initialized_) {
        return true;
    }
    
    // SFML doesn't need explicit initialization beyond window creation
    initialized_ = true;
    return true;
}

void SFMLGraphicsPlugin::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Unload all resources
    textures_.clear();
    fonts_.clear();
    
    // Close window if open
    if (window_ && window_->isOpen()) {
        window_->close();
    }
    window_.reset();
    custom_view_.reset();
    
    initialized_ = false;
}

bool SFMLGraphicsPlugin::is_initialized() const {
    return initialized_;
}

// Global pointer to window for inter-plugin communication
static sf::RenderWindow* g_global_window_ptr = nullptr;

extern "C" {
    sf::RenderWindow* get_sfml_window_ptr() {
        return g_global_window_ptr;
    }
}

// Window management
bool SFMLGraphicsPlugin::create_window(int width, int height, const char* title) {
    if (window_ && window_->isOpen()) {
        return true; // Window already created
    }
    
    window_width_ = width;
    window_height_ = height;
    window_title_ = title;
    
    try {
        window_ = std::make_unique<sf::RenderWindow>(
            sf::VideoMode(sf::Vector2u(static_cast<unsigned>(width), static_cast<unsigned>(height))),
            title,
            sf::Style::Default
        );
        
        if (!window_->isOpen()) {
            return false;
        }

        // Set global pointer
        g_global_window_ptr = window_.get();
        
        // Set default view
        view_center_ = Vector2f(static_cast<float>(width) / 2.0f, static_cast<float>(height) / 2.0f);
        view_size_ = Vector2f(static_cast<float>(width), static_cast<float>(height));
        
        // Create custom view for later use
        custom_view_ = std::make_unique<sf::View>();
        
        // Create default pink/black checkerboard texture
        create_default_texture();
        // Create default font
        create_default_font();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to create SFML window: " << e.what() << std::endl;
        return false;
    }
}

void SFMLGraphicsPlugin::close_window() {
    g_global_window_ptr = nullptr;
    if (window_ && window_->isOpen()) {
        window_->close();
    }
}

bool SFMLGraphicsPlugin::is_window_open() const {
    if (!window_) {
        return false;
    }
    
    // Process events to check for close request
    // Note: In SFML 3.0, pollEvent returns std::optional<sf::Event>
    while (std::optional<sf::Event> event = const_cast<sf::RenderWindow*>(window_.get())->pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            const_cast<sf::RenderWindow*>(window_.get())->close();
            return false;
        }
    }
    
    return window_->isOpen();
}

void SFMLGraphicsPlugin::set_fullscreen(bool fullscreen) {
    if (!window_ || is_fullscreen_ == fullscreen) {
        return;
    }
    
    is_fullscreen_ = fullscreen;
    
    if (fullscreen) {
        window_->create(
            sf::VideoMode::getDesktopMode(),
            window_title_,
            sf::Style::Default,
            sf::State::Fullscreen
        );
    } else {
        window_->create(
            sf::VideoMode(sf::Vector2u(static_cast<unsigned>(window_width_), static_cast<unsigned>(window_height_))),
            window_title_,
            sf::Style::Default,
            sf::State::Windowed
        );
    }
}

void SFMLGraphicsPlugin::set_vsync(bool enabled) {
    if (window_) {
        window_->setVerticalSyncEnabled(enabled);
        if (!enabled) {
            window_->setFramerateLimit(0);
        }
    }
}

// Rendering
void SFMLGraphicsPlugin::clear(Color color) {
    if (!window_ || !window_->isOpen()) {
        return;
    }
    
    window_->clear(to_sfml_color(color));
}

void SFMLGraphicsPlugin::display() {
    if (!window_ || !window_->isOpen()) {
        return;
    }
    
    window_->display();
}

// Drawing primitives
void SFMLGraphicsPlugin::draw_sprite(const Sprite& sprite, Vector2f position) {
    if (!window_ || !window_->isOpen() || sprite.texture_handle == INVALID_HANDLE) {
        return;
    }
    
    auto it = textures_.find(sprite.texture_handle);
    if (it == textures_.end() || !it->second.texture) {
        return;
    }
    
    sf::Sprite sfml_sprite(*it->second.texture);
    
    // Apply custom view transformation if needed
    Vector2f draw_pos = position;
    if (using_custom_view_) {
        draw_pos.x = position.x - view_center_.x + view_size_.x / 2.0f;
        draw_pos.y = position.y - view_center_.y + view_size_.y / 2.0f;
    }
    
    sfml_sprite.setPosition(to_sfml_vector2(draw_pos));
    
    // Set source rectangle if valid (for spritesheets)
    // SFML 3.0: IntRect constructor takes position and size as separate Vector2i
    if (sprite.source_rect.is_valid()) {
        sfml_sprite.setTextureRect(sf::IntRect(
            sf::Vector2i(static_cast<int>(sprite.source_rect.x), static_cast<int>(sprite.source_rect.y)),
            sf::Vector2i(static_cast<int>(sprite.source_rect.width), static_cast<int>(sprite.source_rect.height))
        ));
    }
    
    // Calculate scale to achieve desired size
    sf::FloatRect bounds = sfml_sprite.getLocalBounds();
    if (bounds.size.x > 0 && bounds.size.y > 0) {
        float scaleX = sprite.size.x / bounds.size.x;
        float scaleY = sprite.size.y / bounds.size.y;
        sfml_sprite.setScale(sf::Vector2f(scaleX, scaleY));
    }
    
    // Set origin for rotation
    sfml_sprite.setOrigin(to_sfml_vector2(sprite.origin));
    
    // Set rotation (SFML 3.0 uses sf::degrees)
    sfml_sprite.setRotation(sf::degrees(sprite.rotation));
    
    // Set tint color
    sfml_sprite.setColor(to_sfml_color(sprite.tint));
    
    window_->draw(sfml_sprite);
}

void SFMLGraphicsPlugin::draw_text(const std::string& text, Vector2f position, Color color,
                                   FontHandle font_handle, int font_size) {
    if (!window_ || !window_->isOpen()) {
        return;
    }
    
    // SFML 3.0 requires a font to create text
    if (font_handle == INVALID_HANDLE) {
        // Fallback to default font
        if (default_font_ != INVALID_HANDLE) {
            font_handle = default_font_;
        } else {
            return;
        }
    }
    
    auto it = fonts_.find(font_handle);
    if (it == fonts_.end() || !it->second.font) {
        return;
    }
    
    // Apply custom view transformation if needed
    Vector2f draw_pos = position;
    if (using_custom_view_) {
        draw_pos.x = position.x - view_center_.x + view_size_.x / 2.0f;
        draw_pos.y = position.y - view_center_.y + view_size_.y / 2.0f;
    }
    
    // SFML 3.0: Text constructor requires font
    sf::Text sfml_text(*it->second.font, text, static_cast<unsigned int>(font_size));
    sfml_text.setFillColor(to_sfml_color(color));
    sfml_text.setPosition(to_sfml_vector2(draw_pos));
    
    window_->draw(sfml_text);
}

void SFMLGraphicsPlugin::draw_rectangle(const Rectangle& rect, Color color) {
    if (!window_ || !window_->isOpen()) {
        return;
    }
    
    // Apply custom view transformation if needed
    Rectangle draw_rect = rect;
    if (using_custom_view_) {
        draw_rect.x = rect.x - view_center_.x + view_size_.x / 2.0f;
        draw_rect.y = rect.y - view_center_.y + view_size_.y / 2.0f;
    }
    
    sf::RectangleShape shape(sf::Vector2f(draw_rect.width, draw_rect.height));
    // SFML 3.0: setPosition takes Vector2f
    shape.setPosition(sf::Vector2f(draw_rect.x, draw_rect.y));
    shape.setFillColor(to_sfml_color(color));
    
    window_->draw(shape);
}

void SFMLGraphicsPlugin::draw_rectangle_outline(const Rectangle& rect, Color color, float thickness) {
    if (!window_ || !window_->isOpen()) {
        return;
    }
    
    // Apply custom view transformation if needed
    Rectangle draw_rect = rect;
    if (using_custom_view_) {
        draw_rect.x = rect.x - view_center_.x + view_size_.x / 2.0f;
        draw_rect.y = rect.y - view_center_.y + view_size_.y / 2.0f;
    }
    
    sf::RectangleShape shape(sf::Vector2f(draw_rect.width, draw_rect.height));
    shape.setPosition(sf::Vector2f(draw_rect.x, draw_rect.y));
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(to_sfml_color(color));
    shape.setOutlineThickness(thickness);
    
    window_->draw(shape);
}

void SFMLGraphicsPlugin::draw_circle(Vector2f center, float radius, Color color) {
    if (!window_ || !window_->isOpen()) {
        return;
    }
    
    // Apply custom view transformation if needed
    Vector2f draw_center = center;
    if (using_custom_view_) {
        draw_center.x = center.x - view_center_.x + view_size_.x / 2.0f;
        draw_center.y = center.y - view_center_.y + view_size_.y / 2.0f;
    }
    
    sf::CircleShape shape(radius);
    // SFML circles are positioned by top-left, so offset by radius
    shape.setPosition(sf::Vector2f(draw_center.x - radius, draw_center.y - radius));
    shape.setFillColor(to_sfml_color(color));
    
    window_->draw(shape);
}

void SFMLGraphicsPlugin::draw_line(Vector2f start, Vector2f end, Color color, float thickness) {
    if (!window_ || !window_->isOpen()) {
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
    
    // Calculate line properties
    float dx = draw_end.x - draw_start.x;
    float dy = draw_end.y - draw_start.y;
    float length = std::sqrt(dx * dx + dy * dy);
    float angle = std::atan2(dy, dx) * 180.0f / 3.14159265f;
    
    sf::RectangleShape line(sf::Vector2f(length, thickness));
    line.setPosition(sf::Vector2f(draw_start.x, draw_start.y - thickness / 2.0f));
    // SFML 3.0: setRotation takes sf::Angle
    line.setRotation(sf::degrees(angle));
    line.setFillColor(to_sfml_color(color));
    
    window_->draw(line);
}

// Resource loading
TextureHandle SFMLGraphicsPlugin::load_texture(const std::string& path) {
    if (!initialized_) {
        throw std::runtime_error("Plugin not initialized");
    }
    
    auto texture = std::make_unique<sf::Texture>();
    
    // SFML 3.0: loadFromFile returns bool, but now uses std::optional pattern
    // Try using the newer API if available
    if (!texture->loadFromFile(path)) {
        throw std::runtime_error("Failed to load texture: " + path);
    }
    
    // Create texture data
    TextureData data;
    data.size = Vector2f(
        static_cast<float>(texture->getSize().x),
        static_cast<float>(texture->getSize().y)
    );
    data.texture = std::move(texture);
    
    // Generate handle
    TextureHandle handle = next_texture_handle_++;

    // Store in cache
    textures_[handle] = std::move(data);

    return handle;
}

TextureHandle SFMLGraphicsPlugin::load_texture_from_memory(const uint8_t* data, size_t size) {
    if (!initialized_) {
        throw std::runtime_error("Plugin not initialized");
    }

    if (data == nullptr || size == 0) {
        throw std::runtime_error("Invalid texture data");
    }

    auto texture = std::make_unique<sf::Texture>();

    // SFML 3.0: loadFromMemory loads image data (PNG, JPG, etc.) from memory
    if (!texture->loadFromMemory(data, size)) {
        throw std::runtime_error("Failed to load texture from memory");
    }

    // Create texture data
    TextureData tex_data;
    tex_data.size = Vector2f(
        static_cast<float>(texture->getSize().x),
        static_cast<float>(texture->getSize().y)
    );
    tex_data.texture = std::move(texture);

    // Generate handle
    TextureHandle handle = next_texture_handle_++;

    // Store in cache
    textures_[handle] = std::move(tex_data);

    return handle;
}

void SFMLGraphicsPlugin::unload_texture(TextureHandle handle) {
    textures_.erase(handle);
}

Vector2f SFMLGraphicsPlugin::get_texture_size(TextureHandle handle) const {
    auto it = textures_.find(handle);
    if (it != textures_.end()) {
        return it->second.size;
    }
    return Vector2f(0.0f, 0.0f);
}

FontHandle SFMLGraphicsPlugin::load_font(const std::string& path) {
    if (!initialized_) {
        throw std::runtime_error("Plugin not initialized");
    }
    
    auto font = std::make_unique<sf::Font>();
    
    // SFML 3.0: openFromFile replaces loadFromFile
    if (!font->openFromFile(path)) {
        throw std::runtime_error("Failed to load font: " + path);
    }
    
    // Create font data
    FontData data;
    data.font = std::move(font);
    
    // Generate handle
    FontHandle handle = next_font_handle_++;
    
    // Store in cache
    fonts_[handle] = std::move(data);
    
    return handle;
}

void SFMLGraphicsPlugin::unload_font(FontHandle handle) {
    fonts_.erase(handle);
}

// Camera/View
void SFMLGraphicsPlugin::set_view(Vector2f center, Vector2f size) {
    view_center_ = center;
    view_size_ = size;
    using_custom_view_ = true;
}

void SFMLGraphicsPlugin::reset_view() {
    if (window_) {
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
void SFMLGraphicsPlugin::create_default_texture() {
    std::cout << "Creating default pink/black checkerboard texture..." << std::endl;

    const unsigned int size = 32;
    const unsigned int check_size = 8;

    // SFML 3.0: Image constructor takes size and optional fill color
    sf::Image image(sf::Vector2u(size, size), sf::Color::Black);
    
    for (unsigned int y = 0; y < size; ++y) {
        for (unsigned int x = 0; x < size; ++x) {
            bool is_pink = ((x / check_size) + (y / check_size)) % 2 == 0;
            if (is_pink) {
                // SFML 3.0: setPixel takes Vector2u
                image.setPixel(sf::Vector2u(x, y), sf::Color(255, 0, 255, 255));  // Pink (magenta)
            }
            // Black pixels are already set by the fill color
        }
    }

    std::cout << "  Image created: " << size << "x" << size << std::endl;

    auto texture = std::make_unique<sf::Texture>();
    if (!texture->loadFromImage(image)) {
        std::cerr << "  ERROR: Failed to create default texture" << std::endl;
        default_texture_ = INVALID_HANDLE;
        return;
    }

    std::cout << "  Texture loaded successfully" << std::endl;

    // Create texture data
    TextureData data;
    data.size = Vector2f(static_cast<float>(size), static_cast<float>(size));
    data.texture = std::move(texture);

    // Generate handle
    default_texture_ = next_texture_handle_++;

    std::cout << "  Default texture handle: " << default_texture_ << std::endl;

    // Store in cache
    textures_[default_texture_] = std::move(data);

    std::cout << "  Default texture created successfully!" << std::endl;
}

void SFMLGraphicsPlugin::create_default_font() {
    std::cout << "Loading default font..." << std::endl;
    // Try to load from assets
    // We assume the executable is run from root or build dir, so try a few paths
    std::vector<std::string> paths = {
        "assets/fonts/default.ttf",
        "../assets/fonts/default.ttf",
        "build/assets/fonts/default.ttf"
    };

    for (const auto& path : paths) {
        try {
            default_font_ = load_font(path);
            std::cout << "  Default font loaded from: " << path << std::endl;
            return;
        } catch (...) {
            continue;
        }
    }
    std::cerr << "  WARNING: Failed to load default font from any known path. Text will be invisible." << std::endl;
}

TextureHandle SFMLGraphicsPlugin::get_default_texture() const {
    return default_texture_;
}

void* SFMLGraphicsPlugin::get_window_handle() const {
    return window_.get();
}

void SFMLGraphicsPlugin::begin_blend_mode(int mode) {
    // SFML blend modes are handled per-draw call via sf::BlendMode
    // This is a stub for API compatibility - SFML handles blending differently
    // For proper SFML blending, use sf::BlendMode when drawing sprites
    (void)mode; // Suppress unused parameter warning
}

void SFMLGraphicsPlugin::end_blend_mode() {
    // SFML blend modes are handled per-draw call, no state to reset
}

}

// Plugin factory functions
extern "C" {
    PLUGIN_API ::engine::IGraphicsPlugin* create_graphics_plugin() {
        return new ::engine::SFMLGraphicsPlugin();
    }

    PLUGIN_API void destroy_graphics_plugin(::engine::IGraphicsPlugin* plugin) {
        delete plugin;
    }
}
