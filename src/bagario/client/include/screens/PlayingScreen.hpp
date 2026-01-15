#pragma once

#include "screens/BaseScreen.hpp"
#include "ScreenManager.hpp"
#include "network/NetworkManager.hpp"
#include "ClientGameState.hpp"
#include "Camera.hpp"
#include "ui/UILabel.hpp"
#include <memory>
#include <functional>
#include <chrono>

namespace bagario {

/**
 * @brief Main gameplay screen
 *
 * Handles:
 * - Connection to server
 * - Rendering game world (entities, grid, background)
 * - Mouse/keyboard input for gameplay
 * - UI overlay (leaderboard, minimap, score)
 */
class PlayingScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;

    PlayingScreen(LocalGameState& game_state, int screen_width, int screen_height);
    ~PlayingScreen() override;

    void initialize() override;
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) override;
    void draw(engine::IGraphicsPlugin* graphics) override;
    void on_enter() override;
    void on_exit() override;

    void set_screen_change_callback(ScreenChangeCallback callback) {
        on_screen_change_ = callback;
    }

    /**
     * @brief Set the network manager (injected from BagarioGame)
     */
    void set_network_manager(client::NetworkManager* network);

private:
    // ============== Setup ==============
    void setup_network_callbacks();

    // ============== Input Handling ==============
    void handle_mouse_input(engine::IInputPlugin* input);
    void handle_keyboard_input(engine::IInputPlugin* input);

    // ============== Rendering ==============
    void draw_background(engine::IGraphicsPlugin* graphics);
    void draw_grid(engine::IGraphicsPlugin* graphics);
    void draw_entities(engine::IGraphicsPlugin* graphics);
    void draw_entity(engine::IGraphicsPlugin* graphics, const client::CachedEntity& entity);
    void draw_player_cell(engine::IGraphicsPlugin* graphics, const client::CachedEntity& entity);
    void draw_food(engine::IGraphicsPlugin* graphics, const client::CachedEntity& entity);
    void draw_virus(engine::IGraphicsPlugin* graphics, const client::CachedEntity& entity);
    void draw_ejected_mass(engine::IGraphicsPlugin* graphics, const client::CachedEntity& entity);
    void draw_player_names(engine::IGraphicsPlugin* graphics);

    // UI Overlay (screen-space)
    void draw_ui_overlay(engine::IGraphicsPlugin* graphics);
    void draw_leaderboard(engine::IGraphicsPlugin* graphics);
    void draw_minimap(engine::IGraphicsPlugin* graphics);
    void draw_score(engine::IGraphicsPlugin* graphics);
    void draw_connection_status(engine::IGraphicsPlugin* graphics);

    // ============== Helpers ==============
    engine::Color uint32_to_color(uint32_t color) const;
    engine::Color darken_color(const engine::Color& color, float factor) const;
    float get_delta_time();

    ScreenChangeCallback on_screen_change_;

    // Network (not owned, managed by BagarioGame)
    client::NetworkManager* network_ = nullptr;

    // Game state
    std::unique_ptr<client::ClientGameState> client_game_state_;
    std::unique_ptr<client::Camera> camera_;

    // Connection state
    bool is_connecting_ = false;
    bool connection_failed_ = false;
    bool join_requested_ = false;  // Prevents sending multiple join requests
    std::string connection_error_;

    // Input state
    float input_send_timer_ = 0.0f;
    static constexpr float INPUT_SEND_INTERVAL = 0.016f;  // 60Hz input (reduced from 30Hz for lower latency)

    // Delta time tracking
    std::chrono::steady_clock::time_point last_update_time_;
    bool first_update_ = true;
};

}  // namespace bagario
