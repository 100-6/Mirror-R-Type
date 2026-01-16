/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AudioSystem - Extended audio system with music, SFX, ambiance and volume control
*/

#ifndef AUDIOSYSTEM_HPP_
#define AUDIOSYSTEM_HPP_

#include "ISystem.hpp"
#include "ecs/Registry.hpp"
#include "plugin_manager/IAudioPlugin.hpp"
#include "core/event/EventBus.hpp"
#include "ecs/events/GameEvents.hpp"
#include "ecs/events/InputEvents.hpp"
#include "ecs/systems/AudioConfigLoader.hpp"
#include "ecs/AudioComponents.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <random>

class AudioSystem : public ISystem {
private:
    engine::IAudioPlugin& audio_plugin;

    // Configuration
    std::unique_ptr<audio::AudioConfiguration> config_;
    std::string configPath_;
    bool configLoaded_ = false;

    // Sound handle caches (keyed by sound ID from config)
    std::unordered_map<std::string, engine::SoundHandle> sfxHandles_;
    std::unordered_map<std::string, engine::MusicHandle> musicHandles_;
    std::unordered_map<std::string, engine::MusicHandle> ambianceHandles_;

    // Legacy sound handles (backward compatible)
    engine::SoundHandle enemy_death_sound = engine::INVALID_HANDLE;
    engine::SoundHandle player_hit_sound = engine::INVALID_HANDLE;
    engine::SoundHandle powerup_sound = engine::INVALID_HANDLE;
    engine::SoundHandle shoot_sound = engine::INVALID_HANDLE;

    // Current playback state
    std::string currentMusicId_;
    std::string currentAmbianceId_;
    engine::MusicHandle currentMusicHandle_ = engine::INVALID_HANDLE;
    engine::MusicHandle currentAmbianceHandle_ = engine::INVALID_HANDLE;

    // Volume state
    float masterVolume_ = 1.0f;
    float musicVolume_ = 0.7f;
    float sfxVolume_ = 1.0f;
    float ambianceVolume_ = 0.5f;
    bool muted_ = false;

    // Music transition state
    bool isFadingOut_ = false;
    bool isFadingIn_ = false;
    float fadeProgress_ = 0.0f;
    float fadeOutDuration_ = 1.0f;
    float fadeInDuration_ = 1.0f;
    std::string pendingMusicId_;
    bool pendingMusicLoop_ = true;
    float fadeStartVolume_ = 0.0f;

    // Ambiance transition state
    bool isCrossfadingAmbiance_ = false;
    float ambianceCrossfadeProgress_ = 0.0f;
    float ambianceCrossfadeDuration_ = 2.0f;
    std::string pendingAmbianceId_;
    engine::MusicHandle fadingOutAmbianceHandle_ = engine::INVALID_HANDLE;

    // Random generator for pitch variation
    std::mt19937 rng_;

    // Subscription IDs
    std::vector<core::EventBus::SubscriptionId> subscriptions;

    // ========== Private Methods ==========

    // Configuration loading
    void loadConfiguration(const std::string& configPath);
    void preloadSounds();
    void preloadMusic();
    void preloadAmbiance();
    void loadLegacySounds();

    // Sound playback helpers
    void playSfx(const std::string& sfxId, float volumeMultiplier = 1.0f);
    void playMusic(const std::string& musicId, bool loop = true);
    void playAmbiance(const std::string& ambianceId);
    void stopMusic();
    void stopAmbiance();

    // Transition helpers
    void startMusicFade(const std::string& newMusicId, float fadeOut, float fadeIn, bool loop);
    void startAmbianceCrossfade(const std::string& newAmbianceId, float duration);
    void updateMusicFade(float dt);
    void updateAmbianceCrossfade(float dt);

    // Volume helpers
    float getEffectiveVolume(audio::AudioCategory category) const;
    void updateAllVolumes();

    // Event handlers
    void onEnemyKilled(const ecs::EnemyKilledEvent& event);
    void onPlayerHit(const ecs::PlayerHitEvent& event);
    void onPowerUpCollected(const ecs::PowerUpCollectedEvent& event);
    void onShotFired(const ecs::ShotFiredEvent& event);
    void onCompanionShot(const ecs::CompanionShotEvent& event);
    void onExplosionSound(const ecs::ExplosionSoundEvent& event);
    void onSceneChange(const ecs::SceneChangeEvent& event);
    void onMusicChangeRequest(const ecs::MusicChangeRequestEvent& event);
    void onAmbianceChangeRequest(const ecs::AmbianceChangeRequestEvent& event);

public:
    /**
     * @brief Constructor with optional config path
     * @param plugin Audio plugin reference
     * @param configPath Path to audio configuration JSON (optional)
     */
    explicit AudioSystem(engine::IAudioPlugin& plugin,
                        const std::string& configPath = "assets/audio/audio_config.json");
    ~AudioSystem() override;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

    // ========== Public Volume Control API ==========

    void setMasterVolume(float volume);
    void setMusicVolume(float volume);
    void setSfxVolume(float volume);
    void setAmbianceVolume(float volume);
    void setMuted(bool muted);

    float getMasterVolume() const { return masterVolume_; }
    float getMusicVolume() const { return musicVolume_; }
    float getSfxVolume() const { return sfxVolume_; }
    float getAmbianceVolume() const { return ambianceVolume_; }
    bool isMuted() const { return muted_; }

    // ========== Public Playback API ==========

    /**
     * @brief Request music change with fade transition
     */
    void requestMusicChange(const std::string& musicId, float fadeOut = 1.0f,
                           float fadeIn = 1.0f, bool loop = true);

    /**
     * @brief Request ambiance change with crossfade
     */
    void requestAmbianceChange(const std::string& ambianceId, float crossfade = 2.0f);

    /**
     * @brief Trigger a sound effect by ID
     */
    void triggerSfx(const std::string& sfxId, float volumeMultiplier = 1.0f);

    /**
     * @brief Get current music ID
     */
    const std::string& getCurrentMusicId() const { return currentMusicId_; }

    /**
     * @brief Get current ambiance ID
     */
    const std::string& getCurrentAmbianceId() const { return currentAmbianceId_; }

    /**
     * @brief Check if configuration was loaded successfully
     */
    bool isConfigLoaded() const { return configLoaded_; }
};

#endif /* !AUDIOSYSTEM_HPP_ */
