#include "screens/MainMenuScreen.hpp"
#include "NetworkClient.hpp"
#include "ScreenManager.hpp"
#include <iostream>

namespace rtype::client {

MainMenuScreen::MainMenuScreen(NetworkClient& network_client, int screen_width, int screen_height)
    : BaseScreen(network_client, screen_width, screen_height) {
}

void MainMenuScreen::initialize() {
    labels_.clear();
    buttons_.clear();

    float center_x = screen_width_ / 2.0f;
    float start_y = 120.0f;

    // Title
    auto title = std::make_unique<UILabel>(center_x, start_y, "R-TYPE", 120);
    title->set_alignment(UILabel::Alignment::CENTER);
    title->set_color(engine::Color{100, 150, 255, 255});
    labels_.push_back(std::move(title));

    // Buttons
    float button_width = 300.0f;
    float button_height = 50.0f;
    float button_x = center_x - button_width / 2.0f;
    float button_spacing = 70.0f;
    float buttons_start_y = start_y + 200;

    // Quick Match button
    auto quick_match_btn = std::make_unique<UIButton>(
        button_x, buttons_start_y, button_width, button_height, "Quick Match");
    quick_match_btn->set_on_click([this]() {
        std::cout << "[MainMenuScreen] Quick Match selected\n";
        network_client_.send_join_lobby(protocol::GameMode::SQUAD, protocol::Difficulty::NORMAL);
        if (on_screen_change_) {
            on_screen_change_(GameScreen::WAITING);
        }
    });
    buttons_.push_back(std::move(quick_match_btn));

    // Browse Rooms button
    auto browse_btn = std::make_unique<UIButton>(
        button_x, buttons_start_y + button_spacing, button_width, button_height, "Browse Rooms");
    browse_btn->set_on_click([this]() {
        std::cout << "[MainMenuScreen] Browse Rooms selected\n";
        network_client_.send_request_room_list();
        if (on_screen_change_) {
            on_screen_change_(GameScreen::BROWSE_ROOMS);
        }
    });
    buttons_.push_back(std::move(browse_btn));

    // Create Room button
    auto create_btn = std::make_unique<UIButton>(
        button_x, buttons_start_y + button_spacing * 2, button_width, button_height, "Create Room");
    create_btn->set_on_click([this]() {
        std::cout << "[MainMenuScreen] Create Room selected\n";
        if (on_screen_change_) {
            on_screen_change_(GameScreen::CREATE_ROOM);
        }
    });
    buttons_.push_back(std::move(create_btn));

    // Quit button
    auto quit_btn = std::make_unique<UIButton>(
        button_x, buttons_start_y + button_spacing * 3, button_width, button_height, "Quit");
    quit_btn->set_on_click([this]() {
        std::cout << "[MainMenuScreen] Quit selected\n";
        network_client_.disconnect();
        exit(0);
    });
    buttons_.push_back(std::move(quit_btn));
}

void MainMenuScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    for (auto& button : buttons_) {
        button->update(graphics, input);
    }
}

void MainMenuScreen::draw(engine::IGraphicsPlugin* graphics) {
    graphics->clear(engine::Color{20, 20, 30, 255});

    for (auto& label : labels_) {
        label->draw(graphics);
    }
    for (auto& button : buttons_) {
        button->draw(graphics);
    }
}

}  // namespace rtype::client
