/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AudioSystem - Extended implementation
*/

#include "ecs/systems/AudioSystem.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>

AudioSystem::AudioSystem(engine::IAudioPlugin& plugin, const std::string& configPath)
    : audio_plugin(plugin)
    , configPath_(configPath)
    , rng_(std::random_device{}())
{
}

AudioSystem::~AudioSystem()
{
}

void AudioSystem::init(Registry& registry)
{
    std::cout << "AudioSystem: Initialisation" << std::endl;

    // Load configuration
    loadConfiguration(configPath_);

    // Load sounds from config
    if (configLoaded_) {
        preloadSounds();
        preloadMusic();
        preloadAmbiance();
    } else {
        std::cerr << "AudioSystem: No configuration loaded, audio will not work" << std::endl;
    }

    // Initialize volume from config if available
    if (configLoaded_ && config_) {
        if (config_->categories.count("master")) {
            masterVolume_ = config_->categories["master"].defaultVolume;
        }
        if (config_->categories.count("music")) {
            musicVolume_ = config_->categories["music"].defaultVolume;
        }
        if (config_->categories.count("sfx")) {
            sfxVolume_ = config_->categories["sfx"].defaultVolume;
        }
        if (config_->categories.count("ambiance")) {
            ambianceVolume_ = config_->categories["ambiance"].defaultVolume;
        }
    }

    auto& eventBus = registry.get_event_bus();

    // Event subscriptions
    subscriptions.push_back(eventBus.subscribe<ecs::EnemyKilledEvent>(
        [this](const ecs::EnemyKilledEvent& e) { onEnemyKilled(e); }
    ));

    subscriptions.push_back(eventBus.subscribe<ecs::EnemyHitEvent>(
        [this](const ecs::EnemyHitEvent& e) { onEnemyHit(e); }
    ));

    subscriptions.push_back(eventBus.subscribe<ecs::PlayerHitEvent>(
        [this](const ecs::PlayerHitEvent& e) { onPlayerHit(e); }
    ));

    subscriptions.push_back(eventBus.subscribe<ecs::PowerUpCollectedEvent>(
        [this](const ecs::PowerUpCollectedEvent& e) { onPowerUpCollected(e); }
    ));

    subscriptions.push_back(eventBus.subscribe<ecs::ShotFiredEvent>(
        [this](const ecs::ShotFiredEvent& e) { onShotFired(e); }
    ));

    subscriptions.push_back(eventBus.subscribe<ecs::CompanionShotEvent>(
        [this](const ecs::CompanionShotEvent& e) { onCompanionShot(e); }
    ));

    // New event subscriptions
    subscriptions.push_back(eventBus.subscribe<ecs::ExplosionSoundEvent>(
        [this](const ecs::ExplosionSoundEvent& e) { onExplosionSound(e); }
    ));

    subscriptions.push_back(eventBus.subscribe<ecs::SceneChangeEvent>(
        [this](const ecs::SceneChangeEvent& e) { onSceneChange(e); }
    ));

    subscriptions.push_back(eventBus.subscribe<ecs::MusicChangeRequestEvent>(
        [this](const ecs::MusicChangeRequestEvent& e) { onMusicChangeRequest(e); }
    ));

    subscriptions.push_back(eventBus.subscribe<ecs::AmbianceChangeRequestEvent>(
        [this](const ecs::AmbianceChangeRequestEvent& e) { onAmbianceChangeRequest(e); }
    ));

    std::cout << "AudioSystem: Initialisation complete" << std::endl;
}

void AudioSystem::update(Registry& registry, float dt)
{
    (void)registry;

    // Update music fade transitions
    updateMusicFade(dt);

    // Update ambiance crossfade transitions
    updateAmbianceCrossfade(dt);
}

void AudioSystem::shutdown()
{
    std::cout << "AudioSystem: Shutdown" << std::endl;

    // Stop all playback
    stopMusic();
    stopAmbiance();

    // Unload sounds
    for (auto& [id, handle] : sfxHandles_) {
        if (handle != engine::INVALID_HANDLE) {
            audio_plugin.unload_sound(handle);
        }
    }

    for (auto& [id, handle] : musicHandles_) {
        if (handle != engine::INVALID_HANDLE) {
            audio_plugin.unload_music(handle);
        }
    }

    for (auto& [id, handle] : ambianceHandles_) {
        if (handle != engine::INVALID_HANDLE) {
            audio_plugin.unload_music(handle);
        }
    }

    sfxHandles_.clear();
    musicHandles_.clear();
    ambianceHandles_.clear();
}

