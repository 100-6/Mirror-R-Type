/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** MiniaudioPlugin - Miniaudio implementation of IAudioPlugin
*/

// Prevent Windows.h from defining min/max macros
#if defined(_WIN32) || defined(_WIN64)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "MiniaudioPlugin.hpp"
#include <stdexcept>
#include <iostream>
#include <algorithm>

namespace rtype {

// Constructor
MiniaudioPlugin::MiniaudioPlugin()
    : initialized_(false)
    , next_sound_handle_(1) // Start at 1, 0 is INVALID_HANDLE
    , next_music_handle_(1)
    , current_music_handle_(INVALID_HANDLE)
    , music_volume_(1.0f)
    , master_volume_(1.0f)
    , muted_(false) {
}

// Destructor
MiniaudioPlugin::~MiniaudioPlugin() {
    shutdown();
}

// IPlugin interface
const char* MiniaudioPlugin::get_name() const {
    return "Miniaudio Plugin";
}

const char* MiniaudioPlugin::get_version() const {
    return "1.0.0";
}

bool MiniaudioPlugin::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return true;
    }

    // Initialize miniaudio engine
    ma_result result = ma_engine_init(NULL, &engine_);
    if (result != MA_SUCCESS) {
        std::cerr << "Failed to initialize miniaudio engine: " << result << std::endl;
        return false;
    }

    initialized_ = true;
    return true;
}

void MiniaudioPlugin::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return;
    }

    // Stop and unload all sounds
    for (auto& [handle, sound_data] : sounds_) {
        ma_sound_uninit(&sound_data.sound);
    }
    sounds_.clear();

    // Stop and unload all music
    for (auto& [handle, music_data] : musics_) {
        ma_sound_uninit(&music_data.sound);
    }
    musics_.clear();

    // Uninitialize the engine
    ma_engine_uninit(&engine_);

    initialized_ = false;
}

bool MiniaudioPlugin::is_initialized() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return initialized_;
}

// Sound effects
SoundHandle MiniaudioPlugin::load_sound(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        throw std::runtime_error("Plugin not initialized");
    }

    // Validate path
    if (path.empty()) {
        throw std::invalid_argument("Sound path cannot be empty");
    }

    // Check for handle overflow
    if (next_sound_handle_ == INVALID_HANDLE) {
        throw std::runtime_error("Sound handle overflow");
    }

    // Create sound data
    SoundData sound_data;

    // Load sound using miniaudio
    ma_result result = ma_sound_init_from_file(
        &engine_,
        path.c_str(),
        0, // flags (0 = default)
        NULL, // group (NULL = no group)
        NULL, // fence (NULL = no fence)
        &sound_data.sound
    );

    if (result != MA_SUCCESS) {
        throw std::runtime_error("Failed to load sound: " + path);
    }

    // Generate handle
    SoundHandle handle = next_sound_handle_++;

    // Store in cache
    sounds_[handle] = sound_data;

    return handle;
}

void MiniaudioPlugin::unload_sound(SoundHandle handle) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sounds_.find(handle);
    if (it != sounds_.end()) {
        ma_sound_uninit(&it->second.sound);
        sounds_.erase(it);
    }
}

bool MiniaudioPlugin::play_sound(SoundHandle handle, float volume, float pitch) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || handle == INVALID_HANDLE) {
        return false;
    }

    auto it = sounds_.find(handle);
    if (it == sounds_.end()) {
        return false;
    }

    // Clamp volume to valid range [0.0, 1.0]
    volume = std::max(0.0f, std::min(1.0f, volume));

    // Clamp pitch to reasonable range [0.1, 10.0]
    pitch = std::max(0.1f, std::min(10.0f, pitch));

    // Set volume (apply master volume and mute)
    float final_volume = muted_ ? 0.0f : volume * master_volume_;
    ma_sound_set_volume(&it->second.sound, final_volume);

    // Set pitch
    ma_sound_set_pitch(&it->second.sound, pitch);

    // Stop if already playing to restart
    if (ma_sound_is_playing(&it->second.sound)) {
        ma_sound_stop(&it->second.sound);
    }

    // Seek to start
    ma_sound_seek_to_pcm_frame(&it->second.sound, 0);

    // Start playing
    ma_result result = ma_sound_start(&it->second.sound);
    if (result != MA_SUCCESS) {
        return false;
    }

    it->second.is_playing = true;
    return true;
}

