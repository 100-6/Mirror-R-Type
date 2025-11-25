/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MiniaudioPlugin - Miniaudio implementation of IAudioPlugin
*/

#pragma once

#include "IAudioPlugin.hpp"
#include <miniaudio.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

namespace rtype {

/**
 * @brief Miniaudio implementation of the audio plugin interface
 *
 * This plugin uses miniaudio for playing sounds and music.
 * It provides a clean abstraction over miniaudio's API through
 * the IAudioPlugin interface.
 */
class MiniaudioPlugin : public IAudioPlugin {
public:
    MiniaudioPlugin();
    ~MiniaudioPlugin() override;

    // IPlugin interface
    const char* get_name() const override;
    const char* get_version() const override;
    bool initialize() override;
    void shutdown() override;
    bool is_initialized() const override;

    // Sound effects
    SoundHandle load_sound(const std::string& path) override;
    void unload_sound(SoundHandle handle) override;
    bool play_sound(SoundHandle handle, float volume = 1.0f, float pitch = 1.0f) override;
    void stop_sound(SoundHandle handle) override;
    bool is_sound_playing(SoundHandle handle) const override;

    // Music
    MusicHandle load_music(const std::string& path) override;
    void unload_music(MusicHandle handle) override;
    bool play_music(MusicHandle handle, bool loop = true, float volume = 1.0f) override;
    void stop_music() override;
    void pause_music() override;
    void resume_music() override;
    bool is_music_playing() const override;
    void set_music_volume(float volume) override;
    float get_music_volume() const override;

    // Global settings
    void set_master_volume(float volume) override;
    float get_master_volume() const override;
    void set_muted(bool muted) override;
    bool is_muted() const override;

private:
    struct SoundData {
        ma_sound sound;
        bool is_playing = false;
    };

    struct MusicData {
        ma_sound sound;
        bool is_playing = false;
        bool is_looping = false;
        float volume = 1.0f;
    };

    bool initialized_;

    // Miniaudio engine
    ma_engine engine_;

    // Resource caches
    std::unordered_map<SoundHandle, SoundData> sounds_;
    std::unordered_map<MusicHandle, MusicData> musics_;

    // Handle generators
    SoundHandle next_sound_handle_;
    MusicHandle next_music_handle_;

    // Current music state
    MusicHandle current_music_handle_;
    float music_volume_;

    // Global settings
    float master_volume_;
    bool muted_;

    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace rtype
