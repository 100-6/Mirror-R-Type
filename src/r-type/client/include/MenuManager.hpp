#pragma once

#include <memory>
#include <map>
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "NetworkClient.hpp"
#include "ScreenManager.hpp"
#include "screens/BaseScreen.hpp"
#include "screens/MainMenuScreen.hpp"
#include "screens/CreateRoomScreen.hpp"
#include "screens/BrowseRoomsScreen.hpp"
#include "screens/RoomLobbyScreen.hpp"
#include "screens/PasswordDialog.hpp"
#include "screens/SettingsScreen.hpp"
#include "protocol/Payloads.hpp"

namespace rtype::client {

/**
 * @brief Manages all menu screens and UI interactions
 *
 * Refactored to use a screen-based architecture where each menu screen
 * is implemented as a separate class inheriting from BaseScreen.
 */
class MenuManager {
public:
    MenuManager(NetworkClient& network_client, int screen_width, int screen_height);

    /**
     * @brief Initialize menus and setup network callbacks
     */
    void initialize();

    /**
     * @brief Update current menu (handle input, animations)
     */
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);

    /**
     * @brief Render current menu
     */
    void draw(engine::IGraphicsPlugin* graphics);

    /**
     * @brief Get current screen
     */
    GameScreen get_current_screen() const { return current_screen_; }

    /**
     * @brief Set current screen
     */
    void set_screen(GameScreen screen);

    /**
     * @brief Handle player name update (called by ClientGame)
     */
    void on_player_name_updated(const protocol::ServerPlayerNameUpdatedPayload& payload);

    /**
     * @brief Handle player skin update (called by ClientGame)
     */
    void on_player_skin_updated(const protocol::ServerPlayerSkinUpdatedPayload& payload);

    /**
     * @brief Handle lobby state update (called by ClientGame)
     * Updates player list with names and skins
     */
    void on_lobby_state(const protocol::ServerLobbyStatePayload& state,
                        const std::vector<protocol::PlayerLobbyEntry>& players);

    /**
     * @brief Get settings screen (for accessing key bindings)
     */
    SettingsScreen* get_settings_screen() { return settings_screen_.get(); }

private:
    NetworkClient& network_client_;
    int screen_width_;
    int screen_height_;
    GameScreen current_screen_;

    // Screen instances
    std::unique_ptr<MainMenuScreen> main_menu_screen_;
    std::unique_ptr<CreateRoomScreen> create_room_screen_;
    std::unique_ptr<BrowseRoomsScreen> browse_rooms_screen_;
    std::unique_ptr<RoomLobbyScreen> room_lobby_screen_;
    std::unique_ptr<PasswordDialog> password_dialog_;
    std::unique_ptr<SettingsScreen> settings_screen_;

    // Room state tracking (for periodic refresh)
    float room_list_refresh_timer_ = 0.0f;

    // Network callbacks
    void on_room_created(const protocol::ServerRoomCreatedPayload& payload);
    void on_room_joined(const protocol::ServerRoomJoinedPayload& payload);
    void on_room_list(const std::vector<protocol::RoomInfo>& rooms);
    void on_room_error(const protocol::ServerRoomErrorPayload& payload);
};

}  // namespace rtype::client
