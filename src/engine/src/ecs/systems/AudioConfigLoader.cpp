/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AudioConfigLoader implementation
*/

#include "ecs/systems/AudioConfigLoader.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

namespace audio {

AudioConfiguration loadAudioConfig(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open audio configuration file: " + filepath);
    }

    json j;
    try {
        file >> j;
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse audio JSON: " + std::string(e.what()));
    }

    AudioConfiguration config;

    // Load version
    if (j.contains("version")) {
        config.version = j["version"].get<std::string>();
    }

    // Load categories
    if (j.contains("categories") && j["categories"].is_object()) {
        for (auto& [key, value] : j["categories"].items()) {
            CategorySettings settings;
            if (value.contains("defaultVolume")) {
                settings.defaultVolume = value["defaultVolume"].get<float>();
                // Clamp to valid range
                if (settings.defaultVolume < 0.0f) settings.defaultVolume = 0.0f;
                if (settings.defaultVolume > 1.0f) settings.defaultVolume = 1.0f;
            }
            config.categories[key] = settings;
        }
    }

    // Helper lambda to parse SoundDefinition
    auto parseSoundDef = [](const json& soundJson) -> SoundDefinition {
        SoundDefinition def;

        if (soundJson.contains("path")) {
            def.path = soundJson["path"].get<std::string>();
        }
        if (soundJson.contains("volume")) {
            def.volume = soundJson["volume"].get<float>();
            if (def.volume < 0.0f) def.volume = 0.0f;
            if (def.volume > 1.0f) def.volume = 1.0f;
        }
        if (soundJson.contains("pitchVariation")) {
            def.pitchVariation = soundJson["pitchVariation"].get<float>();
            if (def.pitchVariation < 0.0f) def.pitchVariation = 0.0f;
            if (def.pitchVariation > 1.0f) def.pitchVariation = 1.0f;
        }
        if (soundJson.contains("loop")) {
            def.loop = soundJson["loop"].get<bool>();
        }

        return def;
    };

    // Load music definitions
    if (j.contains("music") && j["music"].is_object()) {
        for (auto& [key, value] : j["music"].items()) {
            config.music[key] = parseSoundDef(value);
        }
    }

    // Load SFX definitions
    if (j.contains("sfx") && j["sfx"].is_object()) {
        for (auto& [key, value] : j["sfx"].items()) {
            config.sfx[key] = parseSoundDef(value);
        }
    }

    // Load ambiance definitions
    if (j.contains("ambiance") && j["ambiance"].is_object()) {
        for (auto& [key, value] : j["ambiance"].items()) {
            config.ambiance[key] = parseSoundDef(value);
        }
    }

    // Load level audio mappings
    if (j.contains("levelAudio") && j["levelAudio"].is_object()) {
        for (auto& [key, value] : j["levelAudio"].items()) {
            LevelAudioMapping mapping;
            if (value.contains("music")) {
                mapping.musicId = value["music"].get<std::string>();
            }
            if (value.contains("ambiance")) {
                mapping.ambianceId = value["ambiance"].get<std::string>();
            }
            config.levelAudio[key] = mapping;
        }
    }

    std::cout << "[AudioConfigLoader] Loaded audio config v" << config.version
              << " with " << config.music.size() << " music, "
              << config.sfx.size() << " sfx, "
              << config.ambiance.size() << " ambiance tracks" << std::endl;

    return config;
}

bool validateAudioConfig(const AudioConfiguration& config) {
    bool valid = true;

    // Validate music entries
    for (const auto& [id, def] : config.music) {
        if (def.path.empty()) {
            std::cerr << "[AudioConfigLoader] Warning: Music '" << id << "' has empty path" << std::endl;
            valid = false;
        }
    }

    // Validate SFX entries
    for (const auto& [id, def] : config.sfx) {
        if (def.path.empty()) {
            std::cerr << "[AudioConfigLoader] Warning: SFX '" << id << "' has empty path" << std::endl;
            valid = false;
        }
    }

    // Validate ambiance entries
    for (const auto& [id, def] : config.ambiance) {
        if (def.path.empty()) {
            std::cerr << "[AudioConfigLoader] Warning: Ambiance '" << id << "' has empty path" << std::endl;
            valid = false;
        }
    }

    // Validate level audio mappings reference existing audio
    for (const auto& [levelId, mapping] : config.levelAudio) {
        if (!mapping.musicId.empty() && config.music.find(mapping.musicId) == config.music.end()) {
            std::cerr << "[AudioConfigLoader] Warning: Level '" << levelId
                      << "' references unknown music '" << mapping.musicId << "'" << std::endl;
        }
        if (!mapping.ambianceId.empty() && config.ambiance.find(mapping.ambianceId) == config.ambiance.end()) {
            std::cerr << "[AudioConfigLoader] Warning: Level '" << levelId
                      << "' references unknown ambiance '" << mapping.ambianceId << "'" << std::endl;
        }
    }

    return valid;
}

} // namespace audio
