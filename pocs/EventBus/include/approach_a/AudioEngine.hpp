#pragma once

#include <string>
#include <vector>

namespace approach_a {

/**
 * @brief Audio engine responsible for playing sounds
 */
class AudioEngine {
public:
    AudioEngine() = default;
    ~AudioEngine() = default;

    /**
     * @brief Play a sound effect
     * @param soundFile The path to the sound file
     */
    void playSound(const std::string& soundFile);

    /**
     * @brief Get the list of played sounds (for testing)
     * @return Vector of sound file names
     */
    const std::vector<std::string>& getPlayedSounds() const { return playedSounds_; }

    /**
     * @brief Clear the played sounds history
     */
    void clear() { playedSounds_.clear(); }

private:
    std::vector<std::string> playedSounds_;
};

}
