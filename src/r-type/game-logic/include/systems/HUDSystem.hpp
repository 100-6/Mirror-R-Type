/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** HUDSystem - Modern UI rendering system
*/

#ifndef HUD_SYSTEM_HPP
#define HUD_SYSTEM_HPP

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "ecs/CoreComponents.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include <string>
#include <cmath>

/**
 * @brief System for rendering a modern, stylized HUD with health bars, shields, score
 *
 * Features:
 * - Animated health bar with smooth transitions
 * - Shield indicator with visual effects
 * - Modern score display with background panel
 * - Wave indicator with progress
 * - Speed boost timer display
 * - Weapon type indicator
 */
class HUDSystem : public ISystem {
public:
    /**
     * @brief Constructor
     * @param plugin Graphics plugin for rendering
     * @param inputPlugin Input plugin for edit mode
     * @param screenWidth Width of the screen
     * @param screenHeight Height of the screen
     */
    HUDSystem(engine::IGraphicsPlugin& plugin, engine::IInputPlugin* inputPlugin, int screenWidth, int screenHeight);
    ~HUDSystem() override = default;

    /**
     * @brief Initialize the system
     * @param registry ECS registry
     */
    void init(Registry& registry) override;

    /**
     * @brief Update and render HUD elements
     * @param registry ECS registry
     * @param dt Delta time
     */
    void update(Registry& registry, float dt) override;

    /**
     * @brief Shutdown the system
     */
    void shutdown() override;

    /**
     * @brief Update the lives display on the HUD
     * @param registry Game registry
     * @param lives Number of lives remaining
     */
    void update_lives(Registry& registry, uint8_t lives);

private:
    engine::IGraphicsPlugin& m_graphicsPlugin;
    engine::IInputPlugin* m_inputPlugin;
    int m_screenWidth;
    int m_screenHeight;

    // Animation state
    float m_healthBarAnimated = 100.0f;  // Smoothly interpolated health value
    float m_pulseTimer = 0.0f;           // For pulsing effects
    float m_timeSincePlayerDisappeared = 0.0f;  // Grace period before hiding HUD

    // HUD Layout Constants
    static constexpr float MARGIN = 30.0f;
    static constexpr float HEALTH_BAR_WIDTH = 400.0f;
    static constexpr float HEALTH_BAR_HEIGHT = 35.0f;
    static constexpr float PANEL_PADDING = 15.0f;

    // UI Entity IDs
    Entity m_healthPanelEntity = 0;
    Entity m_healthBarEntity = 0;
    Entity m_healthTextEntity = 0;
    Entity m_healthLabelEntity = 0;
    Entity m_scorePanelEntity = 0;
    Entity m_scoreTextEntity = 0;
    Entity m_scoreLabelEntity = 0;
    Entity m_wavePanelEntity = 0;
    Entity m_waveTextEntity = 0;
    Entity m_livesTextEntity = 0;

    // Edit mode for HUD positioning
    bool m_editMode = false;  // Set to true to enable edit mode
    int m_selectedElement = 0;  // 0=HEALTH, 1=SCORE, 2=WAVE
    float m_moveSpeed = 5.0f;  // Pixels to move per key press

    /**
     * @brief Get color for health percentage
     */
    engine::Color getHealthColor(float healthPercent) const;

    /**
     * @brief Interpolate between two values
     */
    float lerp(float a, float b, float t) const;
};

#endif // HUD_SYSTEM_HPP
