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

    // Network client
    std::unique_ptr<rtype::client::NetworkClient> network_client_;

    // Game state
    std::atomic<bool> running_;
    uint32_t client_tick_;
    Entity wave_tracker_;

    // Initialization helpers
    bool load_plugins();
    bool load_textures();
    void setup_registry();
    void setup_systems();
    void setup_background();
    void setup_network_callbacks();

    // Update methods
    void update(float delta_time);
    void handle_input();
};

}
