/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AudioComponents - Components for audio state management
*/

#ifndef AUDIO_COMPONENTS_HPP_
#define AUDIO_COMPONENTS_HPP_

#include <string>

namespace audio {

/**
 * @brief Audio category types for volume control
 */
enum class AudioCategory {
    MASTER,
    MUSIC,
    SFX,
    AMBIANCE
};

/**
 * @brief Component for managing audio volume settings
 * This can be attached to a singleton entity for global audio control
 */
struct VolumeController {
    float masterVolume = 1.0f;
    float musicVolume = 0.7f;
    float sfxVolume = 1.0f;
    float ambianceVolume = 0.5f;
    bool muted = false;

    /**
     * @brief Calculate effective volume for a category
     * @param category The audio category
     * @return Combined volume (master * category), 0 if muted
     */
    float getEffectiveVolume(AudioCategory category) const {
        if (muted) return 0.0f;

        float categoryVolume = 1.0f;
        switch (category) {
            case AudioCategory::MUSIC:
                categoryVolume = musicVolume;
                break;
            case AudioCategory::SFX:
                categoryVolume = sfxVolume;
                break;
            case AudioCategory::AMBIANCE:
                categoryVolume = ambianceVolume;
                break;
            case AudioCategory::MASTER:
            default:
                break;
        }
        return masterVolume * categoryVolume;
    }
};

/**
 * @brief Component for tracking music transition state
 */
struct MusicState {
    std::string currentMusicId;
    std::string pendingMusicId;
    float fadeProgress = 0.0f;
    float fadeOutDuration = 1.0f;
    float fadeInDuration = 1.0f;
    bool isFadingOut = false;
    bool isFadingIn = false;
    bool pendingLoop = true;
};

/**
 * @brief Component for tracking ambiance transition state
 */
struct AmbianceState {
    std::string currentAmbianceId;
    std::string pendingAmbianceId;
    float crossfadeProgress = 0.0f;
    float crossfadeDuration = 2.0f;
    bool isCrossfading = false;
};

} // namespace audio

#endif /* !AUDIO_COMPONENTS_HPP_ */
