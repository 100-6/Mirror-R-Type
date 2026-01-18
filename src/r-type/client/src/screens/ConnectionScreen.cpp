#include "screens/ConnectionScreen.hpp"
#include "NetworkClient.hpp"
#include "AssetsPaths.hpp"
#include <iostream>

namespace rtype::client {

ConnectionScreen::ConnectionScreen(NetworkClient& network_client, int screen_width, int screen_height)
    : BaseScreen(network_client, screen_width, screen_height) {
}

void ConnectionScreen::initialize() {
    labels_.clear();
    fields_.clear();
    buttons_.clear();
    float center_x = screen_width_ / 2.0f;
    float field_width = 400.0f;
    float field_height = 60.0f;
    float field_spacing = 90.0f;
    float start_y = screen_height_ / 2.0f - 120.0f;
    auto title = std::make_unique<UILabel>(
        center_x, start_y - 120.0f, "R-TYPE", 60);
    title->set_alignment(UILabel::Alignment::CENTER);
    title->set_color({180, 120, 255, 255});
    labels_.push_back(std::move(title));
    auto subtitle = std::make_unique<UILabel>(
        center_x, start_y - 60.0f, "Connect to Server", 28);
    subtitle->set_alignment(UILabel::Alignment::CENTER);
    subtitle->set_color({160, 140, 200, 255});
    labels_.push_back(std::move(subtitle));
    auto host_label = std::make_unique<UILabel>(
        center_x - field_width / 2.0f, start_y - 15.0f, "Server Address", 18);
    host_label->set_alignment(UILabel::Alignment::LEFT);
    host_label->set_color({140, 120, 180, 255});
    labels_.push_back(std::move(host_label));
    auto host_field = std::make_unique<UITextField>(
        center_x - field_width / 2.0f, start_y, field_width, field_height, "127.0.0.1");
    host_field->set_text("127.0.0.1");
    host_field->set_max_length(45);
    fields_.push_back(std::move(host_field));
    auto port_label = std::make_unique<UILabel>(
        center_x - field_width / 2.0f, start_y + field_spacing - 15.0f, "Port", 18);
    port_label->set_alignment(UILabel::Alignment::LEFT);
    port_label->set_color({140, 120, 180, 255});
    labels_.push_back(std::move(port_label));
    auto port_field = std::make_unique<UITextField>(
        center_x - field_width / 2.0f, start_y + field_spacing, field_width, field_height, "4242");
    port_field->set_text("4242");
    port_field->set_max_length(5);
    fields_.push_back(std::move(port_field));
    auto name_label = std::make_unique<UILabel>(
        center_x - field_width / 2.0f, start_y + field_spacing * 2 - 15.0f, "Player Name", 18);
    name_label->set_alignment(UILabel::Alignment::LEFT);
    name_label->set_color({140, 120, 180, 255});
    labels_.push_back(std::move(name_label));
    auto name_field = std::make_unique<UITextField>(
        center_x - field_width / 2.0f, start_y + field_spacing * 2, field_width, field_height, "Pilot");
    name_field->set_text("Pilot");
    name_field->set_max_length(16);
    fields_.push_back(std::move(name_field));
    float button_width = 200.0f;
    float button_height = 60.0f;
    auto connect_btn = std::make_unique<UIButton>(
        center_x - button_width / 2.0f, start_y + field_spacing * 3 + 20.0f,
        button_width, button_height, "CONNECT");
    connect_btn->set_on_click([this]() {
        attempt_connect();
    });
    buttons_.push_back(std::move(connect_btn));
    auto quit_btn = std::make_unique<UIButton>(
        center_x - button_width / 2.0f, start_y + field_spacing * 3 + 100.0f,
        button_width, button_height, "QUIT");
    quit_btn->set_on_click([]() {
        exit(0);
    });
    buttons_.push_back(std::move(quit_btn));
}

void ConnectionScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    if (error_display_timer_ > 0.0f) {
        error_display_timer_ -= 0.016f;
        if (error_display_timer_ <= 0.0f)
            error_message_.clear();
    }
    if (input->is_key_just_pressed(engine::Key::Enter) && !is_connecting_) {
        attempt_connect();
        return;
    }
    if (input->is_key_just_pressed(engine::Key::Tab)) {
        int focused_index = -1;
        for (size_t i = 0; i < fields_.size(); ++i) {
            if (fields_[i]->is_focused()) {
                focused_index = static_cast<int>(i);
                fields_[i]->set_focused(false);
                break;
            }
        }

        if (input->is_key_pressed(engine::Key::LShift) || input->is_key_pressed(engine::Key::RShift))
            focused_index = (focused_index <= 0) ? static_cast<int>(fields_.size()) - 1 : focused_index - 1;
        else
            focused_index = (focused_index + 1) % static_cast<int>(fields_.size());
        fields_[focused_index]->set_focused(true);
    }
    for (auto& field : fields_)
        field->update(graphics, input);
    for (auto& button : buttons_)
        button->update(graphics, input);
}