void MiniaudioPlugin::stop_sound(SoundHandle handle) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || handle == INVALID_HANDLE) {
        return;
    }

    auto it = sounds_.find(handle);
    if (it != sounds_.end()) {
        ma_sound_stop(&it->second.sound);
        it->second.is_playing = false;
    }
}

bool MiniaudioPlugin::is_sound_playing(SoundHandle handle) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || handle == INVALID_HANDLE) {
        return false;
    }

    auto it = sounds_.find(handle);
    if (it == sounds_.end()) {
        return false;
    }

    return ma_sound_is_playing(&it->second.sound);
}

// Music
MusicHandle MiniaudioPlugin::load_music(const std::string& path) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        throw std::runtime_error("Plugin not initialized");
    }

    // Validate path
    if (path.empty()) {
        throw std::invalid_argument("Music path cannot be empty");
    }

    // Check for handle overflow
    if (next_music_handle_ == INVALID_HANDLE) {
        throw std::runtime_error("Music handle overflow");
    }

    // Create music data
    MusicData music_data;

    // Load music using miniaudio with streaming flag
    ma_result result = ma_sound_init_from_file(
        &engine_,
        path.c_str(),
        MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION, // Stream for music
        NULL, // group
        NULL, // fence
        &music_data.sound
    );

    if (result != MA_SUCCESS) {
        throw std::runtime_error("Failed to load music: " + path);
    }

    // Generate handle
    MusicHandle handle = next_music_handle_++;

    // Store in cache
    musics_[handle] = music_data;

    return handle;
}

void MiniaudioPlugin::unload_music(MusicHandle handle) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = musics_.find(handle);
    if (it != musics_.end()) {
        ma_sound_uninit(&it->second.sound);
        musics_.erase(it);

        // Clear current music if it was this one
        if (current_music_handle_ == handle) {
            current_music_handle_ = INVALID_HANDLE;
        }
    }
}

bool MiniaudioPlugin::play_music(MusicHandle handle, bool loop, float volume) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || handle == INVALID_HANDLE) {
        return false;
    }

    auto it = musics_.find(handle);
    if (it == musics_.end()) {
        return false;
    }

    // Clamp volume to valid range [0.0, 1.0]
    volume = std::max(0.0f, std::min(1.0f, volume));

    // Stop current music if any
    if (current_music_handle_ != INVALID_HANDLE && current_music_handle_ != handle) {
        auto current_it = musics_.find(current_music_handle_);
        if (current_it != musics_.end()) {
            ma_sound_stop(&current_it->second.sound);
            current_it->second.is_playing = false;
        }
    }

    // Set looping
    ma_sound_set_looping(&it->second.sound, loop ? MA_TRUE : MA_FALSE);
    it->second.is_looping = loop;

    // Set volume (apply master volume and mute)
    float final_volume = muted_ ? 0.0f : volume * master_volume_;
    ma_sound_set_volume(&it->second.sound, final_volume);
    it->second.volume = volume;
    music_volume_ = volume;

    // Stop if already playing to restart
    if (ma_sound_is_playing(&it->second.sound)) {
        ma_sound_stop(&it->second.sound);
    }

    // Seek to start
    ma_sound_seek_to_pcm_frame(&it->second.sound, 0);

    // Start playing
    ma_result result = ma_sound_start(&it->second.sound);
    if (result != MA_SUCCESS) {
        return false;
    }

    it->second.is_playing = true;
    current_music_handle_ = handle;

    return true;
}

void MiniaudioPlugin::stop_music() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || current_music_handle_ == INVALID_HANDLE) {
        return;
    }

    auto it = musics_.find(current_music_handle_);
    if (it != musics_.end()) {
        ma_sound_stop(&it->second.sound);
        it->second.is_playing = false;
    }

    current_music_handle_ = INVALID_HANDLE;
}

void MiniaudioPlugin::pause_music() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || current_music_handle_ == INVALID_HANDLE) {
        return;
    }

    auto it = musics_.find(current_music_handle_);
    if (it != musics_.end() && it->second.is_playing) {
        ma_sound_stop(&it->second.sound);
        // Note: miniaudio doesn't have a direct pause, so we stop
        // Resume will need to remember position
    }
}

