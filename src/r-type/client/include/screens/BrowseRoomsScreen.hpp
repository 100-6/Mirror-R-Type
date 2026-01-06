#pragma once

#include "BaseScreen.hpp"
#include "ScreenManager.hpp"
#include "protocol/Payloads.hpp"
#include <functional>

namespace rtype::client {

/**
 * @brief Screen for browsing available rooms with Join buttons
 */
class BrowseRoomsScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;
    using PasswordDialogCallback = std::function<void(uint32_t)>;

    BrowseRoomsScreen(NetworkClient& network_client, int screen_width, int screen_height);

    void initialize() override;
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) override;
    void draw(engine::IGraphicsPlugin* graphics) override;

    void set_screen_change_callback(ScreenChangeCallback callback) {
        on_screen_change_ = callback;
    }

    void set_password_dialog_callback(PasswordDialogCallback callback) {
        on_password_dialog_ = callback;
    }

    void set_room_list(const std::vector<protocol::RoomInfo>& rooms);

private:
    void create_room_join_buttons();

    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UIButton>> buttons_;
    std::vector<std::unique_ptr<UIButton>> room_join_buttons_;

    std::vector<protocol::RoomInfo> available_rooms_;

    ScreenChangeCallback on_screen_change_;
    PasswordDialogCallback on_password_dialog_;
};

}  // namespace rtype::client