void ConnectionScreen::draw(engine::IGraphicsPlugin* graphics) {
    if (!background_loaded_) {
        background_texture_ = graphics->load_texture(assets::paths::UI_MENU_BACKGROUND);
        background_loaded_ = true;
    }
    graphics->clear(engine::Color{10, 8, 20, 255});
    if (background_texture_ != engine::INVALID_HANDLE) {
        engine::Sprite bg_sprite;
        bg_sprite.texture_handle = background_texture_;
        bg_sprite.size = {static_cast<float>(screen_width_), static_cast<float>(screen_height_)};
        bg_sprite.origin = {0.0f, 0.0f};
        bg_sprite.tint = {100, 100, 100, 255};
        graphics->draw_sprite(bg_sprite, {0.0f, 0.0f});
    }
    float panel_width = 500.0f;
    float panel_height = 520.0f;
    float panel_x = (screen_width_ - panel_width) / 2.0f;
    float panel_y = screen_height_ / 2.0f - 200.0f;
    engine::Rectangle panel{panel_x, panel_y, panel_width, panel_height};
    graphics->draw_rectangle(panel, {20, 15, 35, 220});
    graphics->draw_rectangle_outline(panel, {100, 70, 150, 200}, 2.0f);
    for (auto& label : labels_)
        label->draw(graphics);
    for (auto& field : fields_)
        field->draw(graphics);
    for (auto& button : buttons_)
        button->draw(graphics);
    if (!error_message_.empty()) {
        float error_y = screen_height_ / 2.0f + 280.0f;
        engine::Vector2f error_pos{screen_width_ / 2.0f - 150.0f, error_y};
        graphics->draw_text(error_message_, error_pos, {255, 80, 80, 255}, engine::INVALID_HANDLE, 20);
    }
    if (is_connecting_) {
        engine::Rectangle overlay{0, 0, static_cast<float>(screen_width_), static_cast<float>(screen_height_)};
        graphics->draw_rectangle(overlay, {0, 0, 0, 180});
        engine::Vector2f text_pos{screen_width_ / 2.0f - 80.0f, screen_height_ / 2.0f};
        graphics->draw_text("Connecting...", text_pos, {180, 120, 255, 255}, engine::INVALID_HANDLE, 32);
    }
}

void ConnectionScreen::attempt_connect() {
    if (is_connecting_)
        return;
    std::string host = fields_[FIELD_HOST]->get_text();
    std::string port_str = fields_[FIELD_PORT]->get_text();
    std::string player_name = fields_[FIELD_NAME]->get_text();

    if (host.empty()) {
        set_error_message("Please enter a server address");
        return;
    }
    uint16_t port = 4242;
    try {
        int port_int = std::stoi(port_str);
        if (port_int <= 0 || port_int > 65535) {
            set_error_message("Invalid port number (1-65535)");
            return;
        }
        port = static_cast<uint16_t>(port_int);
    } catch (const std::exception&) {
        set_error_message("Invalid port number");
        return;
    }
    if (player_name.empty())
        player_name = "Pilot";
    std::cout << "[ConnectionScreen] Connecting to " << host << ":" << port << " as " << player_name << "\n";
    is_connecting_ = true;
    clear_error_message();
    if (on_connect_)
        on_connect_(host, port, player_name);
}

void ConnectionScreen::set_error_message(const std::string& error) {
    error_message_ = error;
    error_display_timer_ = 5.0f;
    is_connecting_ = false;
}

void ConnectionScreen::clear_error_message() {
    error_message_.clear();
    error_display_timer_ = 0.0f;
}

void ConnectionScreen::set_defaults(const std::string& host, uint16_t port, const std::string& player_name) {
    if (!fields_.empty()) {
        if (fields_.size() > FIELD_HOST)
            fields_[FIELD_HOST]->set_text(host);
        if (fields_.size() > FIELD_PORT)
            fields_[FIELD_PORT]->set_text(std::to_string(port));
        if (fields_.size() > FIELD_NAME)
            fields_[FIELD_NAME]->set_text(player_name);
    }
}

}
