/*
** Bagario Client
** Agar.io-like game using the ECS engine
*/

#include "BagarioGame.hpp"
#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/PluginPaths.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include <iostream>
#include <cstring>

void print_help(const char* program_name) {
    std::cout << "=== Bagario Client ===\n\n";
    std::cout << "USAGE:\n";
    std::cout << "  " << program_name << " [OPTIONS]\n\n";
    std::cout << "OPTIONS:\n";
    std::cout << "  -h, --help    Show this help message and exit\n\n";
    std::cout << "CONTROLS:\n";
    std::cout << "  Mouse         Move your cell towards cursor\n";
    std::cout << "  Space         Split your cell\n";
    std::cout << "  W             Eject mass\n";
    std::cout << "  ESC           Quit game\n\n";
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

    std::cout << "=== Bagario Client ===\n";
    std::cout << "Press ESC to exit\n\n";

    // Initialize plugin manager
    engine::PluginManager plugin_manager;

    // Load graphics plugin
    engine::IGraphicsPlugin* graphics = nullptr;
    try {
        graphics = plugin_manager.load_plugin<engine::IGraphicsPlugin>(
            engine::PluginPaths::get_plugin_path(engine::PluginPaths::SFML_GRAPHICS),
            "create_graphics_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "[Bagario] Failed to load graphics plugin: " << e.what() << "\n";
        return 1;
    }

    if (!graphics) {
        std::cerr << "[Bagario] Graphics plugin is null after loading!\n";
        return 1;
    }

    // Create window
    if (!graphics->create_window(SCREEN_WIDTH, SCREEN_HEIGHT, "Bagario - Eat or be eaten!")) {
        std::cerr << "[Bagario] Failed to create window!\n";
        return 1;
    }
    graphics->set_vsync(false);  // VSync off by default for lower latency, can be enabled in Settings

    // Load input plugin separately
    engine::IInputPlugin* input = nullptr;
    try {
        input = plugin_manager.load_plugin<engine::IInputPlugin>(
            engine::PluginPaths::get_plugin_path(engine::PluginPaths::SFML_INPUT),
            "create_input_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "[Bagario] Failed to load input plugin: " << e.what() << "\n";
        graphics->close_window();
        return 1;
    }

    if (!input) {
        std::cerr << "[Bagario] Input plugin is null!\n";
        graphics->close_window();
        return 1;
    }

    // Pass window handle to input plugin (critical for SFML to handle relative coords)
    input->set_window_handle(graphics->get_window_handle());

    // Create and run game
    bagario::BagarioGame game(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!game.initialize(graphics, input)) {
        std::cerr << "[Bagario] Failed to initialize game!\n";
        graphics->close_window();
        return 1;
    }

    game.run();
    game.shutdown();

    // Cleanup
    graphics->close_window();
    plugin_manager.unload_all();

    std::cout << "[Bagario] Goodbye!\n";
    return 0;
}
