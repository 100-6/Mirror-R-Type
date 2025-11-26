/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Test loading the Raylib Graphics Plugin
*/

#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include <iostream>
#include <stdexcept>

int main() {
    try {
        std::cout << "=== Raylib Graphics Plugin Test ===" << std::endl;
        
        // Create plugin manager
        engine::PluginManager plugin_manager;
        
        std::cout << "Loading Raylib Graphics Plugin..." << std::endl;
        
        // Load the plugin (Windows: .dll, Linux/Mac: .so)
#ifdef _WIN32
        const char* plugin_path = "raylib_graphics.dll";
#else
        const char* plugin_path = "libraylib_graphics.so";
#endif
        
        auto* graphics = plugin_manager.load_plugin<engine::IGraphicsPlugin>(
            plugin_path,
            "create_graphics_plugin"
        );
        
        if (!graphics) {
            std::cerr << "Failed to load graphics plugin from: " << plugin_path << std::endl;
            return 1;
        }
        
        std::cout << "✓ Plugin loaded successfully!" << std::endl;
        std::cout << "  Name: " << graphics->get_name() << std::endl;
        std::cout << "  Version: " << graphics->get_version() << std::endl;
        std::cout << "  Initialized: " << (graphics->is_initialized() ? "Yes" : "No") << std::endl;
        
        // Test window creation
        std::cout << "\nTesting window creation..." << std::endl;
        if (graphics->create_window(800, 600, "Raylib Plugin Test")) {
            std::cout << "✓ Window created successfully!" << std::endl;
            std::cout << "  Window open: " << (graphics->is_window_open() ? "Yes" : "No") << std::endl;
            
            // Test basic rendering
            std::cout << "\nTesting basic rendering..." << std::endl;
            graphics->clear(engine::Color::Black);
            
            // Draw a simple rectangle
            engine::Rectangle rect(100.0f, 100.0f, 200.0f, 150.0f);
            graphics->draw_rectangle(rect, engine::Color::Red);
            
            // Draw text
            graphics->draw_text(
                "Raylib Plugin Works!",
                engine::Vector2f(200.0f, 250.0f),
                engine::Color::White,
                engine::INVALID_HANDLE,
                30
            );
            
            // Draw a circle
            graphics->draw_circle(engine::Vector2f(400.0f, 400.0f), 50.0f, engine::Color::Blue);
            
            graphics->display();
            
            std::cout << "✓ Rendering test completed!" << std::endl;
            std::cout << "\nPress Enter to close the window..." << std::endl;
            std::cin.get();
            
            graphics->close_window();
        } else {
            std::cerr << "✗ Failed to create window" << std::endl;
            return 1;
        }
        
        // Unload plugin
        std::cout << "\nUnloading plugin..." << std::endl;
        plugin_manager.unload_plugin(plugin_path);
        std::cout << "✓ Plugin unloaded successfully!" << std::endl;
        
        std::cout << "\n=== All tests passed! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

