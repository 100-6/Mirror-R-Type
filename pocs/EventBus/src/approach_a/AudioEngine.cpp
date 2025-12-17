#include "approach_a/AudioEngine.hpp"

namespace approach_a {

void AudioEngine::playSound(const std::string& soundFile) {
    // In a real implementation, this would play actual audio
    // For the POC, we just record that the sound was played
    playedSounds_.push_back(soundFile);
}

}
