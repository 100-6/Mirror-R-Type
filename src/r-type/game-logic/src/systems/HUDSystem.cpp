/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** HUDSystem implementation - ECS-compliant version
*/

#include "systems/HUDSystem.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

HUDSystem::HUDSystem(engine::IGraphicsPlugin& plugin, int screenWidth, int screenHeight)
    : m_graphicsPlugin(plugin)
    , m_screenWidth(screenWidth)
    , m_screenHeight(screenHeight)
{
}

void HUDSystem::init(Registry& registry) {
    std::cout << "✓ HUDSystem initialized - Creating UI entities..." << std::endl;

    // Register UI components
    registry.register_component<UIPanel>();
    registry.register_component<UIBar>();
    registry.register_component<UIText>();

    // Create health bar panel background
    m_healthPanelEntity = registry.spawn_entity();
    registry.add_component(m_healthPanelEntity, Position{MARGIN, MARGIN});
    registry.add_component(m_healthPanelEntity, UIPanel{
        HEALTH_BAR_WIDTH + 2 * PANEL_PADDING,  // width
        HEALTH_BAR_HEIGHT + 2 * PANEL_PADDING,  // height
        engine::Color{20, 20, 30, 200},         // backgroundColor
        engine::Color{100, 100, 120, 255},      // borderColor
        2.0f,                                    // borderThickness
        true,                                    // active
        100                                      // layer
    });

    // Create health bar
    m_healthBarEntity = registry.spawn_entity();
    registry.add_component(m_healthBarEntity, Position{MARGIN + PANEL_PADDING, MARGIN + PANEL_PADDING});
    registry.add_component(m_healthBarEntity, UIBar{
        HEALTH_BAR_WIDTH,                       // width
        HEALTH_BAR_HEIGHT,                      // height
        100.0f,                                  // currentValue
        100.0f,                                  // maxValue
        engine::Color{40, 40, 50, 255},         // backgroundColor
        engine::Color{0, 255, 0, 255},          // fillColor
        engine::Color{150, 150, 180, 255},      // borderColor
        2.0f,                                    // borderThickness
        true,                                    // active
        101                                      // layer
    });

    // Create health text
    m_healthTextEntity = registry.spawn_entity();
    registry.add_component(m_healthTextEntity, Position{
        MARGIN + PANEL_PADDING + HEALTH_BAR_WIDTH / 2.0f - 50.0f,
        MARGIN + PANEL_PADDING + 5.0f
    });
    registry.add_component(m_healthTextEntity, UIText{
        "100 / 100",                            // text
        engine::Color::White,                    // color
        engine::Color{0, 0, 0, 180},            // shadowColor
        24,                                      // fontSize
        true,                                    // hasShadow
        2.0f,                                    // shadowOffsetX
        2.0f,                                    // shadowOffsetY
        true,                                    // active
        102                                      // layer
    });

    // Create "HEALTH" label
    m_healthLabelEntity = registry.spawn_entity();
    registry.add_component(m_healthLabelEntity, Position{MARGIN + PANEL_PADDING, MARGIN + PANEL_PADDING - 20.0f});
    registry.add_component(m_healthLabelEntity, UIText{
        "HEALTH",
        engine::Color{150, 150, 180, 255},
        engine::Color{0, 0, 0, 180},
        16,
        false,
        2.0f,
        2.0f,
        true,
        102
    });

    // Create score panel
    m_scorePanelEntity = registry.spawn_entity();
    float scoreX = m_screenWidth - MARGIN - 350.0f;
    registry.add_component(m_scorePanelEntity, Position{scoreX, MARGIN});
    registry.add_component(m_scorePanelEntity, UIPanel{
        320.0f,                                  // width
        70.0f,                                   // height
        engine::Color{20, 20, 30, 200},
        engine::Color{255, 215, 0, 255},        // Gold border
        3.0f,
        true,
        100
    });

    // Create score text
    m_scoreTextEntity = registry.spawn_entity();
    registry.add_component(m_scoreTextEntity, Position{scoreX + 15.0f, MARGIN + 35.0f});
    registry.add_component(m_scoreTextEntity, UIText{
        "00000000",
        engine::Color{255, 215, 0, 255},        // Gold
        engine::Color{0, 0, 0, 200},
        32,
        true,
        2.0f,
        2.0f,
        true,
        102
    });

    // Create "SCORE" label
    m_scoreLabelEntity = registry.spawn_entity();
    registry.add_component(m_scoreLabelEntity, Position{scoreX + 15.0f, MARGIN + 10.0f});
    registry.add_component(m_scoreLabelEntity, UIText{
        "SCORE",
        engine::Color{180, 180, 200, 255},
        engine::Color{0, 0, 0, 200},
        20,
        false,
        2.0f,
        2.0f,
        true,
        102
    });

    // Create wave panel (center-top)
    float waveX = (m_screenWidth / 2.0f) - 150.0f;
    m_wavePanelEntity = registry.spawn_entity();
    registry.add_component(m_wavePanelEntity, Position{waveX, MARGIN});
    registry.add_component(m_wavePanelEntity, UIPanel{
        300.0f,                                  // width
        60.0f,                                   // height
        engine::Color{20, 20, 30, 200},
        engine::Color{100, 150, 255, 255},      // Blue border
        3.0f,
        true,
        100
    });

    // Create wave text
    m_waveTextEntity = registry.spawn_entity();
    registry.add_component(m_waveTextEntity, Position{waveX + 150.0f - 80.0f, MARGIN + 15.0f});
    registry.add_component(m_waveTextEntity, UIText{
        "WAVE 0 / 0",
        engine::Color{255, 255, 255, 255},
        engine::Color{0, 0, 0, 200},
        28,
        true,
        2.0f,
        2.0f,
        true,
        102
    });

    std::cout << "✓ HUD UI entities created successfully!" << std::endl;
}

