/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Main Client - Multiplayer Game with Network Synchronization
*/

#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <unordered_map>
#include <arpa/inet.h>

#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "components/CombatHelpers.hpp"
#include "cmath"

// Systems (minimal for multiplayer client - server handles game logic)
#include "ecs/systems/InputSystem.hpp"
#include "ecs/systems/SpriteAnimationSystem.hpp"
#include "systems/ScrollingSystem.hpp"
#include "ecs/systems/DestroySystem.hpp"
#include "ecs/systems/RenderSystem.hpp"
#include "ecs/systems/AudioSystem.hpp"
#include "systems/HitEffectSystem.hpp"
#include "systems/HUDSystem.hpp"
#include "systems/AttachmentSystem.hpp"

// Plugins
#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "plugin_manager/IAudioPlugin.hpp"
#include "plugin_manager/INetworkPlugin.hpp"

// Network
#include "NetworkClient.hpp"
#include "protocol/Payloads.hpp"
#include "protocol/PacketTypes.hpp"
#include "protocol/NetworkConfig.hpp"

using namespace rtype::protocol;

static std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    std::cout << "\n[Client] Signal " << signal << " received, stopping...\n";
    g_running = false;
}

// Structure to track network entities
struct NetworkEntity {
    Entity entity;
    EntityType type;
    float last_x, last_y;
    uint32_t owner_player_id;  // For players, their player_id
    bool is_local_player;
};

