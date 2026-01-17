/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GameStateSystem - Manages game states (Playing, Game Over, Victory)
*/

#ifndef GAME_STATE_SYSTEM_HPP
#define GAME_STATE_SYSTEM_HPP

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "ecs/CoreComponents.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include <vector>
#include <string>

/**
 * @brief Leaderboard entry for display in GameStateSystem
 */
struct LeaderboardDisplayEntry {
    uint32_t player_id;
    std::string player_name;
    uint32_t score;
    uint8_t rank;
};

/**
 * @brief System for managing game state transitions and displaying state screens
 *
 * Features:
 * - Detects player death and triggers Game Over
 * - Detects mission completion and triggers Victory
 * - Creates UI entities for overlay screens using UIPanel/UIText components
 * - Handles restart input
 * - Displays leaderboard at end of game
 */
class GameStateSystem : public ISystem {
public:
    /**
     * @brief Constructor
     * @param plugin Graphics plugin for input handling
     * @param screenWidth Width of the screen
     * @param screenHeight Height of the screen
     */
    GameStateSystem(engine::IGraphicsPlugin& plugin, int screenWidth, int screenHeight);
    ~GameStateSystem() override = default;

    /**
     * @brief Initialize the system and create UI entities
     * @param registry ECS registry
     */
    void init(Registry& registry) override;

    /**
     * @brief Update game state and handle transitions
     * @param registry ECS registry
     * @param dt Delta time
     */
    void update(Registry& registry, float dt) override;

    /**
     * @brief Shutdown the system
     */
    void shutdown() override;

    /**
     * @brief Set leaderboard data to display when game ends
     * @param entries Sorted leaderboard entries
     */
    void setLeaderboard(const std::vector<LeaderboardDisplayEntry>& entries);

    /**
     * @brief Check if leaderboard data is available
     */
    bool hasLeaderboard() const { return !m_leaderboardEntries.empty(); }

private:
    engine::IGraphicsPlugin& m_graphicsPlugin;
    int m_screenWidth;
    int m_screenHeight;

    // UI Entity IDs for Game Over/Victory screen
    Entity m_backgroundEntity = 0;
    Entity m_overlayPanelEntity = 0;
    Entity m_titleTextEntity = 0;
    Entity m_scoreTextEntity = 0;
    Entity m_scoreLabelEntity = 0;
    Entity m_gameStateEntity = 0;

    // Background texture
    engine::TextureHandle m_backgroundTexture = engine::INVALID_HANDLE;

    // Animation
    float m_fadeAlpha = 0.0f;
    static constexpr float FADE_SPEED = 2.0f;

    // Input tracking
    bool m_restartKeyPressed = false;

    // Layout constants
    static constexpr float OVERLAY_WIDTH = 500.0f;
    static constexpr float OVERLAY_HEIGHT = 400.0f;  // Increased for leaderboard
    static constexpr int OVERLAY_LAYER = 200;  // Above HUD
    static constexpr int BACKGROUND_LAYER = 199;  // Below overlay but above game

    // Leaderboard data
    std::vector<LeaderboardDisplayEntry> m_leaderboardEntries;

    /**
     * @brief Check if all players are dead
     */
    bool areAllPlayersDead(Registry& registry);

    /**
     * @brief Check if victory conditions are met
     */
    bool isVictoryAchieved(Registry& registry);

    /**
     * @brief Activate the overlay UI entities
     */
    void showOverlay(Registry& registry, bool isVictory, int finalScore);

    /**
     * @brief Deactivate the overlay UI entities
     */
    void hideOverlay(Registry& registry);

    /**
     * @brief Update overlay animation
     */
    void updateOverlayAnimation(Registry& registry, float dt);

    /**
     * @brief Render the leaderboard on the overlay
     */
    void renderLeaderboard();
};

#endif // GAME_STATE_SYSTEM_HPP
