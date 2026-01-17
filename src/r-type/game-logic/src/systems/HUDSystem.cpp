/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** HUDSystem implementation - ECS-compliant version
*/

#include "systems/HUDSystem.hpp"
#include "AssetsPaths.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

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

    // Load heart texture
    m_heartTexture = m_graphicsPlugin.load_texture(assets::paths::UI_HEART_ICON);

    // Create heart sprites for lives display - centered above health bar
    // Health bar is at x=750 with width=400, so center is at 750 + 200 = 950
    float healthBarCenterX = 750.0f + HEALTH_BAR_WIDTH / 2.0f;
    float livesY = 962.0f - 55.0f;  // 55px above health bar
    float heartSize = 32.0f;  // Size of each heart icon
    float heartSpacing = 10.0f;  // Space between hearts

    // Starting X position to center the hearts (assuming 3 lives initially)
    int initialLives = 3;
    float totalWidth = (initialLives * heartSize) + ((initialLives - 1) * heartSpacing);
    float startX = healthBarCenterX - (totalWidth / 2.0f);

    // Create heart sprite entities
    registry.register_component<Sprite>();
    for (int i = 0; i < MAX_LIVES_DISPLAY; ++i) {
        m_heartEntities[i] = registry.spawn_entity();
        float heartX = startX + (i * (heartSize + heartSpacing));

        registry.add_component(m_heartEntities[i], Position{heartX, livesY});
        registry.add_component(m_heartEntities[i], Sprite{
            m_heartTexture,                      // heart texture
            heartSize,                           // width
            heartSize,                           // height
            0.0f,                                // rotation
            engine::Color{255, 255, 255, 255},  // white tint (full color)
            0.0f,                                // origin_x
            0.0f,                                // origin_y
            102                                  // layer (on top)
        });
    }

    // Hide extra hearts initially (only show 3)
    for (int i = initialLives; i < MAX_LIVES_DISPLAY; ++i) {
        auto& sprites = registry.get_components<Sprite>();
        if (sprites.has_entity(m_heartEntities[i])) {
            sprites[m_heartEntities[i]].tint = engine::Color{255, 255, 255, 0};  // Invisible
        }
    }

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
    // But give a grace period during respawn (2 seconds)
    bool playerExists = localPlayers.size() > 0;
    if (!playerExists) {
        m_timeSincePlayerDisappeared += dt;
        if (m_timeSincePlayerDisappeared >= 2.0f) {  // 2 second grace period
            hideHUD = true;
        }
    } else {
        m_timeSincePlayerDisappeared = 0.0f;  // Reset timer when player exists
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
    if (uitexts.has_entity(m_livesTextEntity)) {
        uitexts[m_livesTextEntity].active = !hideHUD;
    }
    // Wave text is now rendered directly, no need to toggle entity

    // If HUD is hidden, don't update values
    if (hideHUD) {
        return;
    }

    // Find the LOCAL player entity
    // We need to find the entity that has the Score component, not just LocalPlayer/Controllable
    // because the Score might be on a different entity (the network entity)
    Entity playerEntity = 0;
    bool playerFound = false;

    static bool logged_entity = false;

    // Strategy: Find an entity that has BOTH LocalPlayer AND Score components
    // This ensures we get the right entity that receives network score updates
    if (!logged_entity) {
        std::cout << "[HUDSystem] DEBUG - Searching for player entity..." << std::endl;
        std::cout << "[HUDSystem]   Total entities with Score: " << scores.size() << std::endl;
        std::cout << "[HUDSystem]   Total entities with LocalPlayer: " << localPlayers.size() << std::endl;
        for (size_t i = 0; i < scores.size(); ++i) {
            Entity entity = scores.get_entity_at(i);
            std::cout << "[HUDSystem]   Score entity " << entity
                      << ": score=" << scores[entity].value
                      << ", has_LocalPlayer=" << localPlayers.has_entity(entity) << std::endl;
        }
    }

    for (size_t i = 0; i < scores.size(); ++i) {
        Entity entity = scores.get_entity_at(i);
        if (localPlayers.has_entity(entity)) {
            playerEntity = entity;
            playerFound = true;
            if (!logged_entity) {
                std::cout << "[HUDSystem] ✅ Found entity with LocalPlayer + Score: " << playerEntity
                          << " (score: " << scores[playerEntity].value << ")" << std::endl;
                logged_entity = true;
            }
            break;
        }
    }

    // Fallback 1: Find LocalPlayer (for health display even if no score yet)
    if (!playerFound) {
        for (size_t i = 0; i < localPlayers.size(); ++i) {
            playerEntity = localPlayers.get_entity_at(i);
            playerFound = true;
            if (!logged_entity) {
                std::cout << "[HUDSystem] Found LocalPlayer entity (no score): " << playerEntity << std::endl;
                logged_entity = true;
            }
            break;
        }
    }

    // Fallback 2: Use first Controllable
    if (!playerFound) {
        for (size_t i = 0; i < controllables.size(); ++i) {
            playerEntity = controllables.get_entity_at(i);
            playerFound = true;
            if (!logged_entity) {
                std::cout << "[HUDSystem] Fallback to Controllable entity: " << playerEntity << std::endl;
                logged_entity = true;
            }
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
    } else {
        // Debug: log why score isn't updating
        static bool logged = false;
        if (!logged) {
            std::cout << "[HUDSystem] Score debug - playerEntity: " << playerEntity
                      << ", has_score: " << scores.has_entity(playerEntity)
                      << ", has_score_text: " << uitexts.has_entity(m_scoreTextEntity) << std::endl;
            if (scores.has_entity(playerEntity)) {
                std::cout << "[HUDSystem] Score value: " << scores[playerEntity].value << std::endl;
            }
            logged = true;
        }
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

    // Render scoreboard overlay if Tab is pressed
    renderScoreboard(registry);
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

void HUDSystem::update_lives(Registry& registry, uint8_t lives) {
    std::cout << "[HUDSystem] Lives updated: " << static_cast<int>(lives) << std::endl;

    // Update heart sprites visibility and opacity
    auto& sprites = registry.get_components<Sprite>();

    // Clamp lives to max display
    int displayLives = std::min(static_cast<int>(lives), MAX_LIVES_DISPLAY);

    // Show hearts based on lives count
    for (int i = 0; i < MAX_LIVES_DISPLAY; ++i) {
        if (sprites.has_entity(m_heartEntities[i])) {
            if (i < displayLives) {
                // Show heart with full opacity
                sprites[m_heartEntities[i]].tint = engine::Color{255, 255, 255, 255};
            } else {
                // Hide heart (transparent)
                sprites[m_heartEntities[i]].tint = engine::Color{255, 255, 255, 0};
            }
        }
    }

    // Recenter hearts based on current lives count
    if (displayLives > 0) {
        float healthBarCenterX = 750.0f + HEALTH_BAR_WIDTH / 2.0f;
        float heartSize = 32.0f;
        float heartSpacing = 10.0f;
        float totalWidth = (displayLives * heartSize) + ((displayLives - 1) * heartSpacing);
        float startX = healthBarCenterX - (totalWidth / 2.0f);

        auto& positions = registry.get_components<Position>();
        for (int i = 0; i < displayLives; ++i) {
            if (positions.has_entity(m_heartEntities[i])) {
                positions[m_heartEntities[i]].x = startX + (i * (heartSize + heartSpacing));
            }
        }
    }
}

void HUDSystem::update_player_score(uint32_t player_id, const std::string& player_name, uint32_t score) {
    m_playerScores[player_id] = {player_name, score};
}

void HUDSystem::renderScoreboard(Registry& registry) {
    if (!m_showScoreboard)
        return;

    // Semi-transparent background
    float overlayWidth = 350.0f;
    float overlayHeight = 200.0f;
    float overlayX = (m_screenWidth - overlayWidth) / 2.0f;
    float overlayY = 100.0f;

    // Draw background panel
    engine::Rectangle bgRect;
    bgRect.x = overlayX;
    bgRect.y = overlayY;
    bgRect.width = overlayWidth;
    bgRect.height = overlayHeight;
    m_graphicsPlugin.draw_rectangle(bgRect, engine::Color{20, 20, 40, 200});

    // Draw border
    engine::Rectangle borderRect;
    borderRect.x = overlayX;
    borderRect.y = overlayY;
    borderRect.width = overlayWidth;
    borderRect.height = 3.0f;
    m_graphicsPlugin.draw_rectangle(borderRect, engine::Color{100, 100, 200, 255});

    // Draw header
    engine::Vector2f headerPos{overlayX + overlayWidth / 2.0f - 60.0f, overlayY + 15.0f};
    m_graphicsPlugin.draw_text("SCOREBOARD", headerPos, engine::Color{200, 200, 255, 255}, engine::INVALID_HANDLE, 20);

    // Sort players by score descending
    std::vector<std::pair<uint32_t, PlayerScoreInfo>> sortedPlayers(m_playerScores.begin(), m_playerScores.end());
    std::sort(sortedPlayers.begin(), sortedPlayers.end(),
              [](const auto& a, const auto& b) { return a.second.score > b.second.score; });

    // Draw player entries
    float entryY = overlayY + 50.0f;
    float entryHeight = 30.0f;
    int rank = 1;

    for (size_t i = 0; i < sortedPlayers.size() && i < 4; ++i) {
        const auto& [player_id, info] = sortedPlayers[i];

        // Rank color
        engine::Color rankColor;
        switch (i) {
            case 0: rankColor = {255, 215, 0, 255}; break;    // Gold
            case 1: rankColor = {192, 192, 192, 255}; break;  // Silver
            case 2: rankColor = {205, 127, 50, 255}; break;   // Bronze
            default: rankColor = {200, 200, 200, 255}; break;
        }

        // Rank
        std::stringstream rankSs;
        rankSs << "#" << rank++;
        engine::Vector2f rankPos{overlayX + 20.0f, entryY};
        m_graphicsPlugin.draw_text(rankSs.str(), rankPos, rankColor, engine::INVALID_HANDLE, 16);

        // Name (truncate if too long)
        std::string displayName = info.name;
        if (displayName.length() > 12) {
            displayName = displayName.substr(0, 9) + "...";
        }
        engine::Vector2f namePos{overlayX + 60.0f, entryY};
        m_graphicsPlugin.draw_text(displayName, namePos, {255, 255, 255, 255}, engine::INVALID_HANDLE, 16);

        // Score
        std::stringstream scoreSs;
        scoreSs << std::setw(6) << std::setfill('0') << info.score;
        engine::Vector2f scorePos{overlayX + overlayWidth - 100.0f, entryY};
        m_graphicsPlugin.draw_text(scoreSs.str(), scorePos, rankColor, engine::INVALID_HANDLE, 16);

        entryY += entryHeight;
    }

    // If no scores, show message
    if (m_playerScores.empty()) {
        engine::Vector2f msgPos{overlayX + overlayWidth / 2.0f - 50.0f, overlayY + 80.0f};
        m_graphicsPlugin.draw_text("No scores yet", msgPos, {150, 150, 150, 255}, engine::INVALID_HANDLE, 14);
    }
}