int main(int argc, char* argv[]) {
    std::string server_host = "localhost";
    uint16_t server_port = config::DEFAULT_TCP_PORT;
    std::string player_name = "Player";

    // Parse command line arguments
    // Usage: ./r-type_multiplayer [host] [player_name] [port]
    // Or:    ./r-type_multiplayer [host] [port] [player_name] (if port is numeric)
    if (argc > 1) server_host = argv[1];
    if (argc > 2) {
        // Check if argv[2] is a number (port) or string (player name)
        try {
            int port = std::stoi(argv[2]);
            server_port = static_cast<uint16_t>(port);
            if (argc > 3) player_name = argv[3];
        } catch (const std::invalid_argument&) {
            // argv[2] is not a number, treat it as player name
            player_name = argv[2];
            if (argc > 3) {
                try {
                    server_port = static_cast<uint16_t>(std::stoi(argv[3]));
                } catch (...) {}
            }
        }
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Configuration
    const int SCREEN_WIDTH = 1920;
    const int SCREEN_HEIGHT = 1080;

    std::cout << "=== R-Type Client - Multiplayer ===" << std::endl;
    std::cout << "Server: " << server_host << ":" << server_port << std::endl;
    std::cout << "Player: " << player_name << std::endl;
    std::cout << std::endl;

    // ====================
    // LOAD PLUGINS
    // ====================
    engine::PluginManager pluginManager;
    engine::IGraphicsPlugin* graphicsPlugin = nullptr;
    engine::IInputPlugin* inputPlugin = nullptr;
    engine::IAudioPlugin* audioPlugin = nullptr;
    engine::INetworkPlugin* networkPlugin = nullptr;

    std::cout << "Loading plugins..." << std::endl;

    try {
        graphicsPlugin = pluginManager.load_plugin<engine::IGraphicsPlugin>(
            "plugins/raylib_graphics.so", "create_graphics_plugin");
        inputPlugin = pluginManager.load_plugin<engine::IInputPlugin>(
            "plugins/raylib_input.so", "create_input_plugin");
        networkPlugin = pluginManager.load_plugin<engine::INetworkPlugin>(
            "plugins/asio_network.so", "create_network_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "Plugin load error: " << e.what() << std::endl;
        return 1;
    }

    if (!graphicsPlugin || !inputPlugin || !networkPlugin) {
        std::cerr << "Required plugins not available" << std::endl;
        return 1;
    }

    // Optional audio plugin
    try {
        audioPlugin = pluginManager.load_plugin<engine::IAudioPlugin>(
            "plugins/miniaudio_audio.so", "create_audio_plugin");
    } catch (...) {
        std::cout << "Audio plugin not available (continuing without sound)" << std::endl;
    }

    // Initialize network plugin
    if (!networkPlugin->initialize()) {
        std::cerr << "Failed to initialize network plugin" << std::endl;
        return 1;
    }

    std::cout << "Plugins loaded successfully" << std::endl;

    // Create window
    if (!graphicsPlugin->create_window(SCREEN_WIDTH, SCREEN_HEIGHT, "R-Type - Multiplayer")) {
        std::cerr << "Failed to create window" << std::endl;
        return 1;
    }
    graphicsPlugin->set_vsync(true);

    // ====================
    // LOAD TEXTURES
    // ====================
    engine::TextureHandle backgroundTex = graphicsPlugin->load_texture("assets/sprite/symmetry.png");
    engine::TextureHandle menuBackgroundTex = graphicsPlugin->load_texture("assets/sprite/background_rtype_menu.jpg");
    engine::TextureHandle playerTex1 = graphicsPlugin->load_texture("assets/sprite/ship1.png");
    engine::TextureHandle playerTex2 = graphicsPlugin->load_texture("assets/sprite/ship2.png");
    engine::TextureHandle playerTex3 = graphicsPlugin->load_texture("assets/sprite/ship3.png");
    engine::TextureHandle playerTex4 = graphicsPlugin->load_texture("assets/sprite/ship4.png");
    engine::TextureHandle bulletTex = graphicsPlugin->load_texture("assets/sprite/bullet.png");
    engine::TextureHandle enemyTex = graphicsPlugin->load_texture("assets/sprite/enemy.png");
    engine::TextureHandle wallTex = graphicsPlugin->load_texture("assets/sprite/lock.png");

    if (backgroundTex == 0 || playerTex1 == 0 || bulletTex == 0) {
        std::cerr << "Failed to load textures" << std::endl;
        graphicsPlugin->shutdown();
        return 1;
    }

    engine::Vector2f playerSize = graphicsPlugin->get_texture_size(playerTex1);
    const float PLAYER_SCALE = 0.20f;
    float playerWidth = playerSize.x * PLAYER_SCALE;
    float playerHeight = playerSize.y * PLAYER_SCALE;

    // ====================
    // CREATE REGISTRY
    // ====================
    Registry registry;
    registry.register_component<Position>();
    registry.register_component<Velocity>();
    registry.register_component<Input>();
    registry.register_component<Collider>();
    registry.register_component<Sprite>();
    registry.register_component<Controllable>();
    registry.register_component<Enemy>();
    registry.register_component<Projectile>();
    registry.register_component<Wall>();
    registry.register_component<Health>();
    registry.register_component<HitFlash>();
    registry.register_component<Damage>();
    registry.register_component<ToDestroy>();
    registry.register_component<Weapon>();
    registry.register_component<Score>();
    registry.register_component<Background>();
    registry.register_component<Invulnerability>();
    registry.register_component<AI>();
    registry.register_component<Scrollable>();
    registry.register_component<NoFriction>();
    registry.register_component<WaveController>();
    registry.register_component<Bonus>();
    registry.register_component<Shield>();
    registry.register_component<SpeedBoost>();
    registry.register_component<CircleEffect>();
    registry.register_component<TextEffect>();
    registry.register_component<SpriteAnimation>();
    registry.register_component<Attached>();
    registry.register_component<GameState>();

    // ====================
    // REGISTER SYSTEMS (minimal for multiplayer - server handles ALL game logic)
    // Client only: captures input, animates sprites, renders
    // ====================
    registry.register_system<InputSystem>(*inputPlugin);           // Capture key states
    registry.register_system<ScrollingSystem>(-100.0f, static_cast<float>(SCREEN_WIDTH));  // Background scrolling
    registry.register_system<SpriteAnimationSystem>();             // Visual animations only
    registry.register_system<HitEffectSystem>();                   // Visual hit effects
    registry.register_system<AttachmentSystem>();                  // Attached entities
    if (audioPlugin) {
        registry.register_system<AudioSystem>(*audioPlugin);       // Sound effects
    }
    registry.register_system<DestroySystem>();                     // Cleanup destroyed entities
    registry.register_system<RenderSystem>(*graphicsPlugin);       // Draw sprites
    registry.register_system<HUDSystem>(*graphicsPlugin, SCREEN_WIDTH, SCREEN_HEIGHT);  // UI

    // ====================
    // CREATE BACKGROUND
    // ====================
    Entity background1 = registry.spawn_entity();
    registry.add_component(background1, Position{0.0f, 0.0f});
    registry.add_component(background1, Background{});
    registry.add_component(background1, Scrollable{1.0f, true, false});
    registry.add_component(background1, Sprite{
        backgroundTex, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT),
        0.0f, engine::Color::White, 0.0f, 0.0f, -100
    });

    Entity background2 = registry.spawn_entity();
    registry.add_component(background2, Position{static_cast<float>(SCREEN_WIDTH), 0.0f});
    registry.add_component(background2, Background{});
    registry.add_component(background2, Scrollable{1.0f, true, false});
    registry.add_component(background2, Sprite{
        backgroundTex, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT),
        0.0f, engine::Color::White, 0.0f, 0.0f, -100
    });

    // ====================
    // NETWORK CLIENT
    // ====================
    rtype::client::NetworkClient client(*networkPlugin);

    // Network entity mapping (server entity ID -> local Entity)
    std::unordered_map<uint32_t, NetworkEntity> networkEntities;
    Entity localPlayerEntity = 0;
    uint32_t localPlayerId = 0;
    bool gameStarted = false;

    // Color for other players (darker/tinted)
    engine::Color remotePlayerColor = {150, 150, 200, 255};  // Bluish tint

    // Callbacks
    client.set_on_accepted([&](uint32_t player_id) {
        std::cout << "[Network] Connected! Player ID: " << player_id << std::endl;
        localPlayerId = player_id;
    });

    client.set_on_rejected([](uint8_t reason, const std::string& message) {
        std::cerr << "[Network] Connection rejected: " << message << std::endl;
        g_running = false;
    });

    client.set_on_lobby_state([](const ServerLobbyStatePayload& state) {
        std::cout << "[Lobby] " << (int)state.current_player_count << "/"
                  << (int)state.required_player_count << " players" << std::endl;
    });

    client.set_on_countdown([](uint8_t seconds) {
        std::cout << "[Lobby] Game starts in " << (int)seconds << "s..." << std::endl;
    });

    client.set_on_game_start([&](uint32_t session_id, uint16_t udp_port) {
        std::cout << "[Game] STARTING! Session: " << session_id << std::endl;
        gameStarted = true;
    });

    client.set_on_entity_spawn([&](const ServerEntitySpawnPayload& spawn) {
        uint32_t net_id = ntohl(spawn.entity_id);

        // Check if entity already exists
        if (networkEntities.find(net_id) != networkEntities.end()) {
            return;
        }

        Entity e = registry.spawn_entity();
        registry.add_component(e, Position{spawn.spawn_x, spawn.spawn_y});
        registry.add_component(e, Velocity{0.0f, 0.0f});

        bool isLocalPlayer = false;
        uint32_t ownerPlayerId = 0;

        // Determine texture and components based on entity type
        if (spawn.entity_type == EntityType::PLAYER) {
            // For players, subtype contains the player_id (low byte)
            ownerPlayerId = spawn.subtype;
            isLocalPlayer = (ownerPlayerId == (localPlayerId & 0xFF));

            // Use different color for remote players
            engine::Color playerColor = isLocalPlayer ? engine::Color::White : remotePlayerColor;

            registry.add_component(e, Sprite{
                playerTex1, playerWidth, playerHeight,
                0.0f, playerColor, 0.0f, 0.0f, 1
            });
            registry.add_component(e, SpriteAnimation{
                {playerTex1, playerTex2, playerTex3, playerTex4},
                0.10f, 0.0f, 0, true, true
            });
            registry.add_component(e, Collider{playerWidth, playerHeight});

            // Use health from spawn payload
            uint16_t health = ntohs(spawn.health);
            registry.add_component(e, Health{static_cast<int>(health), static_cast<int>(health)});

            if (isLocalPlayer) {
                localPlayerEntity = e;
                registry.add_component(e, Input{});
                registry.add_component(e, Controllable{300.0f});
                registry.add_component(e, Score{0});
                registry.add_component(e, Weapon{});
                std::cout << "[Game] Local player entity created: " << e << " (player_id=" << localPlayerId << ")" << std::endl;
            } else {
                std::cout << "[Game] Remote player entity created: " << e << " (player_id=" << (int)ownerPlayerId << ")" << std::endl;
            }
        } else if (spawn.entity_type == EntityType::WALL) {
            // Wall entity
            const float WALL_SIZE = 64.0f;
            if (wallTex != 0) {
                registry.add_component(e, Sprite{
                    wallTex, WALL_SIZE, WALL_SIZE,
                    0.0f, engine::Color::White, 0.0f, 0.0f, 0
                });
            }
            registry.add_component(e, Collider{WALL_SIZE, WALL_SIZE});
            registry.add_component(e, Wall{});
            registry.add_component(e, NoFriction{});
            std::cout << "[Spawn] Wall " << net_id << " at (" << spawn.spawn_x << ", " << spawn.spawn_y << ")" << std::endl;
        } else if (spawn.entity_type == EntityType::PROJECTILE) {
            // Projectile entity
            const float BULLET_WIDTH = 32.0f;
            const float BULLET_HEIGHT = 8.0f;
            if (bulletTex != 0) {
                registry.add_component(e, Sprite{
                    bulletTex, BULLET_WIDTH, BULLET_HEIGHT,
                    0.0f, engine::Color::White, 0.0f, 0.0f, 2
                });
            }
            registry.add_component(e, Collider{BULLET_WIDTH, BULLET_HEIGHT});
            registry.add_component(e, Projectile{0.0f, 5.0f, 0.0f, ProjectileFaction::Player});
            registry.add_component(e, NoFriction{});
            std::cout << "[Spawn] Projectile " << net_id << " at (" << spawn.spawn_x << ", " << spawn.spawn_y << ")" << std::endl;
        } else {
            // Enemy entity (default for ENEMY_BASIC, ENEMY_FAST, ENEMY_TANK, ENEMY_BOSS)
            // Use much larger scale for better visibility (2.1 = 6x the original 0.35)
            const float ENEMY_SCALE = 2.1f;
            float size = 64.0f;
            if (enemyTex != 0) {
                engine::Vector2f enemySize = graphicsPlugin->get_texture_size(enemyTex);
                float scaledWidth = enemySize.x * ENEMY_SCALE;
                float scaledHeight = enemySize.y * ENEMY_SCALE;
                registry.add_component(e, Sprite{
                    enemyTex, scaledWidth, scaledHeight,
                    0.0f, engine::Color::White, 0.0f, 0.0f, 0
                });
                size = scaledWidth;
            }
            registry.add_component(e, Collider{size, size});
            registry.add_component(e, Enemy{});
            registry.add_component(e, NoFriction{});
            std::cout << "[Spawn] Enemy " << net_id << " at (" << spawn.spawn_x << ", " << spawn.spawn_y << ")" << std::endl;
        }

        networkEntities[net_id] = {e, spawn.entity_type, spawn.spawn_x, spawn.spawn_y, ownerPlayerId, isLocalPlayer};
    });

    client.set_on_entity_destroy([&](const ServerEntityDestroyPayload& destroy) {
        uint32_t net_id = ntohl(destroy.entity_id);
        auto it = networkEntities.find(net_id);
        if (it != networkEntities.end()) {
            if (it->second.entity == localPlayerEntity) {
                localPlayerEntity = 0;  // Our player was destroyed
            }
            registry.kill_entity(it->second.entity);
            networkEntities.erase(it);
            std::cout << "[Destroy] Entity " << net_id << std::endl;
        }
    });

    // Snapshot callback - update ALL positions and health from server state
    // In authoritative server architecture, ALL positions come from the server
    client.set_on_snapshot([&](const std::vector<EntityState>& entities) {
        auto& positions = registry.get_components<Position>();
        auto& healths = registry.get_components<Health>();

        for (const auto& state : entities) {
            auto it = networkEntities.find(state.entity_id);
            if (it == networkEntities.end())
                continue;

            Entity localEntity = it->second.entity;

            // Update position for ALL entities (including local player)
            // Server is authoritative - it decides where everything is
            if (positions.has_entity(localEntity)) {
                positions[localEntity].x = state.position_x;
                positions[localEntity].y = state.position_y;
            }

            // Update health
            if (healths.has_entity(localEntity)) {
                healths[localEntity].current = static_cast<int>(state.health);
            }
        }
    });

    client.set_on_game_over([&](const ServerGameOverPayload& result) {
        std::cout << "[Game] GAME OVER! " << (result.result == GameResult::VICTORY ? "VICTORY!" : "DEFEAT") << std::endl;
        // Could show game over screen here
    });

    client.set_on_disconnected([&]() {
        std::cout << "[Network] Disconnected from server" << std::endl;
        g_running = false;
    });

    // ====================
    // CONNECT TO SERVER
    // ====================
    std::cout << "\nConnecting to server..." << std::endl;
    if (!client.connect(server_host, server_port)) {
        std::cerr << "Failed to connect to server" << std::endl;
        graphicsPlugin->shutdown();
        return 1;
    }

    // Verify connection is still active
    if (!client.is_tcp_connected()) {
        std::cerr << "TCP connection lost immediately after connect" << std::endl;
        graphicsPlugin->shutdown();
        return 1;
    }

    std::cout << "TCP connected, sending connect request..." << std::endl;
    client.send_connect(player_name);

    // Wait for server response with timeout
    bool accepted = false;
    auto connect_start = std::chrono::steady_clock::now();
    const int CONNECT_TIMEOUT_MS = 5000;

    while (!accepted && g_running) {
        client.update();

        if (localPlayerId != 0) {
            accepted = true;
            break;
        }

        // Check for timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - connect_start).count();
        if (elapsed > CONNECT_TIMEOUT_MS) {
            std::cerr << "Connection timeout - server did not respond" << std::endl;
            break;
        }

        // Check if still connected
        if (!client.is_tcp_connected()) {
            std::cerr << "TCP connection lost while waiting for server accept" << std::endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!accepted) {
        std::cerr << "Failed to get server acceptance" << std::endl;
        graphicsPlugin->shutdown();
        return 1;
    }

    std::cout << "Server accepted connection! Player ID: " << localPlayerId << std::endl;

    // Join DUO lobby (2 players)
    std::cout << "Joining DUO lobby..." << std::endl;
    client.send_join_lobby(GameMode::DUO, Difficulty::NORMAL);

    // Lobby state for display
    uint8_t lobbyPlayerCount = 0;
    uint8_t lobbyRequiredCount = 2;
    uint8_t countdownSeconds = 0;

    // Update lobby callbacks to track state
    client.set_on_lobby_state([&](const ServerLobbyStatePayload& state) {
        lobbyPlayerCount = state.current_player_count;
        lobbyRequiredCount = state.required_player_count;
        std::cout << "[Lobby] " << (int)lobbyPlayerCount << "/" << (int)lobbyRequiredCount << " players" << std::endl;
    });

    client.set_on_countdown([&](uint8_t seconds) {
        countdownSeconds = seconds;
        std::cout << "[Lobby] Game starts in " << (int)seconds << "s..." << std::endl;
    });

    // ====================
    // LOBBY LOOP - Wait for players
    // ====================
    std::cout << "\n=== WAITING FOR ALLIED PLAYERS ===" << std::endl;

    float dotTimer = 0.0f;
    int dotCount = 0;

    while (graphicsPlugin->is_window_open() && g_running && !client.is_in_game() && client.is_tcp_connected()) {
        float dt = 1.0f / 60.0f;
        dotTimer += dt;
        if (dotTimer >= 0.5f) {
            dotTimer = 0.0f;
            dotCount = (dotCount + 1) % 4;
        }

        // Process network packets
        client.update();

        // Draw lobby screen
        graphicsPlugin->clear({20, 20, 30, 255});

        // Draw menu background
        if (menuBackgroundTex != 0) {
            engine::Sprite bgSprite;
            bgSprite.texture_handle = menuBackgroundTex;
            bgSprite.size = {static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT)};
            bgSprite.tint = engine::Color::White;
            graphicsPlugin->draw_sprite(bgSprite, {0, 0});
        }

        // Draw semi-transparent overlay for text readability
        graphicsPlugin->draw_rectangle(
            engine::Rectangle(0, static_cast<float>(SCREEN_HEIGHT) / 2 - 100,
                              static_cast<float>(SCREEN_WIDTH), 200.0f),
            {0, 0, 0, 180});

        // Draw waiting text
        std::string waitingText = "En attente des joueurs allies";
        std::string dots(dotCount, '.');
        float textX = (SCREEN_WIDTH / 2.0f) - 200.0f;
        float textY = (SCREEN_HEIGHT / 2.0f) - 40.0f;
        graphicsPlugin->draw_text(waitingText + dots, {textX, textY}, engine::Color::White, 0, 32);

        // Draw player count
        std::string countText = std::to_string(lobbyPlayerCount) + " / " + std::to_string(lobbyRequiredCount) + " joueurs";
        float countX = (SCREEN_WIDTH / 2.0f) - 80.0f;
        float countY = textY + 50.0f;
        graphicsPlugin->draw_text(countText, {countX, countY}, {100, 200, 255, 255}, 0, 28);

        // Draw countdown if active
        if (countdownSeconds > 0) {
            std::string countdownText = "Lancement dans " + std::to_string(countdownSeconds) + "...";
            float cdX = (SCREEN_WIDTH / 2.0f) - 120.0f;
            float cdY = countY + 50.0f;
            graphicsPlugin->draw_text(countdownText, {cdX, cdY}, {255, 200, 100, 255}, 0, 28);
        }

        graphicsPlugin->display();

        // Check for ESC key to quit
        if (inputPlugin->is_key_pressed(engine::Key::Escape)) {
            g_running = false;
        }
    }

    // ====================
    // GAME LOOP
    // ====================
    if (client.is_in_game()) {
        std::cout << "\n=== GAME STARTED ===" << std::endl;
        std::cout << "Controls: WASD/Arrows to move, SPACE to shoot, ESC to quit" << std::endl;
    }

    uint32_t tick = 0;
    auto lastInputTime = std::chrono::steady_clock::now();

    while (graphicsPlugin->is_window_open() && g_running && client.is_in_game() && client.is_tcp_connected()) {
        float dt = 1.0f / 60.0f;

        // Update input plugin state (IMPORTANT: must be called every frame!)
        inputPlugin->update();

        // Process network packets
        client.update();

        // Run local systems
        registry.run_systems(dt);

        // Send inputs
        if (localPlayerEntity != 0) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastInputTime).count();

            if (elapsed >= 16) { // ~60 Hz input rate
                uint16_t flags = 0;

                // Read input directly from plugin
                if (inputPlugin->is_key_pressed(engine::Key::W) || inputPlugin->is_key_pressed(engine::Key::Up))
                    flags |= INPUT_UP;
                if (inputPlugin->is_key_pressed(engine::Key::S) || inputPlugin->is_key_pressed(engine::Key::Down))
                    flags |= INPUT_DOWN;
                if (inputPlugin->is_key_pressed(engine::Key::A) || inputPlugin->is_key_pressed(engine::Key::Left))
                    flags |= INPUT_LEFT;
                if (inputPlugin->is_key_pressed(engine::Key::D) || inputPlugin->is_key_pressed(engine::Key::Right))
                    flags |= INPUT_RIGHT;
                if (inputPlugin->is_key_pressed(engine::Key::Space))
                    flags |= INPUT_SHOOT;

                // Log when there's actual input
                if (flags != 0) {
                    static int nonzero_input_count = 0;
                    nonzero_input_count++;
                    if (nonzero_input_count % 30 == 1) {  // Log every ~0.5s when pressing keys
                        std::cout << "[Client] Input flags=0x" << std::hex << flags << std::dec << "\n";
                    }
                }

                client.send_input(flags, tick++);
                lastInputTime = now;
            }
        }

        // Draw player name at bottom center
        float textWidth = player_name.length() * 12.0f;
        float textX = (SCREEN_WIDTH - textWidth) / 2.0f;
        float textY = SCREEN_HEIGHT - 50.0f;
        graphicsPlugin->draw_text(player_name, {textX, textY}, engine::Color::White, 0, 24);

        // Display
        graphicsPlugin->display();

        // Check for ESC key to quit
        if (inputPlugin->is_key_pressed(engine::Key::Escape)) {
            g_running = false;
        }
    }

    // ====================
    // CLEANUP
    // ====================
    std::cout << "Cleaning up..." << std::endl;

    // Disconnect gracefully (send_disconnect is already called inside disconnect())
    if (client.is_tcp_connected()) {
        client.disconnect();
    }

    std::cout << "Shutting down plugins..." << std::endl;
    inputPlugin->shutdown();
    graphicsPlugin->shutdown();
    if (audioPlugin) audioPlugin->shutdown();

    std::cout << "=== Client stopped ===" << std::endl;
    return 0;
}
