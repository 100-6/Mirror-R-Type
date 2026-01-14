#pragma once

#include <string>

namespace bagario {

/**
 * @brief Local game state (stored in memory, no server)
 */
struct LocalGameState {
    std::string username = "Player";
    int music_volume = 70;
    int sfx_volume = 80;
    bool fullscreen = false;
};

}  // namespace bagario
