#include "ScreenManager.hpp"
#include "screens/BaseScreen.hpp"
#include "screens/WelcomeScreen.hpp"
#include "screens/SettingsScreen.hpp"
#include "screens/SkinScreen.hpp"
#include "screens/PlayingScreen.hpp"
#include "network/NetworkManager.hpp"
#include <iostream>

namespace bagario {

ScreenManager::ScreenManager(LocalGameState& game_state, int screen_width, int screen_height)
    : game_state_(game_state)
    , screen_width_(screen_width)
    , screen_height_(screen_height) {
}

ScreenManager::~ScreenManager() = default;

void ScreenManager::initialize() {
    // Create welcome screen
    auto welcome = std::make_unique<WelcomeScreen>(game_state_, screen_width_, screen_height_);
    welcome->set_screen_change_callback([this](GameScreen screen) {
        handle_screen_change(screen);
    });
    welcome->initialize();
    welcome_screen_ = std::move(welcome);

    // Create settings screen
    auto settings = std::make_unique<SettingsScreen>(game_state_, screen_width_, screen_height_);
    settings->set_screen_change_callback([this](GameScreen screen) {
        handle_screen_change(screen);
    });
    settings->initialize();
    settings_screen_ = std::move(settings);

    // Create skin customization screen
    auto skin = std::make_unique<SkinScreen>(game_state_, screen_width_, screen_height_);
    skin->set_screen_change_callback([this](GameScreen screen) {
        handle_screen_change(screen);
    });
    skin->initialize();
    skin_screen_ = std::move(skin);

    // Create playing screen
    auto playing = std::make_unique<PlayingScreen>(game_state_, screen_width_, screen_height_);
    playing->set_screen_change_callback([this](GameScreen screen) {
        handle_screen_change(screen);
    });
    playing->initialize();
    if (network_manager_) {
        playing->set_network_manager(network_manager_);
    }
    playing_screen_ = std::move(playing);

    current_screen_ = GameScreen::WELCOME;
}

void ScreenManager::set_screen(GameScreen screen) {
    handle_screen_change(screen);
}

void ScreenManager::set_network_manager(client::NetworkManager* network) {
    network_manager_ = network;
    // If playing screen already exists, update it
    if (playing_screen_) {
        auto* playing = dynamic_cast<PlayingScreen*>(playing_screen_.get());
        if (playing) {
            playing->set_network_manager(network);
        }
    }
}

void ScreenManager::handle_screen_change(GameScreen new_screen) {
    // Exit current screen
    switch (current_screen_) {
        case GameScreen::WELCOME:
            if (welcome_screen_) welcome_screen_->on_exit();
            break;
        case GameScreen::SETTINGS:
            if (settings_screen_) settings_screen_->on_exit();
            break;
        case GameScreen::SKIN:
            if (skin_screen_) skin_screen_->on_exit();
            break;
        case GameScreen::PLAYING:
            if (playing_screen_) playing_screen_->on_exit();
            break;
    }

    current_screen_ = new_screen;

    // Enter new screen
    switch (current_screen_) {
        case GameScreen::WELCOME:
            if (welcome_screen_) welcome_screen_->on_enter();
            break;
        case GameScreen::SETTINGS:
            if (settings_screen_) settings_screen_->on_enter();
            break;
        case GameScreen::SKIN:
            if (skin_screen_) skin_screen_->on_enter();
            break;
        case GameScreen::PLAYING:
            if (playing_screen_) playing_screen_->on_enter();
            break;
    }
}

void ScreenManager::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    switch (current_screen_) {
        case GameScreen::WELCOME:
            if (welcome_screen_) welcome_screen_->update(graphics, input);
            break;
        case GameScreen::SETTINGS:
            if (settings_screen_) settings_screen_->update(graphics, input);
            break;
        case GameScreen::SKIN:
            if (skin_screen_) skin_screen_->update(graphics, input);
            break;
        case GameScreen::PLAYING:
            if (playing_screen_) playing_screen_->update(graphics, input);
            break;
    }
}

void ScreenManager::draw(engine::IGraphicsPlugin* graphics) {
    switch (current_screen_) {
        case GameScreen::WELCOME:
            if (welcome_screen_) welcome_screen_->draw(graphics);
            break;
        case GameScreen::SETTINGS:
            if (settings_screen_) settings_screen_->draw(graphics);
            break;
        case GameScreen::SKIN:
            if (skin_screen_) skin_screen_->draw(graphics);
            break;
        case GameScreen::PLAYING:
            if (playing_screen_) playing_screen_->draw(graphics);
            break;
    }
}

}  // namespace bagario
