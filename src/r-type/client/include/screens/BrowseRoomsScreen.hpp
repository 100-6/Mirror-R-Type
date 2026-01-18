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

    engine::TextureHandle background_texture_ = engine::INVALID_HANDLE;
    bool background_loaded_ = false;

    // Edit mode for button positioning
    bool edit_mode_ = false;  // Set to true to enable edit mode for buttons
    float move_speed_ = 5.0f;  // Pixels to move per key press

    // Button positions (finalized from edit mode)
    float back_button_x_ = 125.0f;
    float back_button_y_ = 80.0f;
    float back_button_width_ = 165.0f;
    float back_button_height_ = 52.0f;

    float refresh_button_x_ = 1655.0f;
    float refresh_button_y_ = 85.0f;
    float refresh_button_width_ = 180.0f;
    float refresh_button_height_ = 55.0f;

    // Join button positioning variables
    float join_button_width_ = 145.0f;
    float join_button_height_ = 55.0f;
    float join_button_offset_ = -15.0f;  // Offset from right edge (after screen_width - 250 - button_width)

    int selected_button_ = 0;  // 0=back, 1=refresh, 2=join
};

}  // namespace rtype::client
