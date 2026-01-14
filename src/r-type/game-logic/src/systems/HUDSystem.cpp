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

HUDSystem::HUDSystem(engine::IGraphicsPlugin& plugin, engine::IInputPlugin* inputPlugin, int screenWidth, int screenHeight)
    : m_graphicsPlugin(plugin)
    , m_inputPlugin(inputPlugin)
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
    registry.register_component<LocalPlayer>();

    // Create health bar panel background
    m_healthPanelEntity = registry.spawn_entity();
    registry.add_component(m_healthPanelEntity, Position{750.0f, 962.0f});
    registry.add_component(m_healthPanelEntity, UIPanel{
        HEALTH_BAR_WIDTH + 2 * PANEL_PADDING,  // width
        HEALTH_BAR_HEIGHT + 2 * PANEL_PADDING,  // height
        engine::Color{20, 20, 30, 200},         // backgroundColor
        engine::Color{150, 100, 255, 255},      // borderColor (violet)
        2.0f,                                    // borderThickness
        true,                                    // active
        100                                      // layer
    });

    // Create health bar
    m_healthBarEntity = registry.spawn_entity();
    registry.add_component(m_healthBarEntity, Position{750.0f + PANEL_PADDING, 962.0f + PANEL_PADDING});
    registry.add_component(m_healthBarEntity, UIBar{
        HEALTH_BAR_WIDTH,                       // width
        HEALTH_BAR_HEIGHT,                      // height
        100.0f,                                  // currentValue
        100.0f,                                  // maxValue
        engine::Color{40, 40, 50, 255},         // backgroundColor
        engine::Color{150, 100, 255, 255},      // fillColor (violet)
        engine::Color{150, 100, 255, 255},      // borderColor (violet)
        2.0f,                                    // borderThickness
        true,                                    // active
        101                                      // layer
    });

    // Create health text
    m_healthTextEntity = registry.spawn_entity();
    registry.add_component(m_healthTextEntity, Position{
        750.0f + PANEL_PADDING + HEALTH_BAR_WIDTH / 2.0f - 50.0f,
        962.0f + PANEL_PADDING + 5.0f
    });
    registry.add_component(m_healthTextEntity, UIText{
        "100 / 100",                            // text
        engine::Color{0, 0, 0, 255},            // color (noir/black)
        engine::Color{255, 255, 255, 80},       // shadowColor (white shadow)
        24,                                      // fontSize
        true,                                    // hasShadow
        1.0f,                                    // shadowOffsetX
        1.0f,                                    // shadowOffsetY
        true,                                    // active
        102                                      // layer
    });

    // Create "HEALTH" label
    m_healthLabelEntity = registry.spawn_entity();
    registry.add_component(m_healthLabelEntity, Position{750.0f + PANEL_PADDING, 962.0f + PANEL_PADDING - 20.0f});
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
    float scoreX = 1290.0f;
    registry.add_component(m_scorePanelEntity, Position{scoreX, 93.0f});
    registry.add_component(m_scorePanelEntity, UIPanel{
        320.0f,                                  // width
        70.0f,                                   // height
        engine::Color{20, 20, 30, 200},
        engine::Color{150, 100, 255, 255},      // borderColor (violet)
        3.0f,
        true,
        100
    });

    // Create score text
    m_scoreTextEntity = registry.spawn_entity();
    registry.add_component(m_scoreTextEntity, Position{scoreX + 15.0f, 93.0f + 35.0f});
    registry.add_component(m_scoreTextEntity, UIText{
        "00000000",
        engine::Color{150, 100, 255, 255},      // color (violet)
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
    registry.add_component(m_scoreLabelEntity, Position{scoreX + 15.0f, 93.0f + 10.0f});
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

    // Wave text will be rendered directly in update() instead of using UIText entity

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
    auto& controllables = registry.get_components<Controllable>();
    auto& uipanels = registry.get_components<UIPanel>();
    auto& gameStates = registry.get_components<GameState>();
    auto& positions = registry.get_components<Position>();

    // EDIT MODE for positioning HUD elements
    if (m_editMode && m_inputPlugin) {
        // Select element with 1/2/3
        if (m_inputPlugin->is_key_pressed(engine::Key::Num1)) {
            m_selectedElement = 0;  // HEALTH
            std::cout << "[HUD Edit Mode] Selected HEALTH" << std::endl;
        }
        if (m_inputPlugin->is_key_pressed(engine::Key::Num2)) {
            m_selectedElement = 1;  // SCORE
            std::cout << "[HUD Edit Mode] Selected SCORE" << std::endl;
        }
        if (m_inputPlugin->is_key_pressed(engine::Key::Num3)) {
            m_selectedElement = 2;  // WAVE
            std::cout << "[HUD Edit Mode] Selected WAVE" << std::endl;
        }

        // Get the entity to move based on selection
        Entity selectedEntity = 0;
        if (m_selectedElement == 0) selectedEntity = m_healthPanelEntity;
        else if (m_selectedElement == 1) selectedEntity = m_scorePanelEntity;
        else if (m_selectedElement == 2) selectedEntity = m_waveTextEntity;  // Move text directly instead of panel

        if (positions.has_entity(selectedEntity)) {
            Position& pos = positions[selectedEntity];
            float originalX = pos.x;
            float originalY = pos.y;

            // Move with arrow keys or numpad (4=left, 6=right, 8=up, 2=down)
            if (m_inputPlugin->is_key_pressed(engine::Key::Left) || m_inputPlugin->is_key_pressed(engine::Key::Numpad4)) {
                pos.x -= m_moveSpeed;
            }
            if (m_inputPlugin->is_key_pressed(engine::Key::Right) || m_inputPlugin->is_key_pressed(engine::Key::Numpad6)) {
                pos.x += m_moveSpeed;
            }
            if (m_inputPlugin->is_key_pressed(engine::Key::Up) || m_inputPlugin->is_key_pressed(engine::Key::Numpad8)) {
                pos.y -= m_moveSpeed;
            }
            if (m_inputPlugin->is_key_pressed(engine::Key::Down) || m_inputPlugin->is_key_pressed(engine::Key::Numpad2)) {
                pos.y += m_moveSpeed;
            }

            // Print coordinates with P
            if (m_inputPlugin->is_key_pressed(engine::Key::P)) {
                std::cout << "\n[HUD Edit Mode] Current Positions:" << std::endl;
                if (m_selectedElement == 0) {
                    std::cout << "HEALTH: x=" << pos.x << " y=" << pos.y << std::endl;
                } else if (m_selectedElement == 1) {
                    std::cout << "SCORE: x=" << pos.x << " y=" << pos.y << std::endl;
                } else if (m_selectedElement == 2) {
                    std::cout << "WAVE: x=" << pos.x << " y=" << pos.y << std::endl;
                }
            }
        }

        // Draw visual indicator for selected element
        if (positions.has_entity(selectedEntity)) {
            const Position& pos = positions[selectedEntity];
            float width = 150, height = 30;  // Default size for text

            if (uipanels.has_entity(selectedEntity)) {
                const UIPanel& panel = uipanels[selectedEntity];
                width = panel.width;
                height = panel.height;
            }

            // Draw red outline around selected element
            engine::Rectangle outline{pos.x - 5, pos.y - 5, width + 10, height + 10};
            m_graphicsPlugin.draw_rectangle(outline, {255, 0, 0, 255});

            // Draw label
            std::string label = "";
            if (m_selectedElement == 0) label = "HEALTH (Selected)";
            else if (m_selectedElement == 1) label = "SCORE (Selected)";
            else if (m_selectedElement == 2) label = "WAVE TEXT (Selected)";

            m_graphicsPlugin.draw_text(label, {pos.x, pos.y - 30}, {255, 0, 0, 255}, engine::INVALID_HANDLE, 20);
        }

        // Draw instructions
        m_graphicsPlugin.draw_text("Edit Mode: 1/2/3=Select | Arrows or Numpad 4826=Move | P=Print Coords",
                                   {10, static_cast<float>(m_screenHeight - 30)}, {255, 255, 0, 255}, engine::INVALID_HANDLE, 16);
    }

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
    // Wave text is now rendered directly, no need to toggle entity

    // If HUD is hidden, don't update values
    if (hideHUD) {
        return;
    }

    // Find the LOCAL player entity (entity with LocalPlayer component)
    Entity playerEntity = 0;
    bool playerFound = false;


    for (size_t i = 0; i < localPlayers.size(); ++i) {
        playerEntity = localPlayers.get_entity_at(i);
        playerFound = true;
        break;
    }

    // Fallback: if no LocalPlayer component found, use first Controllable
    if (!playerFound) {
        for (size_t i = 0; i < controllables.size(); ++i) {
            playerEntity = controllables.get_entity_at(i);
            playerFound = true;
            break;
        }
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

            // Keep violet color always
            healthBar.fillColor = engine::Color{150, 100, 255, 255};

            // Update health text
            if (uitexts.has_entity(m_healthTextEntity)) {
                UIText& healthText = uitexts[m_healthTextEntity];
                healthText.text = std::to_string(health.current) + " / " + std::to_string(health.max);
            }
        } else {
            // Player is dead - animate health to 0
            m_healthBarAnimated = lerp(m_healthBarAnimated, 0.0f, dt * 5.0f);
            healthBar.currentValue = m_healthBarAnimated;
            healthBar.fillColor = engine::Color{150, 100, 255, 255};  // Keep violet even at 0

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
    // Render wave indicator directly with custom styling (no UIText entity)
    for (size_t i = 0; i < waveControllers.size(); ++i) {
        Entity waveEntity = waveControllers.get_entity_at(i);
        const WaveController& waveCtrl = waveControllers[waveEntity];

        // Position for wave display
        float waveX = 105.0f;
        float waveY = 95.0f;

        std::string waveText;
        if (waveCtrl.currentWaveNumber > 0) {
            waveText = "WAVE " + std::to_string(waveCtrl.currentWaveNumber) +
                       " / " + std::to_string(waveCtrl.totalWaveCount);
        } else {
            waveText = "PREPARING...";
        }

        // Draw shadow for depth
        engine::Vector2f shadowPos{waveX + 2, waveY + 2};
        m_graphicsPlugin.draw_text(waveText, shadowPos, {0, 0, 0, 200}, engine::INVALID_HANDLE, 32);

        // Draw main text in violet
        engine::Vector2f textPos{waveX, waveY};
        m_graphicsPlugin.draw_text(waveText, textPos, {150, 100, 255, 255}, engine::INVALID_HANDLE, 32);

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
