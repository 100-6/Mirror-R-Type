/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Network Test Client - Tests TCP/UDP hybrid architecture
**
** ============================================================
** THIS FILE IS FOR TESTING PURPOSES ONLY
** ============================================================
** Use this to validate the TCP/UDP networking implementation
** before integrating into the main game client.
**
** Usage: ./r-type_test_client [host] [port]
**   Default: localhost:DEFAULT_TCP_PORT
**
** Test flow:
**   1. Connect to server via TCP
**   2. Send CLIENT_CONNECT packet
**   3. Receive SERVER_ACCEPT
**   4. Send CLIENT_JOIN_LOBBY
**   5. Wait for GAME_START (with UDP port)
**   6. Connect UDP and send handshake
**   7. Send test inputs via UDP
** ============================================================
*/

#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

#include "NetworkClient.hpp"
#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/INetworkPlugin.hpp"
#include "plugin_manager/PluginPaths.hpp"
#include "protocol/Payloads.hpp"
#include "protocol/PacketTypes.hpp"
#include "protocol/NetworkConfig.hpp"

// Use the correct namespace
using namespace rtype::protocol;

static std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    std::cout << "\n[TestClient] Received signal " << signal << ", stopping...\n";
    g_running = false;
}

int main(int argc, char* argv[]) {
    std::string host = "localhost";
    uint16_t port = config::DEFAULT_TCP_PORT;

    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        try {
            port = static_cast<uint16_t>(std::stoi(argv[2]));
        } catch (...) {
            std::cerr << "Invalid port: " << argv[2] << "\n";
            return 1;
        }
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cout << "=== R-Type Network Test Client ===\n";
    std::cout << "Connecting to " << host << ":" << port << "\n\n";

    // ============================================================
    // TEST: Load network plugin
    // ============================================================
    engine::PluginManager plugin_manager;
    auto* network_plugin = plugin_manager.load_plugin<engine::INetworkPlugin>(
        engine::PluginPaths::get_plugin_path(engine::PluginPaths::ASIO_NETWORK),
        "create_network_plugin");

    if (!network_plugin) {
        std::cerr << "[TestClient] Failed to load network plugin\n";
        return 1;
    }
    std::cout << "[TestClient] Loaded: " << network_plugin->get_name() << "\n";

    if (!network_plugin->initialize()) {
        std::cerr << "[TestClient] Failed to initialize network plugin\n";
        return 1;
    }

    // ============================================================
    // TEST: Create NetworkClient and setup callbacks
    // ============================================================
    rtype::client::NetworkClient client(*network_plugin);

    client.set_on_accepted([](uint32_t player_id) {
        std::cout << "[TestClient] CONNECTION ACCEPTED! Player ID: " << player_id << "\n";
    });

    client.set_on_rejected([](uint8_t reason, const std::string& message) {
        std::cout << "[TestClient] CONNECTION REJECTED! Reason: " << (int)reason
                  << " - " << message << "\n";
    });

    client.set_on_lobby_state([](const ServerLobbyStatePayload& state, const std::vector<PlayerLobbyEntry>& players) {
        std::cout << "[TestClient] LOBBY STATE: "
                  << (int)state.current_player_count << "/" << (int)state.required_player_count
                  << " players\n";
        for (const auto& entry : players) {
            std::cout << "    - Player " << entry.player_id << " (" << entry.player_name << ")\n";
        }
    });

    client.set_on_countdown([](uint8_t seconds) {
        std::cout << "[TestClient] COUNTDOWN: " << (int)seconds << " seconds\n";
    });

    client.set_on_game_start([&client](uint32_t session_id, uint16_t udp_port, uint16_t map_id, float scroll_speed, uint32_t seed) {
        std::cout << "[TestClient] GAME START! Session: " << session_id
                  << ", UDP port: " << udp_port << ", Map: " << map_id
                  << ", Scroll speed: " << scroll_speed 
                  << ", Seed: " << seed << "\n";
        std::cout << "[TestClient] UDP connection should be automatic...\n";
    });

    client.set_on_entity_spawn([](const ServerEntitySpawnPayload& spawn) {
        std::cout << "[TestClient] ENTITY SPAWN: ID=" << spawn.entity_id
                  << " Type=" << (int)spawn.entity_type
                  << " Pos=(" << spawn.spawn_x << "," << spawn.spawn_y << ")\n";
    });

    client.set_on_entity_destroy([](const ServerEntityDestroyPayload& destroy) {
        std::cout << "[TestClient] ENTITY DESTROY: ID=" << destroy.entity_id << "\n";
    });

    client.set_on_game_over([](const ServerGameOverPayload& result) {
        std::cout << "[TestClient] GAME OVER! Result: " << (result.result == GameResult::VICTORY ? "VICTORY" : "DEFEAT")
                  << " Time: " << result.total_time << "s, Enemies killed: " << result.enemies_killed << "\n";
    });

    client.set_on_disconnected([]() {
        std::cout << "[TestClient] DISCONNECTED from server\n";
        g_running = false;
    });

    // ============================================================
    // TEST: Connect to server via TCP
    // ============================================================
    std::cout << "\n[TestClient] Connecting to server...\n";
    if (!client.connect(host, port)) {
        std::cerr << "[TestClient] Failed to connect to server\n";
        return 1;
    }
    std::cout << "[TestClient] TCP connected!\n";

    // ============================================================
    // TEST: Send CLIENT_CONNECT packet
    // ============================================================
    std::cout << "[TestClient] Sending connection request...\n";
    client.send_connect("TestPlayer");

    // Wait a bit for response
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    client.update();

    if (client.get_player_id() == 0) {
        std::cerr << "[TestClient] No player ID received, connection may have failed\n";
    }

    // ============================================================
    // TEST: Join lobby
    // ============================================================
    std::cout << "[TestClient] Joining lobby (Classic/Normal)...\n";
    client.send_join_lobby(GameMode::SQUAD, Difficulty::NORMAL);

    // ============================================================
    // TEST: Main loop - process packets and send test inputs
    // ============================================================
    std::cout << "\n[TestClient] Entering main loop (Ctrl+C to exit)...\n";
    std::cout << "[TestClient] Waiting for other players or game start...\n\n";

    uint32_t tick = 0;
    auto last_input_time = std::chrono::steady_clock::now();
    auto last_ping_time = std::chrono::steady_clock::now();

    while (g_running) {
        // Process incoming packets
        client.update();

        auto now = std::chrono::steady_clock::now();

        // Send ping every 5 seconds
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_ping_time).count() >= 5) {
            client.send_ping();
            last_ping_time = now;
        }

        // If in game, send test inputs every 50ms (20 Hz)
        if (client.is_in_game()) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_input_time).count() >= 50) {
                // Simulate some inputs (moving right and shooting)
                uint16_t test_inputs = 0;
                // Bit 0 = UP, Bit 1 = DOWN, Bit 2 = LEFT, Bit 3 = RIGHT, Bit 4 = SHOOT
                test_inputs |= (1 << 3);  // RIGHT
                if (tick % 10 == 0) {
                    test_inputs |= (1 << 4);  // SHOOT every 10 ticks
                }

                client.send_input(test_inputs, tick++);
                last_input_time = now;
            }
        }

        // Small sleep to avoid busy loop
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // ============================================================
    // TEST: Cleanup
    // ============================================================
    std::cout << "\n[TestClient] Disconnecting...\n";
    client.send_disconnect();
    client.disconnect();

    std::cout << "[TestClient] Test complete!\n";
    return 0;
}
