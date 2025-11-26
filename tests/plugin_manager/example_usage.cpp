/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Example usage of the PluginManager
*/

#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/INetworkPlugin.hpp"
#include "plugin_manager/IAudioPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include <iostream>

using namespace engine;

/**
 * This example demonstrates how to use the PluginManager to load
 * different types of plugins dynamically.
 * 
 * Usage:
 * 1. Build your plugins as .so files
 * 2. Load them using the PluginManager
 * 3. Use the plugin interfaces to interact with them
 * 4. The plugins are automatically unloaded when PluginManager is destroyed
 */

int main() {
    try {
        // Create the plugin manager
        PluginManager plugin_manager;

        std::cout << "=== Plugin Manager Example ===" << std::endl;
        std::cout << std::endl;

        // Example 1: Load a graphics plugin
        std::cout << "Loading graphics plugin..." << std::endl;
        try {
            auto* graphics = plugin_manager.load_plugin<IGraphicsPlugin>(
                "./plugins/libsfml_graphics.so",
                "create_graphics_plugin"
            );

            std::cout << "  ✓ Loaded: " << graphics->get_name() 
                      << " v" << graphics->get_version() << std::endl;

            // Use the graphics plugin
            if (graphics->create_window(800, 600, "R-Type")) {
                std::cout << "  ✓ Window created successfully" << std::endl;
                
                // Game loop example
                // while (graphics->is_window_open()) {
                //     graphics->clear(Color::Black);
                //     // ... render game ...
                //     graphics->display();
                // }
            }
        } catch (const PluginException& e) {
            std::cerr << "  ✗ Failed to load graphics plugin: " << e.what() << std::endl;
        }

        std::cout << std::endl;

        // Example 2: Load a network plugin
        std::cout << "Loading network plugin..." << std::endl;
        try {
            auto* network = plugin_manager.load_plugin<INetworkPlugin>(
                "./plugins/libasio_network.so",
                "create_network_plugin"
            );

            std::cout << "  ✓ Loaded: " << network->get_name() 
                      << " v" << network->get_version() << std::endl;

            // Start a server
            if (network->start_server(8080)) {
                std::cout << "  ✓ Server started on port 8080" << std::endl;
            }

            // Or connect as a client
            // if (network->connect("127.0.0.1", 8080)) {
            //     std::cout << "  ✓ Connected to server" << std::endl;
            // }
        } catch (const PluginException& e) {
            std::cerr << "  ✗ Failed to load network plugin: " << e.what() << std::endl;
        }

        std::cout << std::endl;

        // Example 3: Load an audio plugin
        std::cout << "Loading audio plugin..." << std::endl;
        try {
            auto* audio = plugin_manager.load_plugin<IAudioPlugin>(
                "./plugins/libsfml_audio.so",
                "create_audio_plugin"
            );

            std::cout << "  ✓ Loaded: " << audio->get_name() 
                      << " v" << audio->get_version() << std::endl;

            // Load and play a sound
            // auto sound = audio->load_sound("./assets/sounds/explosion.wav");
            // audio->play_sound(sound);
        } catch (const PluginException& e) {
            std::cerr << "  ✗ Failed to load audio plugin: " << e.what() << std::endl;
        }

        std::cout << std::endl;

        // Example 4: Query plugin manager state
        std::cout << "Plugin Manager Status:" << std::endl;
        std::cout << "  Loaded plugins: " << plugin_manager.get_plugin_count() << std::endl;
        
        if (plugin_manager.is_plugin_loaded("./plugins/libsfml_graphics.so")) {
            std::cout << "  ✓ Graphics plugin is loaded" << std::endl;
        }

        std::cout << std::endl;

        // Example 5: Get a previously loaded plugin
        std::cout << "Retrieving loaded plugin..." << std::endl;
        auto* graphics = plugin_manager.get_plugin<IGraphicsPlugin>("./plugins/libsfml_graphics.so");
        if (graphics) {
            std::cout << "  ✓ Retrieved graphics plugin: " << graphics->get_name() << std::endl;
        } else {
            std::cout << "  ✗ Graphics plugin not found" << std::endl;
        }

        std::cout << std::endl;

        // Example 6: Unload a specific plugin
        std::cout << "Unloading network plugin..." << std::endl;
        try {
            plugin_manager.unload_plugin("./plugins/libasio_network.so");
            std::cout << "  ✓ Network plugin unloaded" << std::endl;
        } catch (const PluginException& e) {
            std::cerr << "  ✗ Failed to unload: " << e.what() << std::endl;
        }

        std::cout << std::endl;

        // Example 7: Demonstrate error handling
        std::cout << "Testing error handling..." << std::endl;
        try {
            // Try to load a non-existent plugin
            plugin_manager.load_plugin<IGraphicsPlugin>(
                "./plugins/non_existent.so",
                "create_graphics_plugin"
            );
        } catch (const PluginException& e) {
            std::cout << "  ✓ Caught expected error: " << e.what() << std::endl;
        }

        std::cout << std::endl;

        // All remaining plugins will be automatically unloaded
        // when plugin_manager goes out of scope
        std::cout << "Cleaning up..." << std::endl;
        plugin_manager.unload_all();
        std::cout << "  ✓ All plugins unloaded" << std::endl;

        std::cout << std::endl;
        std::cout << "=== Example Complete ===" << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