void HUDSystem::shutdown() {
    // Entities will be cleaned up automatically by the registry
}

void HUDSystem::update(Registry& registry, float dt) {
    // Update animation timers
    m_pulseTimer += dt * 2.0f;

    // Get component arrays
    auto& healths = registry.get_components<Health>();
    auto& scores = registry.get_components<Score>();
    auto& localPlayers = registry.get_components<LocalPlayer>();
    auto& uibars = registry.get_components<UIBar>();
    auto& uitexts = registry.get_components<UIText>();
    auto& waveControllers = registry.get_components<WaveController>();
    auto& uipanels = registry.get_components<UIPanel>();
    auto& gameStates = registry.get_components<GameState>();

    // Check if game is over, victory, or not started - hide HUD elements
    bool hideHUD = false;
    for (size_t i = 0; i < gameStates.size(); ++i) {
        Entity entity = gameStates.get_entity_at(i);
        const GameState& state = gameStates[entity];
        if (state.currentState == GameStateType::GAME_OVER ||
            state.currentState == GameStateType::VICTORY) {
            hideHUD = true;
            break;
        }
    }

    // Also hide HUD if no local player exists (game not started yet / waiting for players)
    bool playerExists = localPlayers.size() > 0;
    if (!playerExists) {
        hideHUD = true;
    }

    // Set visibility of HUD elements
    if (uipanels.has_entity(m_healthPanelEntity)) {
        uipanels[m_healthPanelEntity].active = !hideHUD;
    }
    if (uibars.has_entity(m_healthBarEntity)) {
        uibars[m_healthBarEntity].active = !hideHUD;
    }
    if (uitexts.has_entity(m_healthTextEntity)) {
        uitexts[m_healthTextEntity].active = !hideHUD;
    }
    if (uitexts.has_entity(m_healthLabelEntity)) {
        uitexts[m_healthLabelEntity].active = !hideHUD;
    }
    if (uipanels.has_entity(m_scorePanelEntity)) {
        uipanels[m_scorePanelEntity].active = !hideHUD;
    }
    if (uitexts.has_entity(m_scoreTextEntity)) {
        uitexts[m_scoreTextEntity].active = !hideHUD;
    }
    if (uitexts.has_entity(m_scoreLabelEntity)) {
        uitexts[m_scoreLabelEntity].active = !hideHUD;
    }
    if (uipanels.has_entity(m_wavePanelEntity)) {
        uipanels[m_wavePanelEntity].active = !hideHUD;
    }
    if (uitexts.has_entity(m_waveTextEntity)) {
        uitexts[m_waveTextEntity].active = !hideHUD;
    }

    // If HUD is hidden, don't update values
    if (hideHUD) {
        return;
    }

    // Find the player entity
    Entity playerEntity = 0;
    bool playerFound = false;

    for (size_t i = 0; i < localPlayers.size(); ++i) {
        playerEntity = localPlayers.get_entity_at(i);
        playerFound = true;
        break;
    }

    // Update health bar
    if (uibars.has_entity(m_healthBarEntity)) {
        UIBar& healthBar = uibars[m_healthBarEntity];

        if (playerFound && healths.has_entity(playerEntity)) {
            const Health& health = healths[playerEntity];

            // Smooth interpolation
            float targetHealth = static_cast<float>(health.current);
            m_healthBarAnimated = lerp(m_healthBarAnimated, targetHealth, dt * 5.0f);

            healthBar.currentValue = m_healthBarAnimated;
            healthBar.maxValue = static_cast<float>(health.max);

            // Dynamic color based on health percentage
            float healthPercent = health.current / static_cast<float>(health.max);
            healthBar.fillColor = getHealthColor(healthPercent);

            // Update health text
            if (uitexts.has_entity(m_healthTextEntity)) {
                UIText& healthText = uitexts[m_healthTextEntity];
                healthText.text = std::to_string(health.current) + " / " + std::to_string(health.max);
            }
        } else {
            // Player is dead - animate health to 0
            m_healthBarAnimated = lerp(m_healthBarAnimated, 0.0f, dt * 5.0f);
            healthBar.currentValue = m_healthBarAnimated;
            healthBar.fillColor = getHealthColor(0.0f);  // Red color

            // Update health text to show 0
            if (uitexts.has_entity(m_healthTextEntity)) {
                UIText& healthText = uitexts[m_healthTextEntity];
                healthText.text = "0 / 100";
            }
        }
    }

    // Skip score/wave updates if no player found
    if (!playerFound) {
        return;
    }

    // Update score
    if (scores.has_entity(playerEntity) && uitexts.has_entity(m_scoreTextEntity)) {
        const Score& score = scores[playerEntity];
        UIText& scoreText = uitexts[m_scoreTextEntity];

        std::stringstream ss;
        ss << std::setw(8) << std::setfill('0') << score.value;
        scoreText.text = ss.str();
    }

    // Update wave indicator
    for (size_t i = 0; i < waveControllers.size(); ++i) {
        Entity waveEntity = waveControllers.get_entity_at(i);
        const WaveController& waveCtrl = waveControllers[waveEntity];

        if (uitexts.has_entity(m_waveTextEntity)) {
            UIText& waveText = uitexts[m_waveTextEntity];
            auto& positions = registry.get_components<Position>();
            float waveX = (m_screenWidth / 2.0f) - 150.0f;

            if (waveCtrl.currentWaveNumber > 0) {
                waveText.text = "WAVE " + std::to_string(waveCtrl.currentWaveNumber) +
                               " / " + std::to_string(waveCtrl.totalWaveCount);
                waveText.color = engine::Color{255, 255, 255, 255}; // White

                if (positions.has_entity(m_waveTextEntity)) {
                    Position& textPos = positions[m_waveTextEntity];
                    textPos.x = waveX + 150.0f - 80.0f;
                }
            } else {
                waveText.text = "PREPARING...";
                waveText.color = engine::Color{200, 200, 200, 255}; // Gray

                if (positions.has_entity(m_waveTextEntity)) {
                    Position& textPos = positions[m_waveTextEntity];
                    textPos.x = waveX + 150.0f - 70.0f;
                }
            }
        }
        break; // Only one WaveController
    }
}

engine::Color HUDSystem::getHealthColor(float healthPercent) const {
    if (healthPercent > 0.6f) {
        // Green to yellow gradient
        float t = (healthPercent - 0.6f) / 0.4f;
        uint8_t red = static_cast<uint8_t>(255 * (1.0f - t));
        uint8_t green = 255;
        return engine::Color{red, green, 0, 255};
    } else if (healthPercent > 0.3f) {
        // Yellow to orange gradient
        float t = (healthPercent - 0.3f) / 0.3f;
        uint8_t green = static_cast<uint8_t>(255 * t);
        return engine::Color{255, green, 0, 255};
    } else {
        // Orange to red gradient
        float t = healthPercent / 0.3f;
        uint8_t green = static_cast<uint8_t>(165 * t);
        return engine::Color{255, green, 0, 255};
    }
}

float HUDSystem::lerp(float a, float b, float t) const {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return a + (b - a) * t;
}
