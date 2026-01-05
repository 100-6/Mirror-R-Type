#pragma once

#include "BaseScreen.hpp"
#include "protocol/Payloads.hpp"
#include "ScreenManager.hpp"
#include <functional>
#include <string>

namespace rtype::client {

/**
 * @brief Room lobby screen where players wait for game to start
 *
 * Host can configure min players and start the game.
 * Guests wait for the host to start.
 * Shows countdown when game is starting.
 */
class RoomLobbyScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;

    RoomLobbyScreen(NetworkClient& network_client, int screen_width, int screen_height);

    void initialize() override;
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) override;
    void draw(engine::IGraphicsPlugin* graphics) override;

    void set_screen_change_callback(ScreenChangeCallback callback) {
        on_screen_change_ = callback;
    }

    // Room state setters
    void set_room_info(uint32_t room_id, const std::string& room_name,
                       uint8_t current_players, uint8_t max_players, bool is_host);
    void set_countdown(uint8_t seconds);
    void set_error_message(const std::string& message, float duration = 3.0f);

    // Room state getters
    uint32_t get_room_id() const { return room_id_; }
    uint8_t get_min_players() const { return min_players_to_start_; }

private:
    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UIButton>> buttons_;

    // Room state
    uint32_t room_id_ = 0;
    std::string room_name_ = "";
    uint8_t current_players_ = 0;
    uint8_t max_players_ = 4;
    uint8_t min_players_to_start_ = 2;
    bool is_host_ = false;
    uint8_t countdown_value_ = 0;

    // Error display
    std::string error_message_ = "";
    float error_timer_ = 0.0f;

    ScreenChangeCallback on_screen_change_;
};

}  // namespace rtype::client
