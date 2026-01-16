#include "BagarioGame.hpp"
#include <iostream>

namespace bagario {

BagarioGame::BagarioGame(int screen_width, int screen_height)
    : screen_width_(screen_width)
    , screen_height_(screen_height) {
}

BagarioGame::~BagarioGame() {
    shutdown();
}

bool BagarioGame::initialize(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    graphics_ = graphics;
    input_ = input;

    if (!graphics_ || !input_) {
        std::cerr << "[BagarioGame] Error: Graphics or Input plugin is null!\n";
        return false;
    }

    game_state_.load_all_configs();
    std::cout << "[BagarioGame] Loaded configuration files\n";
    graphics_->set_vsync(game_state_.vsync);
    network_manager_ = std::make_unique<client::NetworkManager>();
    if (!network_manager_->initialize())
        std::cerr << "[BagarioGame] Warning: Failed to initialize network manager\n";
    screen_manager_ = std::make_unique<ScreenManager>(game_state_, screen_width_, screen_height_);
    screen_manager_->set_network_manager(network_manager_.get());
    screen_manager_->initialize();
    std::cout << "[BagarioGame] Initialized successfully\n";
    return true;
}

void BagarioGame::run() {
    std::cout << "[BagarioGame] Starting game loop\n";

    while (!should_close_ && graphics_->is_window_open()) {
        update();
        draw();
    }
}

void BagarioGame::update() {
    if (screen_manager_)
        screen_manager_->update(graphics_, input_);
    if (input_)
        input_->update();
}

void BagarioGame::draw() {
    if (screen_manager_)
        screen_manager_->draw(graphics_);
    graphics_->display();
}

void BagarioGame::shutdown() {
    std::cout << "[BagarioGame] Shutting down\n";
    screen_manager_.reset();
    if (network_manager_) {
        network_manager_->shutdown();
        network_manager_.reset();
    }
}

}
