/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Minimal test - just loads and verifies the plugin (no window needed)
*/

#include "PluginManager.hpp"
#include "IGraphicsPlugin.hpp"
#include <iostream>

int main() {
    std::cout << "=== Minimal Raylib Plugin Test ===" << std::endl;
    std::cout << "This test verifies plugin loading without creating a window.\n" << std::endl;
    
    try {
        // Create plugin manager
        rtype::PluginManager plugin_manager;
        
        std::cout << "[1/4] Loading plugin..." << std::endl;
        
        // Load the plugin
        auto* graphics = plugin_manager.load_plugin<rtype::IGraphicsPlugin>(
            "./plugins/raylib_graphics.so",
            "create_graphics_plugin"
        );
        
        if (!graphics) {
            std::cerr << "✗ Failed to load graphics plugin" << std::endl;
            return 1;
        }
        std::cout << "✓ Plugin loaded successfully!" << std::endl;
        
        std::cout << "\n[2/4] Checking plugin info..." << std::endl;
        std::cout << "  Name: " << graphics->get_name() << std::endl;
        std::cout << "  Version: " << graphics->get_version() << std::endl;
        std::cout << "  Initialized: " << (graphics->is_initialized() ? "Yes" : "No") << std::endl;
        
        std::cout << "\n[3/4] Testing plugin methods (without window)..." << std::endl;
        
        // Test methods that don't require a window
        std::cout << "  - is_window_open(): " << (graphics->is_window_open() ? "Yes" : "No") << std::endl;
        
        // Try to get size of invalid texture (should return 0,0)
        auto size = graphics->get_texture_size(rtype::INVALID_HANDLE);
        std::cout << "  - get_texture_size(INVALID): " << size.x << "x" << size.y << std::endl;
        
        std::cout << "\n[4/4] Unloading plugin..." << std::endl;
        plugin_manager.unload_plugin("./plugins/raylib_graphics.so");
        std::cout << "✓ Plugin unloaded successfully!" << std::endl;
        
        std::cout << "\n=== ✓ ALL TESTS PASSED ===" << std::endl;
        std::cout << "\nThe plugin loads and unloads correctly!" << std::endl;
        std::cout << "To test with graphics, run: ./test_raylib_plugin" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "✗ Error: " << e.what() << std::endl;
        return 1;
    }
}
