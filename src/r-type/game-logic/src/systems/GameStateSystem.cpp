/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GameStateSystem implementation - Uses UI components for overlay screens
*/

#include "systems/GameStateSystem.hpp"
#include "ecs/events/InputEvents.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>

GameStateSystem::GameStateSystem(engine::IGraphicsPlugin& plugin, int screenWidth, int screenHeight)
    : m_graphicsPlugin(plugin)
    , m_screenWidth(screenWidth)
    , m_screenHeight(screenHeight)
{
}

void GameStateSystem::init(Registry& registry) {
    std::cout << "GameStateSystem initialized - Creating overlay UI entities..." << std::endl;

    // Register GameState component if not already registered
    registry.register_component<GameState>();

    // Subscribe to key press events for restart detection
    auto& eventBus = registry.get_event_bus();
    eventBus.subscribe<ecs::RawKeyPressedEvent>([this](const ecs::RawKeyPressedEvent& event) {
        if (event.key == engine::Key::R) {
            m_restartKeyPressed = true;
        }
    });

    // Load background texture for Game Over/Victory screens
    m_backgroundTexture = m_graphicsPlugin.load_texture("assets/sprite/background_rtype_menu.jpg");
    if (m_backgroundTexture == engine::INVALID_HANDLE) {
        std::cerr << "GameStateSystem: Failed to load background texture!" << std::endl;
    } else {
        std::cout << "GameStateSystem: Background texture loaded successfully" << std::endl;
    }

    // Create game state entity
    m_gameStateEntity = registry.spawn_entity();
    registry.add_component(m_gameStateEntity, GameState{});

    // Create fullscreen background sprite (hidden initially)
    m_backgroundEntity = registry.spawn_entity();
    registry.add_component(m_backgroundEntity, Position{0.0f, 0.0f});
    registry.add_component(m_backgroundEntity, Sprite{
        m_backgroundTexture,
        static_cast<float>(m_screenWidth),
        static_cast<float>(m_screenHeight),
        0.0f,
        engine::Color{255, 255, 255, 0},  // Start fully transparent
        0.0f,
        0.0f,
        BACKGROUND_LAYER
    });

    // Calculate center position for overlay
    float overlayX = (m_screenWidth - OVERLAY_WIDTH) / 2.0f;
    float overlayY = (m_screenHeight - OVERLAY_HEIGHT) / 2.0f;

    // Create overlay panel (semi-transparent dark background)
    m_overlayPanelEntity = registry.spawn_entity();
    registry.add_component(m_overlayPanelEntity, Position{overlayX, overlayY});
    registry.add_component(m_overlayPanelEntity, UIPanel{
        OVERLAY_WIDTH,                          // width
        OVERLAY_HEIGHT,                         // height
        engine::Color{0, 0, 0, 180},           // backgroundColor (semi-transparent black)
        engine::Color{255, 50, 50, 255},       // borderColor (red for Game Over)
        4.0f,                                   // borderThickness
        false,                                  // active (hidden initially)
        OVERLAY_LAYER                           // layer
    });

    // Create title text ("GAME OVER" or "VICTORY")
    // Font 48px, ~24px/char. "GAME OVER" = 9 chars = ~216px, half = 108
    m_titleTextEntity = registry.spawn_entity();
    registry.add_component(m_titleTextEntity, Position{
        overlayX + OVERLAY_WIDTH / 2.0f - 108.0f,
        overlayY + 50.0f
    });
    registry.add_component(m_titleTextEntity, UIText{
        "GAME OVER",                            // text
        engine::Color{255, 50, 50, 255},       // color (red)
        engine::Color{0, 0, 0, 255},           // shadowColor
        48,                                     // fontSize
        true,                                   // hasShadow
        3.0f,                                   // shadowOffsetX
        3.0f,                                   // shadowOffsetY
        false,                                  // active (hidden initially)
        OVERLAY_LAYER + 1                       // layer
    });

    // Create "FINAL SCORE" label - centered
    // Font 24px, ~12px/char. "FINAL SCORE" = 11 chars = ~132px, half = 66
    m_scoreLabelEntity = registry.spawn_entity();
    registry.add_component(m_scoreLabelEntity, Position{
        overlayX + OVERLAY_WIDTH / 2.0f - 66.0f,
        overlayY + 130.0f
    });
    registry.add_component(m_scoreLabelEntity, UIText{
        "FINAL SCORE",
        engine::Color{180, 180, 200, 255},
        engine::Color{0, 0, 0, 200},
        24,
        false,
        2.0f,
        2.0f,
        false,                                  // active (hidden initially)
        OVERLAY_LAYER + 1
    });

    // Create score value text - centered
    // Font 36px, ~18px/char. "00000000" = 8 chars = ~144px, half = 72
    m_scoreTextEntity = registry.spawn_entity();
    registry.add_component(m_scoreTextEntity, Position{
        overlayX + OVERLAY_WIDTH / 2.0f - 72.0f,
        overlayY + 165.0f
    });
    registry.add_component(m_scoreTextEntity, UIText{
        "00000000",
        engine::Color{255, 215, 0, 255},       // Gold
        engine::Color{0, 0, 0, 200},
        36,
        true,
        2.0f,
        2.0f,
        false,                                  // active (hidden initially)
        OVERLAY_LAYER + 1
    });

    std::cout << "GameStateSystem UI entities created successfully!" << std::endl;
}

