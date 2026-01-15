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
#include <cstring>

#include "ecs/systems/AudioSystem.hpp"
#include "systems/HealthSystem.hpp"
#include "systems/HitEffectSystem.hpp"
#include "systems/AISystem.hpp"

#include "systems/BonusSystem.hpp"
#include "systems/HUDSystem.hpp"
#include "systems/GameStateSystem.hpp"
#include "systems/AttachmentSystem.hpp"
#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/PluginPaths.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "plugin_manager/IAudioPlugin.hpp"

void print_help(const char* program_name) {
    std::cout << "=== R-Type Client ===\n\n";
    std::cout << "USAGE:\n";
    std::cout << "  " << program_name << " [OPTIONS] [HOST] [PORT] [PLAYER_NAME]\n\n";
    std::cout << "OPTIONS:\n";
    std::cout << "  -h, --help              Show this help message and exit\n\n";
    std::cout << "ARGUMENTS:\n";
    std::cout << "  HOST                    Server IP address or hostname\n";
    std::cout << "                          Default: 127.0.0.1\n\n";
    std::cout << "  PORT                    Server TCP port number\n";
    std::cout << "                          Default: " << rtype::protocol::config::DEFAULT_TCP_PORT << "\n\n";
    std::cout << "  PLAYER_NAME             Your player name (displayed in lobby)\n";
    std::cout << "                          Default: Pilot\n\n";
    std::cout << "EXAMPLES:\n";
    std::cout << "  " << program_name << "\n";
    std::cout << "      Connect to localhost:" << rtype::protocol::config::DEFAULT_TCP_PORT << " as 'Pilot'\n\n";
    std::cout << "  " << program_name << " 192.168.1.100\n";
    std::cout << "      Connect to 192.168.1.100:" << rtype::protocol::config::DEFAULT_TCP_PORT << " as 'Pilot'\n\n";
    std::cout << "  " << program_name << " 192.168.1.100 4242 Alice\n";
    std::cout << "      Connect to 192.168.1.100:4242 as 'Alice'\n\n";
    std::cout << "CONTROLS:\n";
    std::cout << "  Arrow Keys              Move spaceship\n";
    std::cout << "  Space / Left Click      Fire weapon\n";
    std::cout << "  ESC                     Quit game\n\n";
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
    }

    const int SCREEN_WIDTH = 1920;
    const int SCREEN_HEIGHT = 1080;
    std::string host = "127.0.0.1";
    uint16_t tcp_port = rtype::protocol::config::DEFAULT_TCP_PORT;
    std::string player_name = "Pilot";

    if (argc > 1)
        host = argv[1];
    if (argc > 2) {
        try {
            tcp_port = static_cast<uint16_t>(std::stoi(argv[2]));
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid TCP port: " << e.what() << '\n';
            std::cerr << "Use --help for usage information\n";
            return 1;
        }
    }
    if (argc > 3)
        player_name = argv[3];

    std::cout << "=== R-Type Network Client ===\n";
    std::cout << "Server: " << host << ":" << tcp_port << "\n";
    std::cout << "Player: " << player_name << "\n\n";

    rtype::client::ClientGame game(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!game.initialize(host, tcp_port, player_name)) {
        std::cerr << "Failed to initialize game\n";
        std::cerr << "Use --help for usage information\n";
        return 1;
    }
    game.run();
    game.shutdown();
    return 0;
}
