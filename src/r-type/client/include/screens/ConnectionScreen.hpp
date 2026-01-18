#pragma once

#include "BaseScreen.hpp"
#include "ScreenManager.hpp"
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace rtype::client {

/**
 * @brief Connection screen displayed at startup for entering server IP and port
 *
 * This screen allows the user to input the server address and port before
 * connecting to the game server. It is displayed before the main menu.
 */
class ConnectionScreen : public BaseScreen {
public:
    using ConnectCallback = std::function<void(const std::string& host, uint16_t port, const std::string& player_name)>;
    using ScreenChangeCallback = std::function<void(GameScreen)>;

    ConnectionScreen(NetworkClient& network_client, int screen_width, int screen_height);

    void initialize() override;
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) override;
    void draw(engine::IGraphicsPlugin* graphics) override;

    /**
     * @brief Set callback for when user clicks Connect
     */
    void set_connect_callback(ConnectCallback callback) {
        on_connect_ = callback;
    }

    /**
     * @brief Set callback for screen changes
     */
    void set_screen_change_callback(ScreenChangeCallback callback) {
        on_screen_change_ = callback;
    }

    /**
     * @brief Set connection error message to display
     */
    void set_error_message(const std::string& error);

    /**
     * @brief Clear error message
     */
    void clear_error_message();

    /**
     * @brief Set default values for the fields
     */
    void set_defaults(const std::string& host, uint16_t port, const std::string& player_name);

private:
    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UITextField>> fields_;
    std::vector<std::unique_ptr<UIButton>> buttons_;

    ConnectCallback on_connect_;
    ScreenChangeCallback on_screen_change_;

    // Background texture
    engine::TextureHandle background_texture_ = engine::INVALID_HANDLE;
    bool background_loaded_ = false;

    // Field indices
    static constexpr int FIELD_HOST = 0;
    static constexpr int FIELD_PORT = 1;
    static constexpr int FIELD_NAME = 2;

    // Error message
    std::string error_message_;
    float error_display_timer_ = 0.0f;

    // Connection state
    bool is_connecting_ = false;

    void attempt_connect();
};

}
