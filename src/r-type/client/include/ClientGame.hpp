#pragma once

#include <memory>
#include <string>
#include <atomic>
#include "ecs/Registry.hpp"
#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "plugin_manager/IAudioPlugin.hpp"
#include "plugin_manager/INetworkPlugin.hpp"
#include "NetworkClient.hpp"
#include "TextureManager.hpp"
#include "ScreenManager.hpp"
#include "EntityManager.hpp"
#include "StatusOverlay.hpp"
#include "InputHandler.hpp"
#include "MenuManager.hpp"
#include "systems/ChunkManagerSystem.hpp"
#include "systems/ParallaxBackgroundSystem.hpp"
#include "systems/ClientPredictionSystem.hpp"
#include "systems/InterpolationSystem.hpp"
#include "DebugNetworkOverlay.hpp"
#include "ui/ConsoleOverlay.hpp"

namespace rtype::client {

/**
 * @brief Main client game class - manages the entire client lifecycle
 */
class ClientGame {
public:
    ClientGame(int screen_width, int screen_height);
    ~ClientGame();

    /**
     * @brief Initialize the game (plugins, systems, network)
     * @param host Server hostname
     * @param tcp_port Server TCP port
     * @param player_name Player name
     * @return true if initialization succeeded
     */
    bool initialize(const std::string& host, uint16_t tcp_port, const std::string& player_name);

    /**
     * @brief Run the main game loop
     */
    void run();

    /**
     * @brief Shutdown and cleanup
     */
    void shutdown();

private:
    // Screen dimensions
    int screen_width_;
    int screen_height_;

    // Connection parameters
    std::string host_;
    uint16_t tcp_port_;
    std::string player_name_;

    // Plugin manager and plugins
    engine::PluginManager plugin_manager_;
    engine::IGraphicsPlugin* graphics_plugin_;
    engine::IInputPlugin* input_plugin_;
    engine::IAudioPlugin* audio_plugin_;
    engine::INetworkPlugin* network_plugin_;

    // ECS registry
    std::unique_ptr<Registry> registry_;

    // Game components
    std::unique_ptr<TextureManager> texture_manager_;
    std::unique_ptr<ScreenManager> screen_manager_;
    std::unique_ptr<EntityManager> entity_manager_;
    std::unique_ptr<StatusOverlay> status_overlay_;
    std::unique_ptr<InputHandler> input_handler_;
    std::unique_ptr<MenuManager> menu_manager_;

    // Map system (new)
    std::unique_ptr<rtype::ParallaxBackgroundSystem> parallax_system_;
    std::unique_ptr<rtype::ChunkManagerSystem> chunk_manager_;
    double map_scroll_x_ = 0.0;  // Use double for precision over long play sessions
    std::string current_map_id_str_ = "nebula_outpost";
    float server_scroll_speed_ = 60.0f;

    // Network client
    std::unique_ptr<rtype::client::NetworkClient> network_client_;

    // Lag compensation system
    std::unique_ptr<rtype::client::ClientPredictionSystem> prediction_system_;
    std::unique_ptr<rtype::client::InterpolationSystem> interpolation_system_;
    std::unique_ptr<rtype::client::DebugNetworkOverlay> debug_network_overlay_;

    // Admin console overlay
    std::unique_ptr<ConsoleOverlay> console_overlay_;
    bool admin_authenticated_ = false;
    std::string admin_password_;

    // Game state
    std::atomic<bool> running_;
    std::atomic<bool> is_shutting_down_{false};
    uint32_t client_tick_;
    Entity wave_tracker_;
    float current_time_;
    uint16_t current_map_id_ = 1;
    int last_known_score_ = 0;

    // Level transition state (prevents scroll desync during transitions)
    bool level_transition_in_progress_ = false;
    float level_transition_timer_ = 0.0f;

    // Audio state for client-side sound triggers
    bool was_shooting_ = false;
    float shoot_sound_cooldown_ = 0.0f;

    // Background entities (legacy, kept for menu)
    Entity background1_;
    Entity background2_;

    // HUD overlay
    engine::TextureHandle hud_texture_;
    bool hud_loaded_ = false;

    // Visual effects
    bool fade_trigger_ = false;

    // Initialization helpers
    bool load_plugins();
    bool load_textures();
    void setup_registry();
    void setup_systems();
    void setup_background();
    void setup_map_system();
    void load_map(const std::string& mapId);
    void setup_network_callbacks();

    // Map-specific theming
    void apply_map_theme(uint16_t map_id);
    void load_level_checkpoints(uint16_t map_id);

    // Update methods
    void update(float delta_time);
    void handle_input();

    // Client-side prediction
    void apply_input_to_local_player(uint16_t input_flags);

    // Admin console
    void handle_console_command(const std::string& command);
};

}
