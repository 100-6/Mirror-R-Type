/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Simple test for the Miniaudio Audio Plugin (no pausing)
*/

#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/IAudioPlugin.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>

int main() {
    try {
        std::cout << "=== Simple Miniaudio Audio Plugin Test ===" << std::endl;

        // Create plugin manager
        engine::PluginManager plugin_manager;

        std::cout << "Loading Miniaudio Audio Plugin..." << std::endl;

        // Load the plugin
        auto* audio = plugin_manager.load_plugin<engine::IAudioPlugin>(
            "./plugins/miniaudio_audio.so",
            "create_audio_plugin"
        );

        if (!audio) {
            std::cerr << "Failed to load audio plugin" << std::endl;
            return 1;
        }

        std::cout << "✓ Plugin loaded successfully!" << std::endl;

        // Initialize the plugin
        std::cout << "Initializing plugin..." << std::endl;
        if (!audio->initialize()) {
            std::cerr << "✗ Failed to initialize plugin" << std::endl;
            return 1;
        }
        std::cout << "✓ Plugin initialized successfully!" << std::endl;

        // Set volume
        audio->set_master_volume(0.5f);
        audio->set_music_volume(0.5f);

        // Test loading music
        std::cout << "\nLoading music..." << std::endl;
        try {
            engine::MusicHandle music = audio->load_music("assets/music.mp3");
            std::cout << "✓ Music loaded successfully! Handle: " << music << std::endl;

            // Play the music
            std::cout << "\nPlaying music for 5 seconds..." << std::endl;
            if (audio->play_music(music, false, 0.5f)) {
                std::cout << "✓ Music started playing!" << std::endl;
                
                // Just let it play without pausing/resuming
                std::this_thread::sleep_for(std::chrono::seconds(5));

                // Stop the music
                std::cout << "\nStopping music..." << std::endl;
                audio->stop_music();
                std::cout << "✓ Music stopped" << std::endl;
            } else {
                std::cout << "✗ Failed to play music" << std::endl;
            }

            // Unload the music
            audio->unload_music(music);
            std::cout << "✓ Music unloaded" << std::endl;

        } catch (const std::exception& e) {
            std::cout << "⚠ Music test failed: " << e.what() << std::endl;
            return 1;
        }

        // Shutdown the plugin
        std::cout << "\nShutting down plugin..." << std::endl;
        audio->shutdown();
        std::cout << "✓ Plugin shutdown complete!" << std::endl;

        // Unload plugin
        std::cout << "Unloading plugin..." << std::endl;
        plugin_manager.unload_plugin("./plugins/miniaudio_audio.so");
        std::cout << "✓ Plugin unloaded successfully!" << std::endl;

        std::cout << "\n=== Test completed! ===" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
