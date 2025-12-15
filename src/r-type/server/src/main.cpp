/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Main Server - R-Type Game Server
*/

#include "ecs/Registry.hpp"
#include "plugin_manager/PluginManager.hpp"
#include "systems/ServerNetworkSystem.hpp"
#include "ecs/systems/NetworkSystem.hpp"
#include "ecs/systems/PhysiqueSystem.hpp"
#include "systems/ShootingSystem.hpp"
#include "systems/CollisionSystem.hpp"
#include "ecs/CoreComponents.hpp"
#include "components/GameComponents.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

static std::atomic<bool> running{true};

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n[Server] Shutdown signal received..." << std::endl;
        running = false;
    }
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << "\nOptions:" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << "  -p, --port PORT         Server port (default: 4242)" << std::endl;
    std::cout << "  -t, --tickrate RATE     Server tick rate in Hz (default: 60)" << std::endl;
    std::cout << "  -m, --max-players NUM   Maximum players (default: 4)" << std::endl;
    std::cout << "\nExamples:" << std::endl;
    std::cout << "  " << program_name << " -p 8080" << std::endl;
    std::cout << "  " << program_name << " --port 4242 --tickrate 30" << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    uint16_t port = 4242;
    float tickrate = 60.0f;
    uint8_t max_players = 4;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            std::cout << "=== R-Type Server ===" << std::endl;
            std::cout << std::endl;
            print_usage(argv[0]);
            return 0;
        } else if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            try {
                port = std::stoi(argv[++i]);
            } catch (...) {
                std::cerr << "Invalid port number" << std::endl;
                return 1;
            }
        } else if ((arg == "-t" || arg == "--tickrate") && i + 1 < argc) {
            try {
                tickrate = std::stof(argv[++i]);
                if (tickrate <= 0 || tickrate > 120) {
                    std::cerr << "Tick rate must be between 1 and 120 Hz" << std::endl;
                    return 1;
                }
            } catch (...) {
                std::cerr << "Invalid tick rate" << std::endl;
                return 1;
            }
        } else if ((arg == "-m" || arg == "--max-players") && i + 1 < argc) {
            try {
                max_players = std::stoi(argv[++i]);
                if (max_players < 1 || max_players > 8) {
                    std::cerr << "Max players must be between 1 and 8" << std::endl;
                    return 1;
                }
            } catch (...) {
                std::cerr << "Invalid max players" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            std::cerr << "Use -h or --help for usage information" << std::endl;
            return 1;
        }
    }

    std::cout << "=== R-Type Server ===" << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Tick rate: " << tickrate << " Hz" << std::endl;
    std::cout << "Max players: " << static_cast<int>(max_players) << std::endl;
    std::cout << std::endl;

    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        // Create plugin manager and load network plugin
        engine::PluginManager pluginManager;
        auto* networkPlugin = pluginManager.load_plugin<engine::INetworkPlugin>(
            "plugins/asio_network.so",
            "create_network_plugin"
        );

        if (!networkPlugin) {
            std::cerr << "[Server] Failed to load network plugin!" << std::endl;
            return 1;
        }

        std::cout << "[Server] Initializing on port " << port << "..." << std::endl;

        // Create registry and systems
        Registry registry;

        // Register components
        registry.register_component<Position>();
        registry.register_component<Velocity>();
        registry.register_component<Controllable>();
        registry.register_component<Collider>();
        registry.register_component<NoFriction>();
        registry.register_component<Sprite>();
        registry.register_component<Projectile>();
        registry.register_component<Damage>();
        registry.register_component<Health>();
        registry.register_component<ToDestroy>();
        registry.register_component<Enemy>();
        registry.register_component<Weapon>();
        registry.register_component<Input>();  // For player input events
        registry.register_component<Invulnerability>();

        // Register systems
        // registry.register_system<NetworkSystem>(*networkPlugin, true, port);  // DISABLED - ServerNetworkSystem handles networking
        registry.register_system<rtype::server::ServerNetworkSystem>(*networkPlugin, max_players);
        // registry.register_system<ShootingSystem>();     // Handle shooting - DISABLED for debugging
        registry.register_system<PhysiqueSystem>();     // Apply velocity to position
        // registry.register_system<CollisionSystem>();    // Handle collisions - DISABLED for debugging

        std::cout << "[Server] Initialized. Waiting for clients..." << std::endl;
        std::cout << "[Server] Systems:" << std::endl;
        std::cout << "  1. ServerNetworkSystem - R-Type protocol handler + network I/O" << std::endl;
        std::cout << "  2. PhysiqueSystem      - Apply velocity to position" << std::endl;
        std::cout << std::endl;

        // Main server loop
        const float dt = 1.0f / tickrate;

        while (running) {
            // Update all systems
            registry.run_systems(dt);

            // Sleep to maintain target tick rate
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(dt * 1000)));
        }

        std::cout << "[Server] Shutting down..." << std::endl;
        // NetworkSystem will handle stopping the server in its shutdown()

    } catch (const std::exception& e) {
        std::cerr << "[Server] Fatal error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "[Server] Goodbye!" << std::endl;
    return 0;
}
