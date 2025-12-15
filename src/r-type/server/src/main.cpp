/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** R-Type Server Entry Point
*/

#include "Server.hpp"
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
    uint16_t port = 4242; // Default port
    if (argc > 1) {
        try {
            port = static_cast<uint16_t>(std::stoi(argv[1]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid port number: " << argv[1] << "\n";
            std::cerr << "Usage: " << argv[0] << " [port]\n";
            return 1;
        }
    }

    // Register signal handlers for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Create and start server
    std::cout << "=== R-Type Server ===\n";
    std::cout << "Protocol Version: 1.0\n";
    std::cout << "Transport: UDP\n\n";

    g_server = std::make_unique<rtype::server::Server>(port);

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