void MiniaudioPlugin::resume_music() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || current_music_handle_ == INVALID_HANDLE) {
        return;
    }

    auto it = musics_.find(current_music_handle_);
    if (it != musics_.end() && !ma_sound_is_playing(&it->second.sound)) {
        ma_sound_start(&it->second.sound);
        it->second.is_playing = true;
    }
}

bool MiniaudioPlugin::is_music_playing() const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || current_music_handle_ == INVALID_HANDLE) {
        return false;
    }

    auto it = musics_.find(current_music_handle_);
    if (it == musics_.end()) {
        return false;
    }

    return ma_sound_is_playing(&it->second.sound);
}

void MiniaudioPlugin::set_music_volume(float volume) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Clamp volume to valid range [0.0, 1.0]
    volume = std::max(0.0f, std::min(1.0f, volume));

    music_volume_ = volume;

    if (!initialized_ || current_music_handle_ == INVALID_HANDLE) {
        return;
    }

    auto it = musics_.find(current_music_handle_);
    if (it != musics_.end()) {
        float final_volume = muted_ ? 0.0f : volume * master_volume_;
        ma_sound_set_volume(&it->second.sound, final_volume);
        it->second.volume = volume;
    }
}

float MiniaudioPlugin::get_music_volume() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return music_volume_;
}

// Global settings
void MiniaudioPlugin::set_master_volume(float volume) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Clamp volume to valid range [0.0, 1.0]
    volume = std::max(0.0f, std::min(1.0f, volume));

    master_volume_ = volume;

    if (!initialized_) {
        return;
    }

    // Update all sounds
    for (auto& [handle, sound_data] : sounds_) {
        if (ma_sound_is_playing(&sound_data.sound)) {
            float final_volume = muted_ ? 0.0f : volume;
            ma_sound_set_volume(&sound_data.sound, final_volume);
        }
    }

    // Update music
    if (current_music_handle_ != INVALID_HANDLE) {
        auto it = musics_.find(current_music_handle_);
        if (it != musics_.end()) {
            float final_volume = muted_ ? 0.0f : music_volume_ * volume;
            ma_sound_set_volume(&it->second.sound, final_volume);
        }
    }
}

float MiniaudioPlugin::get_master_volume() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return master_volume_;
}

void MiniaudioPlugin::set_muted(bool muted) {
    std::lock_guard<std::mutex> lock(mutex_);

    muted_ = muted;

    if (!initialized_) {
        return;
    }

    // Update all sounds
    for (auto& [handle, sound_data] : sounds_) {
        if (ma_sound_is_playing(&sound_data.sound)) {
            float volume = muted ? 0.0f : master_volume_;
            ma_sound_set_volume(&sound_data.sound, volume);
        }
    }

    // Update music
    if (current_music_handle_ != INVALID_HANDLE) {
        auto it = musics_.find(current_music_handle_);
        if (it != musics_.end()) {
            float volume = muted ? 0.0f : music_volume_ * master_volume_;
            ma_sound_set_volume(&it->second.sound, volume);
        }
    }
}

bool MiniaudioPlugin::is_muted() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return muted_;
}

} // namespace rtype

// Plugin factory functions with C safety wrappers
extern "C" {
    /**
     * @brief Safely create an audio plugin instance
     * @return Pointer to the created plugin, or nullptr on failure
     */
    rtype::IAudioPlugin* create_audio_plugin() {
        try {
            rtype::MiniaudioPlugin* plugin = new (std::nothrow) rtype::MiniaudioPlugin();
            if (!plugin) {
                std::cerr << "Failed to allocate MiniaudioPlugin" << std::endl;
                return nullptr;
            }
            return plugin;
        } catch (const std::exception& e) {
            std::cerr << "Exception in create_audio_plugin: " << e.what() << std::endl;
            return nullptr;
        } catch (...) {
            std::cerr << "Unknown exception in create_audio_plugin" << std::endl;
            return nullptr;
        }
    }

    /**
     * @brief Safely destroy an audio plugin instance
     * @param plugin Plugin to destroy (can be nullptr)
     */
    void destroy_audio_plugin(rtype::IAudioPlugin* plugin) {
        if (!plugin) {
            return;
        }

        try {
            delete plugin;
        } catch (const std::exception& e) {
            std::cerr << "Exception in destroy_audio_plugin: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception in destroy_audio_plugin" << std::endl;
        }
    }
}
