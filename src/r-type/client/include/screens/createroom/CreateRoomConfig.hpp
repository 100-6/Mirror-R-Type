#pragma once

#include "plugin_manager/CommonTypes.hpp"
#include "AssetsPaths.hpp"

namespace rtype::client::createroom {

/**
 * @brief Configuration constants for CreateRoomScreen layout and styling
 */
struct Config {
    // Screen layout
    static constexpr float CONTENT_START_Y = 410.0f;  // Increased from 340 to add even more padding below stepper
    static constexpr float STEPPER_Y = 100.0f;

    // Map selection
    static constexpr float MAP_PREVIEW_WIDTH = 375.0f;
    static constexpr float MAP_PREVIEW_HEIGHT = 250.0f;
    static constexpr float MAP_PREVIEW_SPACING = 80.0f;
    static constexpr float MAP_BUTTON_WIDTH = 300.0f;  // Increased for better proportions
    static constexpr float MAP_BUTTON_HEIGHT = 70.0f;  // Increased for better readability

    // Difficulty selection (circular)
    static constexpr float DIFFICULTY_CIRCLE_SIZE = 320.0f;
    static constexpr float DIFFICULTY_CIRCLE_SPACING = 120.0f;

    // Game mode selection (circular)
    static constexpr float GAMEMODE_CIRCLE_SIZE = 320.0f;
    static constexpr float GAMEMODE_CIRCLE_SPACING = 120.0f;

    // Navigation buttons
    static constexpr float NAV_BUTTON_WIDTH = 200.0f;
    static constexpr float NAV_BUTTON_HEIGHT = 65.0f;
    static constexpr float NAV_BUTTON_SPACING = 30.0f;
    static constexpr float NAV_BUTTON_Y_OFFSET = 100.0f;

    // Stepper
    static constexpr float STEP_WIDTH = 150.0f;
    static constexpr float STEP_HEIGHT = 8.0f;
    static constexpr float STEP_SPACING = 20.0f;

    // Colors (inline functions to avoid constexpr issues)
    static inline engine::Color BG_DARK() { return {15, 15, 25, 255}; }
    static inline engine::Color BG_GRADIENT_1() { return {20, 25, 45, 200}; }
    static inline engine::Color BG_GRADIENT_2() { return {25, 20, 40, 150}; }
    static inline engine::Color BG_GRADIENT_3() { return {35, 25, 50, 100}; }

    static inline engine::Color STEP_COMPLETED() { return {80, 200, 120, 255}; }
    static inline engine::Color STEP_CURRENT() { return {100, 150, 255, 255}; }
    static inline engine::Color STEP_FUTURE() { return {60, 70, 90, 180}; }

    static inline engine::Color SELECTED_BORDER() { return {150, 100, 255, 255}; }
    static inline engine::Color NORMAL_BORDER() { return {80, 100, 140, 200}; }
    static inline engine::Color SHADOW() { return {0, 0, 0, 120}; }

    // Asset paths
    static constexpr const char* MENU_BACKGROUND = assets::paths::UI_STEPPER_BACKGROUND;

    static constexpr const char* MAP_NEBULA = assets::paths::MAP_THUMBNAIL_NEBULA;
    static constexpr const char* MAP_ASTEROID = assets::paths::MAP_THUMBNAIL_ASTEROID;
    static constexpr const char* MAP_BYDO = assets::paths::MAP_THUMBNAIL_BYDO;

    static constexpr const char* DIFFICULTY_EASY = assets::paths::UI_DIFFICULTY_EASY;
    static constexpr const char* DIFFICULTY_MEDIUM = assets::paths::UI_DIFFICULTY_MEDIUM;
    static constexpr const char* DIFFICULTY_HARD = assets::paths::UI_DIFFICULTY_HARD;

    static constexpr const char* GAMEMODE_DUO = assets::paths::UI_GAMEMODE_DUO;
    static constexpr const char* GAMEMODE_TRIO = assets::paths::UI_GAMEMODE_TRIO;
    static constexpr const char* GAMEMODE_SQUAD = assets::paths::UI_GAMEMODE_SQUAD;
};

}  // namespace rtype::client::createroom