// ========== Configuration Loading ==========

void AudioSystem::loadConfiguration(const std::string& configPath)
{
    try {
        config_ = std::make_unique<audio::AudioConfiguration>(
            audio::loadAudioConfig(configPath)
        );
        configLoaded_ = audio::validateAudioConfig(*config_);
        if (configLoaded_) {
            std::cout << "AudioSystem: Configuration loaded successfully" << std::endl;
        } else {
            std::cerr << "AudioSystem: Configuration validation failed" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "AudioSystem: Failed to load config: " << e.what() << std::endl;
        configLoaded_ = false;
    }
}

void AudioSystem::preloadSounds()
{
    if (!config_) return;

    for (const auto& [id, def] : config_->sfx) {
        auto handle = audio_plugin.load_sound(def.path);
        if (handle != engine::INVALID_HANDLE) {
            sfxHandles_[id] = handle;
        } else {
            std::cerr << "AudioSystem: Failed to load SFX '" << id << "' from " << def.path << std::endl;
        }
    }

    std::cout << "AudioSystem: Preloaded " << sfxHandles_.size() << " SFX" << std::endl;
}

void AudioSystem::preloadMusic()
{
    if (!config_) return;

    for (const auto& [id, def] : config_->music) {
        auto handle = audio_plugin.load_music(def.path);
        if (handle != engine::INVALID_HANDLE) {
            musicHandles_[id] = handle;
        } else {
            std::cerr << "AudioSystem: Failed to load music '" << id << "' from " << def.path << std::endl;
        }
    }

    std::cout << "AudioSystem: Preloaded " << musicHandles_.size() << " music tracks" << std::endl;
}

void AudioSystem::preloadAmbiance()
{
    if (!config_) return;

    for (const auto& [id, def] : config_->ambiance) {
        auto handle = audio_plugin.load_music(def.path);
        if (handle != engine::INVALID_HANDLE) {
            ambianceHandles_[id] = handle;
        } else {
            std::cerr << "AudioSystem: Failed to load ambiance '" << id << "' from " << def.path << std::endl;
        }
    }

    std::cout << "AudioSystem: Preloaded " << ambianceHandles_.size() << " ambiance tracks" << std::endl;
}

// ========== Sound Playback Helpers ==========

void AudioSystem::playSfx(const std::string& sfxId, float volumeMultiplier)
{
    if (muted_) return;

    auto it = sfxHandles_.find(sfxId);
    if (it == sfxHandles_.end() || it->second == engine::INVALID_HANDLE) {
        return;
    }

    float baseVolume = 1.0f;
    float pitchVariation = 0.0f;

    if (config_) {
        auto def = config_->getSfx(sfxId);
        if (def) {
            baseVolume = def->volume;
            pitchVariation = def->pitchVariation;
        }
    }

    float finalVolume = getEffectiveVolume(audio::AudioCategory::SFX) * baseVolume * volumeMultiplier;

    // Apply pitch variation
    float pitch = 1.0f;
    if (pitchVariation > 0.0f) {
        std::uniform_real_distribution<float> dist(-pitchVariation, pitchVariation);
        pitch = 1.0f + dist(rng_);
    }

    audio_plugin.play_sound(it->second, finalVolume, pitch);
}

void AudioSystem::playMusic(const std::string& musicId, bool loop)
{
    auto it = musicHandles_.find(musicId);
    if (it == musicHandles_.end() || it->second == engine::INVALID_HANDLE) {
        std::cerr << "AudioSystem: Music '" << musicId << "' not found" << std::endl;
        return;
    }

    float baseVolume = 1.0f;
    if (config_) {
        auto def = config_->getMusic(musicId);
        if (def) {
            baseVolume = def->volume;
        }
    }

    float finalVolume = getEffectiveVolume(audio::AudioCategory::MUSIC) * baseVolume;

    audio_plugin.play_music(it->second, loop, finalVolume);
    currentMusicHandle_ = it->second;
    currentMusicId_ = musicId;
}

void AudioSystem::playAmbiance(const std::string& ambianceId)
{
    if (ambianceId.empty()) {
        stopAmbiance();
        return;
    }

    auto it = ambianceHandles_.find(ambianceId);
    if (it == ambianceHandles_.end() || it->second == engine::INVALID_HANDLE) {
        std::cerr << "AudioSystem: Ambiance '" << ambianceId << "' not found" << std::endl;
        return;
    }

    float baseVolume = 1.0f;
    if (config_) {
        auto def = config_->getAmbiance(ambianceId);
        if (def) {
            baseVolume = def->volume;
        }
    }

    float finalVolume = getEffectiveVolume(audio::AudioCategory::AMBIANCE) * baseVolume;

    // Reset music fading flags since ambiance takes over the music channel
    isFadingIn_ = false;
    isFadingOut_ = false;
    currentMusicHandle_ = engine::INVALID_HANDLE;
    currentMusicId_.clear();

    // Note: Ambiance uses music API but with different volume category
    audio_plugin.play_music(it->second, true, finalVolume);
    currentAmbianceHandle_ = it->second;
    currentAmbianceId_ = ambianceId;
}

void AudioSystem::stopMusic()
{
    audio_plugin.stop_music();
    currentMusicHandle_ = engine::INVALID_HANDLE;
    currentMusicId_.clear();
    isFadingOut_ = false;
    isFadingIn_ = false;
}

void AudioSystem::stopAmbiance()
{
    if (currentAmbianceHandle_ != engine::INVALID_HANDLE) {
        // Note: We can only stop the current music, so if ambiance is separate
        // we need to handle this differently. For now, just clear state.
        currentAmbianceHandle_ = engine::INVALID_HANDLE;
        currentAmbianceId_.clear();
    }
    isCrossfadingAmbiance_ = false;
}

// ========== Transition Helpers ==========

void AudioSystem::startMusicFade(const std::string& newMusicId, float fadeOut, float fadeIn, bool loop)
{
    if (currentMusicId_ == newMusicId && !isFadingOut_) {
        return; // Already playing this music
    }

    pendingMusicId_ = newMusicId;
    fadeOutDuration_ = fadeOut;
    fadeInDuration_ = fadeIn;
    pendingMusicLoop_ = loop;

    if (currentMusicHandle_ != engine::INVALID_HANDLE && fadeOut > 0.0f) {
        // Start fade out
        isFadingOut_ = true;
        isFadingIn_ = false;
        fadeProgress_ = 0.0f;
        fadeStartVolume_ = audio_plugin.get_music_volume();
    } else {
        // No current music or instant transition
        stopMusic();
        if (!newMusicId.empty()) {
            playMusic(newMusicId, loop);
            if (fadeIn > 0.0f) {
                isFadingIn_ = true;
                fadeProgress_ = 0.0f;
                audio_plugin.set_music_volume(0.0f);
            }
        }
    }
}

void AudioSystem::startAmbianceCrossfade(const std::string& newAmbianceId, float duration)
{
    if (currentAmbianceId_ == newAmbianceId && !isCrossfadingAmbiance_) {
        return;
    }

    pendingAmbianceId_ = newAmbianceId;
    ambianceCrossfadeDuration_ = duration;

    if (duration > 0.0f && currentAmbianceHandle_ != engine::INVALID_HANDLE) {
        isCrossfadingAmbiance_ = true;
        ambianceCrossfadeProgress_ = 0.0f;
        fadingOutAmbianceHandle_ = currentAmbianceHandle_;
    } else {
        stopAmbiance();
        if (!newAmbianceId.empty()) {
            playAmbiance(newAmbianceId);
        }
    }
}

void AudioSystem::updateMusicFade(float dt)
{
    if (isFadingOut_) {
        fadeProgress_ += dt;
        float t = std::min(fadeProgress_ / fadeOutDuration_, 1.0f);

        float volume = fadeStartVolume_ * (1.0f - t);
        audio_plugin.set_music_volume(volume);

        if (t >= 1.0f) {
            // Fade out complete
            isFadingOut_ = false;
            stopMusic();

            if (!pendingMusicId_.empty()) {
                playMusic(pendingMusicId_, pendingMusicLoop_);
                if (fadeInDuration_ > 0.0f) {
                    isFadingIn_ = true;
                    fadeProgress_ = 0.0f;
                    audio_plugin.set_music_volume(0.0f);
                }
            }
            pendingMusicId_.clear();
        }
    } else if (isFadingIn_) {
        fadeProgress_ += dt;
        float t = std::min(fadeProgress_ / fadeInDuration_, 1.0f);

        float baseVolume = 1.0f;
        if (config_ && !currentMusicId_.empty()) {
            auto def = config_->getMusic(currentMusicId_);
            if (def) {
                baseVolume = def->volume;
            }
        }

        float targetVolume = getEffectiveVolume(audio::AudioCategory::MUSIC) * baseVolume;
        float volume = targetVolume * t;
        audio_plugin.set_music_volume(volume);

        if (t >= 1.0f) {
            isFadingIn_ = false;
        }
    }
}

void AudioSystem::updateAmbianceCrossfade(float dt)
{
    if (!isCrossfadingAmbiance_) return;

    ambianceCrossfadeProgress_ += dt;
    float t = std::min(ambianceCrossfadeProgress_ / ambianceCrossfadeDuration_, 1.0f);

    // For now, simplified crossfade - just switch at midpoint
    // (Full implementation would need multiple music channels)
    if (t >= 0.5f && currentAmbianceId_ != pendingAmbianceId_) {
        stopAmbiance();
        if (!pendingAmbianceId_.empty()) {
            playAmbiance(pendingAmbianceId_);
        }
    }

    if (t >= 1.0f) {
        isCrossfadingAmbiance_ = false;
        pendingAmbianceId_.clear();
        fadingOutAmbianceHandle_ = engine::INVALID_HANDLE;
    }
}

// ========== Volume Helpers ==========

float AudioSystem::getEffectiveVolume(audio::AudioCategory category) const
{
    if (muted_) return 0.0f;

    float categoryVolume = 1.0f;
    switch (category) {
        case audio::AudioCategory::MUSIC:
            categoryVolume = musicVolume_;
            break;
        case audio::AudioCategory::SFX:
            categoryVolume = sfxVolume_;
            break;
        case audio::AudioCategory::AMBIANCE:
            categoryVolume = ambianceVolume_;
            break;
        case audio::AudioCategory::MASTER:
        default:
            break;
    }
    return masterVolume_ * categoryVolume;
}

void AudioSystem::updateAllVolumes()
{
    // Note: AudioSystem calculates all final volumes (master * category * base)
    // and passes them directly to the plugin via set_music_volume().

    // Determine what's currently playing through the music channel
    // Ambiance and music share the same channel, so we need to check which one is active
    bool ambiancePlaying = (currentAmbianceHandle_ != engine::INVALID_HANDLE && !currentAmbianceId_.empty());
    bool musicPlaying = (currentMusicHandle_ != engine::INVALID_HANDLE && !currentMusicId_.empty());

    // If ambiance is playing, use ambiance volume category
    if (ambiancePlaying && !isFadingOut_ && !isFadingIn_) {
        float baseVolume = 1.0f;
        if (config_) {
            auto def = config_->getAmbiance(currentAmbianceId_);
            if (def) {
                baseVolume = def->volume;
            }
        }
        audio_plugin.set_music_volume(getEffectiveVolume(audio::AudioCategory::AMBIANCE) * baseVolume);
    }
    // Otherwise if music is playing, use music volume category
    else if (musicPlaying && !isFadingOut_ && !isFadingIn_) {
        float baseVolume = 1.0f;
        if (config_) {
            auto def = config_->getMusic(currentMusicId_);
            if (def) {
                baseVolume = def->volume;
            }
        }
        audio_plugin.set_music_volume(getEffectiveVolume(audio::AudioCategory::MUSIC) * baseVolume);
    }
}

// ========== Event Handlers ==========

void AudioSystem::onEnemyKilled(const ecs::EnemyKilledEvent& event)
{
    (void)event;
    playSfx("enemy_death");
}

void AudioSystem::onEnemyHit(const ecs::EnemyHitEvent& event)
{
    (void)event;
    playSfx("enemy_hit");
}

void AudioSystem::onPlayerHit(const ecs::PlayerHitEvent& event)
{
    (void)event;
    playSfx("player_hit");
}

void AudioSystem::onPowerUpCollected(const ecs::PowerUpCollectedEvent& event)
{
    (void)event;
    playSfx("powerup_collect");
}

void AudioSystem::onShotFired(const ecs::ShotFiredEvent& event)
{
    (void)event;
    playSfx("shoot");
}

void AudioSystem::onCompanionShot(const ecs::CompanionShotEvent& event)
{
    (void)event;
    playSfx("companion_shoot");
}

void AudioSystem::onExplosionSound(const ecs::ExplosionSoundEvent& event)
{
    std::string sfxId;
    switch (event.type) {
        case ecs::ExplosionSoundEvent::ExplosionType::ENEMY_BASIC:
            sfxId = "explosion_enemy_basic";
            break;
        case ecs::ExplosionSoundEvent::ExplosionType::ENEMY_TANK:
            sfxId = "explosion_enemy_tank";
            break;
        case ecs::ExplosionSoundEvent::ExplosionType::ENEMY_BOSS:
            sfxId = "explosion_boss";
            break;
        case ecs::ExplosionSoundEvent::ExplosionType::PLAYER:
            sfxId = "explosion_player";
            break;
    }

    // Apply scale-based volume variation
    float volumeMultiplier = std::clamp(event.scale, 0.5f, 1.5f);
    playSfx(sfxId, volumeMultiplier);
}

void AudioSystem::onSceneChange(const ecs::SceneChangeEvent& event)
{
    switch (event.newScene) {
        case ecs::SceneChangeEvent::SceneType::MENU:
            requestMusicChange("menu_theme", 1.0f, 1.0f, true);
            requestAmbianceChange("menu_ambient", 1.5f); // Menu ambiance
            break;

        case ecs::SceneChangeEvent::SceneType::GAMEPLAY: {
            std::string levelKey = "level_" + std::to_string(event.levelId);
            std::string musicId = "gameplay_level1";
            std::string ambianceId = "space_ambient";

            if (config_) {
                auto levelAudio = config_->getLevelAudio(levelKey);
                if (levelAudio) {
                    musicId = levelAudio->musicId;
                    ambianceId = levelAudio->ambianceId;
                }
            }

            requestMusicChange(musicId, 1.0f, 1.0f, true);
            requestAmbianceChange(ambianceId, 2.0f);
            break;
        }

        case ecs::SceneChangeEvent::SceneType::BOSS_FIGHT:
            requestMusicChange("boss_fight", 0.5f, 1.0f, true);
            break;

        case ecs::SceneChangeEvent::SceneType::VICTORY:
            requestMusicChange("victory", 1.0f, 0.5f, false);
            requestAmbianceChange("", 1.0f);
            break;

        case ecs::SceneChangeEvent::SceneType::GAME_OVER:
            requestMusicChange("game_over", 2.0f, 1.0f, false);
            requestAmbianceChange("", 2.0f);
            break;
    }
}

void AudioSystem::onMusicChangeRequest(const ecs::MusicChangeRequestEvent& event)
{
    requestMusicChange(event.musicId, event.fadeOutDuration, event.fadeInDuration, event.loop);
}

void AudioSystem::onAmbianceChangeRequest(const ecs::AmbianceChangeRequestEvent& event)
{
    requestAmbianceChange(event.ambianceId, event.crossfadeDuration);
}

// ========== Public Volume Control API ==========

void AudioSystem::setMasterVolume(float volume)
{
    masterVolume_ = std::clamp(volume, 0.0f, 1.0f);
    updateAllVolumes();
}

void AudioSystem::setMusicVolume(float volume)
{
    musicVolume_ = std::clamp(volume, 0.0f, 1.0f);
    updateAllVolumes();
}

void AudioSystem::setSfxVolume(float volume)
{
    sfxVolume_ = std::clamp(volume, 0.0f, 1.0f);
}

void AudioSystem::setAmbianceVolume(float volume)
{
    ambianceVolume_ = std::clamp(volume, 0.0f, 1.0f);
    updateAllVolumes();
}

void AudioSystem::setMuted(bool muted)
{
    muted_ = muted;
    audio_plugin.set_muted(muted);
}

// ========== Public Playback API ==========

void AudioSystem::requestMusicChange(const std::string& musicId, float fadeOut, float fadeIn, bool loop)
{
    startMusicFade(musicId, fadeOut, fadeIn, loop);
}

void AudioSystem::requestAmbianceChange(const std::string& ambianceId, float crossfade)
{
    startAmbianceCrossfade(ambianceId, crossfade);
}

void AudioSystem::triggerSfx(const std::string& sfxId, float volumeMultiplier)
{
    playSfx(sfxId, volumeMultiplier);
}
