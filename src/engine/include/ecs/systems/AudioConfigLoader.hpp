/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AudioConfigLoader - Loads audio configurations from JSON files
*/

#ifndef AUDIO_CONFIG_LOADER_HPP_
#define AUDIO_CONFIG_LOADER_HPP_

#include <string>
#include <unordered_map>
#include <optional>

namespace audio {

/**
 * @brief Definition for a single sound (SFX, music, or ambiance)
 */
struct SoundDefinition {
    std::string path;
    float volume = 1.0f;
    float pitchVariation = 0.0f;
    bool loop = false;
};

/**
 * @brief Settings for an audio category
 */
struct CategorySettings {
    float defaultVolume = 1.0f;
};

/**
 * @brief Mapping of level ID to music and ambiance IDs
 */
struct LevelAudioMapping {
    std::string musicId;
    std::string ambianceId;
};

/**
 * @brief Complete audio configuration loaded from JSON
 */
struct AudioConfiguration {
    std::string version;

    std::unordered_map<std::string, CategorySettings> categories;
    std::unordered_map<std::string, SoundDefinition> music;
    std::unordered_map<std::string, SoundDefinition> sfx;
    std::unordered_map<std::string, SoundDefinition> ambiance;
    std::unordered_map<std::string, LevelAudioMapping> levelAudio;

    /**
     * @brief Get music definition by ID
     * @param id Music identifier
     * @return SoundDefinition if found, nullopt otherwise
     */
    std::optional<SoundDefinition> getMusic(const std::string& id) const {
        auto it = music.find(id);
        if (it != music.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Get SFX definition by ID
     * @param id SFX identifier
     * @return SoundDefinition if found, nullopt otherwise
     */
    std::optional<SoundDefinition> getSfx(const std::string& id) const {
        auto it = sfx.find(id);
        if (it != sfx.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Get ambiance definition by ID
     * @param id Ambiance identifier
     * @return SoundDefinition if found, nullopt otherwise
     */
    std::optional<SoundDefinition> getAmbiance(const std::string& id) const {
        auto it = ambiance.find(id);
        if (it != ambiance.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Get level audio mapping by level ID
     * @param levelId Level identifier
     * @return LevelAudioMapping if found, nullopt otherwise
     */
    std::optional<LevelAudioMapping> getLevelAudio(const std::string& levelId) const {
        auto it = levelAudio.find(levelId);
        if (it != levelAudio.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};

/**
 * @brief Load audio configuration from JSON file
 * @param filepath Path to the JSON configuration file
 * @return AudioConfiguration structure
 * @throws std::runtime_error if file cannot be opened or parsed
 */
AudioConfiguration loadAudioConfig(const std::string& filepath);

/**
 * @brief Validate audio configuration
 * @param config Configuration to validate
 * @return true if valid, false otherwise
 */
bool validateAudioConfig(const AudioConfiguration& config);

} // namespace audio

#endif /* !AUDIO_CONFIG_LOADER_HPP_ */
