#pragma once

#include "ecs/Registry.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "LocalGameState.hpp"
#include "ScreenManager.hpp"
#include "network/NetworkManager.hpp"
#include <memory>

namespace bagario {

/**
 * @brief Main game class for Bagario
 */
class BagarioGame {
public:
    BagarioGame(int screen_width = 1280, int screen_height = 720);
    ~BagarioGame();

    bool initialize(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void run();
    void shutdown();

    bool should_close() const { return should_close_; }

private:
    void update();
    void draw();

    int screen_width_;
    int screen_height_;
    bool should_close_ = false;

    engine::IGraphicsPlugin* graphics_ = nullptr;
    engine::IInputPlugin* input_ = nullptr;

    Registry registry_;
    LocalGameState game_state_;
    std::unique_ptr<ScreenManager> screen_manager_;
    std::unique_ptr<client::NetworkManager> network_manager_;
};

}  // namespace bagario
