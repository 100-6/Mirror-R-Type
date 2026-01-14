/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ParallaxBackgroundSystem - Multi-layer parallax background scrolling
*/

#ifndef PARALLAX_BACKGROUND_SYSTEM_HPP_
#define PARALLAX_BACKGROUND_SYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/MapTypes.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include <vector>
#include <string>

namespace rtype {

/**
 * @brief Runtime parallax layer data with loaded texture
 */
struct ParallaxLayer {
    std::string path;
    float speedFactor = 1.0f;
    engine::TextureHandle textureHandle = engine::INVALID_HANDLE;
    float offsetX = 0.0f;
    int width = 0;
    int height = 0;
};

/**
 * @brief System for multi-layer parallax background scrolling
 * 
 * Each layer scrolls at a different speed to create depth effect.
 */
class ParallaxBackgroundSystem : public ISystem {
public:
    ParallaxBackgroundSystem(engine::IGraphicsPlugin& graphics, int screenWidth, int screenHeight);
    virtual ~ParallaxBackgroundSystem();

    void init(Registry& registry) override;
    void shutdown() override;
    void update(Registry& registry, float dt) override;
    
    /**
     * @brief Initialize layers from configuration
     * @param layerConfigs Vector of parallax layer configurations
     * @return true if at least one layer loaded successfully
     */
    bool initLayers(const std::vector<ParallaxLayerConfig>& layerConfigs);
    
    /**
     * @brief Update layer offsets based on scroll delta
     * @param scrollDelta Amount scrolled this frame
     */
    void updateScroll(float scrollDelta);
    
    /**
     * @brief Render all parallax layers
     */
    void render() const;
    
    /**
     * @brief Check if layers are loaded and ready to render
     */
    bool isInitialized() const { return m_initialized; }
    
    /**
     * @brief Unload all layers
     */
    void unload();

private:
    engine::IGraphicsPlugin& m_graphics;
    int m_screenWidth;
    int m_screenHeight;
    
    std::vector<ParallaxLayer> m_layers;
    bool m_initialized = false;
};

} // namespace rtype

#endif /* !PARALLAX_BACKGROUND_SYSTEM_HPP_ */
