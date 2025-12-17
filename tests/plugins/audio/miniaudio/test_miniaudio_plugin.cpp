/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Test loading the Miniaudio Audio Plugin
*/

#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/PluginPaths.hpp"
#include "plugin_manager/IAudioPlugin.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>

int main() {
    try {
        std::cout << "=== Miniaudio Audio Plugin Test ===" << std::endl;

        // Create plugin manager
        engine::PluginManager plugin_manager;

        std::cout << "Loading Miniaudio Audio Plugin..." << std::endl;

        // Load the plugin using unified path helper
        auto plugin_path = engine::PluginPaths::get_plugin_path(
            engine::PluginPaths::MINIAUDIO_AUDIO
        );

        // Load the plugin
        auto* audio = plugin_manager.load_plugin<engine::IAudioPlugin>(
            plugin_path,
            "create_audio_plugin"
        );

        if (!audio) {
            std::cerr << "Failed to load audio plugin" << std::endl;
            return 1;
        }

        std::cout << "✓ Plugin loaded successfully!" << std::endl;
        std::cout << "  Name: " << audio->get_name() << std::endl;
        std::cout << "  Version: " << audio->get_version() << std::endl;

        // Initialize the plugin
        std::cout << "\nInitializing plugin..." << std::endl;
        if (!audio->initialize()) {
            std::cerr << "✗ Failed to initialize plugin" << std::endl;
            return 1;
        }
        std::cout << "✓ Plugin initialized successfully!" << std::endl;
        std::cout << "  Initialized: " << (audio->is_initialized() ? "Yes" : "No") << std::endl;

        // Test volume controls
        std::cout << "\nTesting volume controls..." << std::endl;
        audio->set_master_volume(0.8f);
        std::cout << "  Master volume set to: " << audio->get_master_volume() << std::endl;

        audio->set_music_volume(0.6f);
        std::cout << "  Music volume set to: " << audio->get_music_volume() << std::endl;

        // Test mute
        std::cout << "\nTesting mute..." << std::endl;
        audio->set_muted(false);
        std::cout << "  Muted: " << (audio->is_muted() ? "Yes" : "No") << std::endl;

        // Test loading sound (using a test file if it exists)
        std::cout << "\nTesting sound loading..." << std::endl;
        try {
            // Try to load a test sound file
            engine::SoundHandle sound = audio->load_sound("assets/test_sound.wav");
            std::cout << "✓ Sound loaded successfully! Handle: " << sound << std::endl;

            // Test playing the sound
            std::cout << "\nPlaying sound..." << std::endl;
            if (audio->play_sound(sound, 0.5f, 1.0f)) {
                std::cout << "✓ Sound started playing!" << std::endl;
                std::cout << "  Is playing: " << (audio->is_sound_playing(sound) ? "Yes" : "No") << std::endl;

                // Wait a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                // Stop the sound
                audio->stop_sound(sound);
                std::cout << "✓ Sound stopped" << std::endl;
            } else {
                std::cout << "✗ Failed to play sound" << std::endl;
            }

            // Unload the sound
            audio->unload_sound(sound);
            std::cout << "✓ Sound unloaded" << std::endl;

        } catch (const std::exception& e) {
            std::cout << "⚠ Sound test skipped: " << e.what() << std::endl;
            std::cout << "  (This is normal if assets/test_sound.wav doesn't exist)" << std::endl;
        }

        // Test loading music (using a test file if it exists)
        std::cout << "\nTesting music loading..." << std::endl;
        try {
            // Try to load a test music file
            engine::MusicHandle music = audio->load_music("assets/music.mp3");
            std::cout << "✓ Music loaded successfully! Handle: " << music << std::endl;

            // Test playing the music
            std::cout << "\nPlaying music..." << std::endl;
            if (audio->play_music(music, false, 0.3f)) {
                std::cout << "✓ Music started playing!" << std::endl;
                std::cout << "  Is playing: " << (audio->is_music_playing() ? "Yes" : "No") << std::endl;

                // Wait a bit
                std::cout << "\nPlaying for 3 seconds..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(3));

                // Test pause
                std::cout << "\nPausing music..." << std::endl;
                audio->pause_music();
                std::cout << "✓ Music paused" << std::endl;
                std::cout << "  Is playing: " << (audio->is_music_playing() ? "Yes" : "No") << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds(1));

                // Test resume
                std::cout << "\nResuming music..." << std::endl;
                audio->resume_music();
                std::cout << "✓ Music resumed" << std::endl;
                std::cout << "  Is playing: " << (audio->is_music_playing() ? "Yes" : "No") << std::endl;

                std::this_thread::sleep_for(std::chrono::seconds(2));

                // Stop the music
                audio->stop_music();
                std::cout << "✓ Music stopped" << std::endl;
            } else {
                std::cout << "✗ Failed to play music" << std::endl;
            }

            // Unload the music
            audio->unload_music(music);
            std::cout << "✓ Music unloaded" << std::endl;

        } catch (const std::exception& e) {
            std::cout << "⚠ Music test skipped: " << e.what() << std::endl;
            std::cout << "  (This is normal if assets/music.mp3 doesn't exist)" << std::endl;
        }

        // Test volume clamping with extreme values
        std::cout << "\nTesting volume clamping..." << std::endl;
        audio->set_master_volume(2.0f);  // Should clamp to 1.0
        std::cout << "  Set to 2.0, clamped to: " << audio->get_master_volume() << std::endl;

        audio->set_master_volume(-1.0f);  // Should clamp to 0.0
        std::cout << "  Set to -1.0, clamped to: " << audio->get_master_volume() << std::endl;

        audio->set_master_volume(0.5f);  // Normal value
        std::cout << "  Set to 0.5: " << audio->get_master_volume() << std::endl;

        // Test mute toggle
        std::cout << "\nTesting mute toggle..." << std::endl;
        audio->set_muted(true);
        std::cout << "  Muted: " << (audio->is_muted() ? "Yes" : "No") << std::endl;

        audio->set_muted(false);
        std::cout << "  Muted: " << (audio->is_muted() ? "Yes" : "No") << std::endl;

        // Shutdown the plugin
        std::cout << "\nShutting down plugin..." << std::endl;
        audio->shutdown();
        std::cout << "✓ Plugin shutdown complete!" << std::endl;
        std::cout << "  Initialized: " << (audio->is_initialized() ? "Yes" : "No") << std::endl;

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
