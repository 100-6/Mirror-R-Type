/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Multiplayer Network Client entry point
*/

#include "ClientGame.hpp"
#include "protocol/NetworkConfig.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    const int SCREEN_WIDTH = 1920;
    const int SCREEN_HEIGHT = 1080;

    // Parse command line arguments
    std::string host = "127.0.0.1";
    uint16_t tcp_port = rtype::protocol::config::DEFAULT_TCP_PORT;
    std::string player_name = "Pilot";

    if (argc > 1)
        host = argv[1];

    if (argc > 2) {
        try {
            tcp_port = static_cast<uint16_t>(std::stoi(argv[2]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid TCP port: " << e.what() << '\n';
            return 1;
        }
    }

    if (argc > 3)
        player_name = argv[3];

    // Create and initialize game
    rtype::client::ClientGame game(SCREEN_WIDTH, SCREEN_HEIGHT);

    if (!game.initialize(host, tcp_port, player_name)) {
        std::cerr << "Failed to initialize game\n";
        return 1;
    }

    // Run game loop
    game.run();

    // Cleanup
    game.shutdown();

    return 0;
}
