/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** R-Type Server Entry Point
*/

#include "Server.hpp"
#include "ServerConfig.hpp"
#include <iostream>
#include <csignal>
#include <memory>

// Global server instance for signal handling
static std::unique_ptr<rtype::server::Server> g_server;

/**
 * @brief Signal handler for graceful shutdown (SIGINT, SIGTERM)
 */
void signal_handler(int signal) {
    std::cout << "\n[Server] Received signal " << signal << ", shutting down gracefully...\n";
    if (g_server) {
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    uint16_t tcp_port = rtype::server::config::DEFAULT_TCP_PORT;
    uint16_t udp_port = rtype::server::config::DEFAULT_UDP_PORT;
    bool listen_on_all_interfaces = false;

    if (argc > 1) {
        try {
            tcp_port = static_cast<uint16_t>(std::stoi(argv[1]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid TCP port number: " << argv[1] << "\n";
            std::cerr << "Usage: " << argv[0] << " [tcp_port] [udp_port] [--network|-n]\n";
            std::cerr << "  --network, -n : Listen on all network interfaces (0.0.0.0) instead of localhost\n";
            return 1;
        }
    }
    if (argc > 2) {
        try {
            udp_port = static_cast<uint16_t>(std::stoi(argv[2]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid UDP port number: " << argv[2] << "\n";
            std::cerr << "Usage: " << argv[0] << " [tcp_port] [udp_port] [--network|-n]\n";
            std::cerr << "  --network, -n : Listen on all network interfaces (0.0.0.0) instead of localhost\n";
            return 1;
        }
    }
    // Check for network flag
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--network" || arg == "-n") {
            listen_on_all_interfaces = true;
            break;
        }
    }

    // Register signal handlers for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Create and start server
    std::cout << "=== R-Type Server ===\n";
    std::cout << "Protocol Version: 1.0\n";
    std::cout << "Transport: Hybrid TCP/UDP\n";
    std::cout << "Network: " << (listen_on_all_interfaces ? "All interfaces (0.0.0.0)" : "Localhost only (127.0.0.1)") << "\n\n";

    g_server = std::make_unique<rtype::server::Server>(tcp_port, udp_port, listen_on_all_interfaces);

    if (!g_server->start()) {
        std::cerr << "[Server] Failed to start server\n";
        return 1;
    }

    // Run server loop
    g_server->run();

    // Cleanup
    g_server.reset();
    std::cout << "[Server] Shutdown complete\n";

    return 0;
}