void GameStateSystem::shutdown() {
    // Entities will be cleaned up by the registry
}

void GameStateSystem::update(Registry& registry, float dt) {
    auto& gameStates = registry.get_components<GameState>();

    if (!gameStates.has_entity(m_gameStateEntity))
        return;

    GameState& state = gameStates[m_gameStateEntity];

    switch (state.currentState) {
        case GameStateType::PLAYING: {
            // Check for game over conditions
            if (areAllPlayersDead(registry)) {
                // Get final score
                int finalScore = 0;
                auto& scores = registry.get_components<Score>();
                auto& controllables = registry.get_components<Controllable>();

                // Try to get score from any remaining controllable or just use 0
                for (size_t i = 0; i < scores.size(); ++i) {
                    Entity entity = scores.get_entity_at(i);
                    finalScore = scores[entity].value;
                    break;
                }

                state.currentState = GameStateType::GAME_OVER;
                state.finalScore = finalScore;
                state.stateTimer = 0.0f;
                m_fadeAlpha = 0.0f;
                showOverlay(registry, false, finalScore);
                std::cout << "GAME OVER! Final Score: " << finalScore << std::endl;
            }

            // Check for victory conditions
            if (isVictoryAchieved(registry)) {
                int finalScore = 0;
                auto& scores = registry.get_components<Score>();
                auto& controllables = registry.get_components<Controllable>();

                for (size_t i = 0; i < controllables.size(); ++i) {
                    Entity entity = controllables.get_entity_at(i);
                    if (scores.has_entity(entity)) {
                        finalScore = scores[entity].value;
                        break;
                    }
                }

                state.currentState = GameStateType::VICTORY;
                state.finalScore = finalScore;
                state.stateTimer = 0.0f;
                m_fadeAlpha = 0.0f;
                showOverlay(registry, true, finalScore);
                std::cout << "VICTORY! Final Score: " << finalScore << std::endl;
            }
            break;
        }

        case GameStateType::GAME_OVER:
        case GameStateType::VICTORY: {
            state.stateTimer += dt;
            updateOverlayAnimation(registry, dt);

            // Check for restart input (R key via event system)
            if (m_restartKeyPressed) {
                state.restartRequested = true;
                m_restartKeyPressed = false;  // Reset for next time
                std::cout << "Restart requested!" << std::endl;
            }
            break;
        }

        case GameStateType::PAUSED:
            // TODO: Implement pause functionality
            break;
    }
}

bool GameStateSystem::areAllPlayersDead(Registry& registry) {
    auto& controllables = registry.get_components<Controllable>();
    auto& healths = registry.get_components<Health>();

    // No players at all = game over
    if (controllables.size() == 0) {
        return true;
    }

    // Check if any player is still alive
    for (size_t i = 0; i < controllables.size(); ++i) {
        Entity entity = controllables.get_entity_at(i);
        if (healths.has_entity(entity)) {
            const Health& health = healths[entity];
            if (health.current > 0) {
                return false;  // At least one player alive
            }
        }
    }

    return true;  // All players dead
}

bool GameStateSystem::isVictoryAchieved(Registry& registry) {
    auto& waveControllers = registry.get_components<WaveController>();
    auto& controllables = registry.get_components<Controllable>();
    auto& enemies = registry.get_components<Enemy>();

    // Need at least one player alive
    if (controllables.size() == 0) {
        return false;
    }

    // Check if all waves have been spawned
    bool allWavesSpawned = false;
    for (size_t i = 0; i < waveControllers.size(); ++i) {
        Entity entity = waveControllers.get_entity_at(i);
        const WaveController& waveCtrl = waveControllers[entity];
        if (waveCtrl.allWavesCompleted) {
            allWavesSpawned = true;
            break;
        }
    }

    // Victory = all waves spawned AND no enemies remaining
    if (allWavesSpawned && enemies.size() == 0) {
        return true;
    }

    return false;
}

