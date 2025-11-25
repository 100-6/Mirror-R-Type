/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** IAudioPlugin - Audio plugin interface
*/

#pragma once

#include "IPlugin.hpp"
#include "CommonTypes.hpp"
#include <string>

namespace rtype {

/**
 * @brief Audio plugin interface
 * 
 * This interface defines the contract for all audio plugins.
 * Implementations can use SFML, OpenAL, or any other audio library.
 */
class IAudioPlugin : public IPlugin {
public:
    virtual ~IAudioPlugin() = default;

    // Sound effects
    /**
     * @brief Load a sound effect from file
     * @param path Path to the sound file
     * @return Sound handle, or INVALID_HANDLE on failure
     */
    virtual SoundHandle load_sound(const std::string& path) = 0;

    /**
     * @brief Unload a sound effect
     * @param handle Sound handle to unload
     */
    virtual void unload_sound(SoundHandle handle) = 0;

    /**
     * @brief Play a sound effect
     * @param handle Sound handle
     * @param volume Volume (0.0 to 1.0)
     * @param pitch Pitch multiplier (1.0 = normal)
     * @return true if sound started playing
     */
    virtual bool play_sound(SoundHandle handle, float volume = 1.0f, float pitch = 1.0f) = 0;

    /**
     * @brief Stop a playing sound
     * @param handle Sound handle
     */
    virtual void stop_sound(SoundHandle handle) = 0;

    /**
     * @brief Check if a sound is currently playing
     * @param handle Sound handle
     * @return true if sound is playing
     */
    virtual bool is_sound_playing(SoundHandle handle) const = 0;

    // Music
    /**
     * @brief Load music from file
     * @param path Path to the music file
     * @return Music handle, or INVALID_HANDLE on failure
     */
    virtual MusicHandle load_music(const std::string& path) = 0;

    /**
     * @brief Unload music
     * @param handle Music handle to unload
     */
    virtual void unload_music(MusicHandle handle) = 0;

    /**
     * @brief Play music
     * @param handle Music handle
     * @param loop Whether to loop the music
     * @param volume Volume (0.0 to 1.0)
     * @return true if music started playing
     */
    virtual bool play_music(MusicHandle handle, bool loop = true, float volume = 1.0f) = 0;

    /**
     * @brief Stop currently playing music
     */
    virtual void stop_music() = 0;

    /**
     * @brief Pause currently playing music
     */
    virtual void pause_music() = 0;

    /**
     * @brief Resume paused music
     */
    virtual void resume_music() = 0;

    /**
     * @brief Check if music is currently playing
     * @return true if music is playing
     */
    virtual bool is_music_playing() const = 0;

    /**
     * @brief Set music volume
     * @param volume Volume (0.0 to 1.0)
     */
    virtual void set_music_volume(float volume) = 0;

    /**
     * @brief Get current music volume
     * @return Volume (0.0 to 1.0)
     */
    virtual float get_music_volume() const = 0;

    // Global settings
    /**
     * @brief Set master volume for all sounds
     * @param volume Volume (0.0 to 1.0)
     */
    virtual void set_master_volume(float volume) = 0;

    /**
     * @brief Get master volume
     * @return Volume (0.0 to 1.0)
     */
    virtual float get_master_volume() const = 0;

    /**
     * @brief Mute or unmute all audio
     * @param muted true to mute, false to unmute
     */
    virtual void set_muted(bool muted) = 0;

    /**
     * @brief Check if audio is muted
     * @return true if muted
     */
    virtual bool is_muted() const = 0;
};

} // namespace rtype

// Plugin factory function signatures
extern "C" {
    /**
     * @brief Factory function to create an audio plugin instance
     * @return Pointer to the created plugin
     */
    rtype::IAudioPlugin* create_audio_plugin();

    /**
     * @brief Destroy an audio plugin instance
     * @param plugin Plugin to destroy
     */
    void destroy_audio_plugin(rtype::IAudioPlugin* plugin);
}
