/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ParallaxBackgroundSystem - Implementation
*/

#include "systems/ParallaxBackgroundSystem.hpp"
#include <iostream>
#include <cmath>

namespace rtype {

ParallaxBackgroundSystem::ParallaxBackgroundSystem(engine::IGraphicsPlugin& graphics, int screenWidth, int screenHeight)
    : m_graphics(graphics)
    , m_screenWidth(screenWidth)
    , m_screenHeight(screenHeight)
{
}

ParallaxBackgroundSystem::~ParallaxBackgroundSystem() {
    unload();
}

void ParallaxBackgroundSystem::init(Registry& registry) {
    (void)registry;
    // Initialization done via initLayers()
}

void ParallaxBackgroundSystem::shutdown() {
    unload();
}

void ParallaxBackgroundSystem::update(Registry& registry, float dt) {
    (void)registry;
    (void)dt;
    // Scroll update is done via updateScroll() called externally
}

bool ParallaxBackgroundSystem::initLayers(const std::vector<ParallaxLayerConfig>& layerConfigs) {
    m_layers.clear();
    bool anyLoaded = false;
    
    for (const auto& config : layerConfigs) {
        ParallaxLayer layer;
        layer.path = config.path;
        layer.speedFactor = config.speedFactor;
        layer.textureHandle = m_graphics.load_texture(config.path);
        
        if (layer.textureHandle == engine::INVALID_HANDLE) {
            std::cerr << "Failed to load parallax layer: " << config.path << std::endl;
        } else {
            // Get texture dimensions
            auto dimensions = m_graphics.get_texture_size(layer.textureHandle);
            layer.width = static_cast<int>(dimensions.x);
            layer.height = static_cast<int>(dimensions.y);
            
            std::cout << "Loaded parallax layer: " << config.path 
                      << " (" << layer.width << "x" << layer.height << ")"
                      << " speed=" << layer.speedFactor << std::endl;
            anyLoaded = true;
        }
        
        m_layers.push_back(layer);
    }
    
    m_initialized = anyLoaded;
    return anyLoaded;
}

void ParallaxBackgroundSystem::updateScroll(float scrollDelta) {
    for (auto& layer : m_layers) {
        layer.offsetX += scrollDelta * layer.speedFactor;
        
        // Wrap offset to prevent floating point issues
        if (layer.width > 0) {
            layer.offsetX = std::fmod(layer.offsetX, static_cast<float>(layer.width));
        }
    }
}

void ParallaxBackgroundSystem::render() const {
    for (const auto& layer : m_layers) {
        if (layer.textureHandle == engine::INVALID_HANDLE || layer.width <= 0) {
            continue;
        }
        
        // Calculate scale to fit screen height
        float scale = static_cast<float>(m_screenHeight) / layer.height;
        float scaledWidth = layer.width * scale;
        
        // Calculate how many copies needed to cover the screen
        int numCopies = static_cast<int>(std::ceil(m_screenWidth / scaledWidth)) + 2;
        
        // Starting X position (with wrapping)
        float startX = -std::fmod(layer.offsetX * scale, scaledWidth);
        if (startX > 0) {
            startX -= scaledWidth;
        }
        
        // Draw multiple copies to fill the screen
        for (int i = 0; i < numCopies; ++i) {
            float drawX = startX + i * scaledWidth;
            
            engine::Sprite sprite;
            sprite.texture_handle = layer.textureHandle;
            sprite.size = {scaledWidth, static_cast<float>(m_screenHeight)};
            // No source_rect means use full texture
            
            m_graphics.draw_sprite(sprite, {drawX, 0.0f});
        }
    }
}

void ParallaxBackgroundSystem::unload() {
    if (!m_initialized) return;
    
    // Note: IGraphics handles texture cleanup
    m_layers.clear();
    m_initialized = false;
}

} // namespace rtype