void GameStateSystem::showOverlay(Registry& registry, bool isVictory, int finalScore) {
    auto& panels = registry.get_components<UIPanel>();
    auto& texts = registry.get_components<UIText>();
    auto& sprites = registry.get_components<Sprite>();

    // Activate background sprite (will fade in via updateOverlayAnimation)
    if (sprites.has_entity(m_backgroundEntity)) {
        Sprite& bg = sprites[m_backgroundEntity];
        bg.tint.a = 0;  // Start transparent, will fade in
    }

    // Activate panel
    if (panels.has_entity(m_overlayPanelEntity)) {
        UIPanel& panel = panels[m_overlayPanelEntity];
        panel.active = true;
        panel.borderColor = isVictory
            ? engine::Color{0, 255, 100, 255}   // Green for victory
            : engine::Color{255, 50, 50, 255};  // Red for game over
    }

    // Update and activate title
    if (texts.has_entity(m_titleTextEntity)) {
        UIText& title = texts[m_titleTextEntity];
        title.active = true;
        title.text = isVictory ? "VICTORY!" : "GAME OVER";
        title.color = isVictory
            ? engine::Color{0, 255, 100, 255}
            : engine::Color{255, 50, 50, 255};

        // Adjust position based on text length
        // "GAME OVER" = 9 chars, "VICTORY!" = 8 chars at 48px (~24px/char)
        auto& positions = registry.get_components<Position>();
        if (positions.has_entity(m_titleTextEntity)) {
            Position& pos = positions[m_titleTextEntity];
            float overlayX = (m_screenWidth - OVERLAY_WIDTH) / 2.0f;
            // GAME OVER: 9*24=216px, half=108 | VICTORY!: 8*24=192px, half=96
            pos.x = overlayX + OVERLAY_WIDTH / 2.0f - (isVictory ? 96.0f : 108.0f);
        }
    }

    // Activate score label
    if (texts.has_entity(m_scoreLabelEntity)) {
        UIText& label = texts[m_scoreLabelEntity];
        label.active = true;
    }

    // Update and activate score value
    if (texts.has_entity(m_scoreTextEntity)) {
        UIText& score = texts[m_scoreTextEntity];
        score.active = true;
        std::stringstream ss;
        ss << std::setw(8) << std::setfill('0') << finalScore;
        score.text = ss.str();
    }
}

void GameStateSystem::hideOverlay(Registry& registry) {
    auto& panels = registry.get_components<UIPanel>();
    auto& texts = registry.get_components<UIText>();
    auto& sprites = registry.get_components<Sprite>();

    // Hide background
    if (sprites.has_entity(m_backgroundEntity)) {
        sprites[m_backgroundEntity].tint.a = 0;
    }
    if (panels.has_entity(m_overlayPanelEntity)) {
        panels[m_overlayPanelEntity].active = false;
    }
    if (texts.has_entity(m_titleTextEntity)) {
        texts[m_titleTextEntity].active = false;
    }
    if (texts.has_entity(m_scoreLabelEntity)) {
        texts[m_scoreLabelEntity].active = false;
    }
    if (texts.has_entity(m_scoreTextEntity)) {
        texts[m_scoreTextEntity].active = false;
    }
}

void GameStateSystem::updateOverlayAnimation(Registry& registry, float dt) {
    // Fade in effect
    if (m_fadeAlpha < 1.0f) {
        m_fadeAlpha += dt * FADE_SPEED;
        if (m_fadeAlpha > 1.0f) m_fadeAlpha = 1.0f;

        // Update background alpha (fade in to full opacity)
        auto& sprites = registry.get_components<Sprite>();
        if (sprites.has_entity(m_backgroundEntity)) {
            Sprite& bg = sprites[m_backgroundEntity];
            bg.tint.a = static_cast<uint8_t>(255 * m_fadeAlpha);
        }

        // Update panel alpha
        auto& panels = registry.get_components<UIPanel>();
        if (panels.has_entity(m_overlayPanelEntity)) {
            UIPanel& panel = panels[m_overlayPanelEntity];
            panel.backgroundColor.a = static_cast<uint8_t>(180 * m_fadeAlpha);
        }
    }
}
