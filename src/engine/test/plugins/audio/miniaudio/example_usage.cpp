/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Example usage of Miniaudio Audio Plugin
*/

#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/IAudioPlugin.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    try {
        // Create plugin manager
        rtype::PluginManager manager;

        std::cout << "Loading Miniaudio Audio Plugin..." << std::endl;

        // Load the audio plugin
        auto* audio = manager.load_plugin<rtype::IAudioPlugin>(
            "./plugins/miniaudio_audio.so",
            "create_audio_plugin"
        );

        if (!audio) {
            std::cerr << "Failed to load audio plugin" << std::endl;
            return 1;
        }

        std::cout << "Plugin: " << audio->get_name()
                  << " v" << audio->get_version() << std::endl;

        // Initialize the plugin
        if (!audio->initialize()) {
            std::cerr << "Failed to initialize audio plugin" << std::endl;
            return 1;
        }

        std::cout << "Audio plugin initialized!" << std::endl;

        // Set master volume
        audio->set_master_volume(0.8f);

        // Example 1: Play a sound effect
        std::cout << "\n--- Playing sound effect ---" << std::endl;
        try {
            auto sound = audio->load_sound("assets/shoot.wav");

            // Play at 80% volume, normal pitch
            audio->play_sound(sound, 0.8f, 1.0f);

            std::cout << "Playing shoot sound..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // Play again at higher pitch
            audio->play_sound(sound, 0.8f, 1.5f);

            std::cout << "Playing shoot sound at 1.5x pitch..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));

            audio->unload_sound(sound);
        } catch (const std::exception& e) {
            std::cout << "Sound example skipped: " << e.what() << std::endl;
        }

        // Example 2: Play background music
        std::cout << "\n--- Playing background music ---" << std::endl;
        try {
            auto music = audio->load_music("assets/background_music.mp3");

            // Play with looping at 50% volume
            if (audio->play_music(music, true, 0.5f)) {
                std::cout << "Music playing..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));

                // Fade out by reducing volume
                std::cout << "Fading out..." << std::endl;
                for (int i = 5; i >= 0; i--) {
                    audio->set_music_volume(i * 0.1f);
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }

                audio->stop_music();
                std::cout << "Music stopped." << std::endl;
            }

            audio->unload_music(music);
        } catch (const std::exception& e) {
            std::cout << "Music example skipped: " << e.what() << std::endl;
        }

        // Example 3: Pause and resume
        std::cout << "\n--- Testing pause/resume ---" << std::endl;
        try {
            auto music = audio->load_music("assets/music.mp3");

            if (audio->play_music(music, false, 0.6f)) {
                std::cout << "Music playing..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));

                audio->pause_music();
                std::cout << "Music paused" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));

                audio->resume_music();
                std::cout << "Music resumed" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(2));

                audio->stop_music();
            }

            audio->unload_music(music);
        } catch (const std::exception& e) {
            std::cout << "Pause/resume example skipped: " << e.what() << std::endl;
        }

        // Example 4: Mute
        std::cout << "\n--- Testing mute ---" << std::endl;
        try {
            auto sound = audio->load_sound("assets/beep.wav");

            std::cout << "Playing sound (unmuted)..." << std::endl;
            audio->play_sound(sound);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            audio->set_muted(true);
            std::cout << "Playing sound (muted)..." << std::endl;
            audio->play_sound(sound);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            audio->set_muted(false);
            std::cout << "Playing sound (unmuted again)..." << std::endl;
            audio->play_sound(sound);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            audio->unload_sound(sound);
        } catch (const std::exception& e) {
            std::cout << "Mute example skipped: " << e.what() << std::endl;
        }

        // Cleanup
        std::cout << "\n--- Cleanup ---" << std::endl;
        audio->shutdown();
        std::cout << "Audio plugin shutdown complete" << std::endl;

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
