#pragma once

#include "BaseScreen.hpp"
#include "ScreenManager.hpp"
#include <functional>

namespace rtype::client {

/**
 * @brief Screen for creating a custom room with name, password, and max players
 */
class CreateRoomScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;
    using RoomCreatedCallback = std::function<void(uint8_t)>;

    CreateRoomScreen(NetworkClient& network_client, int screen_width, int screen_height);

    void initialize() override;
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) override;
    void draw(engine::IGraphicsPlugin* graphics) override;

    void set_screen_change_callback(ScreenChangeCallback callback) {
        on_screen_change_ = callback;
    }

    void set_room_created_callback(RoomCreatedCallback callback) {
        on_room_created_ = callback;
    }

    uint8_t get_configured_max_players() const { return max_players_; }

private:
    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UITextField>> fields_;
    std::vector<std::unique_ptr<UIButton>> buttons_;

    uint8_t max_players_ = 4;

    ScreenChangeCallback on_screen_change_;
    RoomCreatedCallback on_room_created_;
};

}  // namespace rtype::client
