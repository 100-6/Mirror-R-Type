#include "MenuManager.hpp"
#include <iostream>

namespace rtype::client {

// =============================================================================
// CONSTRUCTOR & INITIALIZATION
// =============================================================================

MenuManager::MenuManager(NetworkClient& network_client, int screen_width, int screen_height)
    : network_client_(network_client)
    , screen_width_(screen_width)
    , screen_height_(screen_height)
    , current_screen_(GameScreen::MAIN_MENU) {
}

void MenuManager::initialize() {
    // Setup network callbacks
    network_client_.set_on_room_created([this](const protocol::ServerRoomCreatedPayload& payload) {
        on_room_created(payload);
    });

    network_client_.set_on_room_joined([this](const protocol::ServerRoomJoinedPayload& payload) {
        on_room_joined(payload);
    });

    network_client_.set_on_room_list([this](const std::vector<protocol::RoomInfo>& rooms) {
        on_room_list(rooms);
    });

    network_client_.set_on_room_error([this](const protocol::ServerRoomErrorPayload& payload) {
        on_room_error(payload);
    });

    network_client_.set_on_countdown([this](uint8_t seconds_remaining) {
        if (room_lobby_screen_) {
            room_lobby_screen_->set_countdown(seconds_remaining);
        }
        std::cout << "[MenuManager] Countdown: " << static_cast<int>(seconds_remaining) << "s\n";
    });

    // Create all screen instances
    main_menu_screen_ = std::make_unique<MainMenuScreen>(network_client_, screen_width_, screen_height_);
    create_room_screen_ = std::make_unique<CreateRoomScreen>(network_client_, screen_width_, screen_height_);
    browse_rooms_screen_ = std::make_unique<BrowseRoomsScreen>(network_client_, screen_width_, screen_height_);
    room_lobby_screen_ = std::make_unique<RoomLobbyScreen>(network_client_, screen_width_, screen_height_);
    password_dialog_ = std::make_unique<PasswordDialog>(screen_width_, screen_height_);

    // Set screen change callbacks
    main_menu_screen_->set_screen_change_callback([this](GameScreen screen) {
        set_screen(screen);
    });

    create_room_screen_->set_screen_change_callback([this](GameScreen screen) {
        set_screen(screen);
    });

    browse_rooms_screen_->set_screen_change_callback([this](GameScreen screen) {
        set_screen(screen);
    });

    room_lobby_screen_->set_screen_change_callback([this](GameScreen screen) {
        set_screen(screen);
    });

    // Set password dialog callbacks
    browse_rooms_screen_->set_password_dialog_callback([this](uint32_t room_id) {
        password_dialog_->show(room_id);
    });

    password_dialog_->set_join_callback([this](uint32_t room_id, const std::string& password) {
        network_client_.send_join_room(room_id, password);
    });

    password_dialog_->set_cancel_callback([this]() {
        // Do nothing, dialog will close
    });

    // Initialize all screens
    main_menu_screen_->initialize();
    create_room_screen_->initialize();
    browse_rooms_screen_->initialize();
    room_lobby_screen_->initialize();
    password_dialog_->initialize();
}

// =============================================================================
// SCREEN NAVIGATION
// =============================================================================

void MenuManager::set_screen(GameScreen screen) {
    // Call on_exit for previous screen
    switch (current_screen_) {
        case GameScreen::MAIN_MENU:
            if (main_menu_screen_) main_menu_screen_->on_exit();
            break;
        case GameScreen::CREATE_ROOM:
            if (create_room_screen_) create_room_screen_->on_exit();
            break;
        case GameScreen::BROWSE_ROOMS:
            if (browse_rooms_screen_) browse_rooms_screen_->on_exit();
            break;
        case GameScreen::ROOM_LOBBY:
            if (room_lobby_screen_) room_lobby_screen_->on_exit();
            break;
        default:
            break;
    }

    current_screen_ = screen;
    std::cout << "[MenuManager] Switched to screen: " << static_cast<int>(screen) << "\n";

    // Reset timers when entering room lobby
    if (screen == GameScreen::ROOM_LOBBY) {
        room_list_refresh_timer_ = 0.9f;  // First refresh in 0.1s
    }

    // Call on_enter for new screen
    switch (current_screen_) {
        case GameScreen::MAIN_MENU:
            if (main_menu_screen_) main_menu_screen_->on_enter();
            break;
        case GameScreen::CREATE_ROOM:
            if (create_room_screen_) create_room_screen_->on_enter();
            break;
        case GameScreen::BROWSE_ROOMS:
            if (browse_rooms_screen_) browse_rooms_screen_->on_enter();
            break;
        case GameScreen::ROOM_LOBBY:
            if (room_lobby_screen_) room_lobby_screen_->on_enter();
            break;
        default:
            break;
    }
}

// =============================================================================
// UPDATE & DRAW
// =============================================================================

void MenuManager::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    // Update password dialog if visible (it overlays the browse screen)
    if (password_dialog_ && password_dialog_->is_visible()) {
        password_dialog_->update(graphics, input);
        return;  // Don't update screen behind dialog
    }

    // Update current screen
    switch (current_screen_) {
        case GameScreen::MAIN_MENU:
            if (main_menu_screen_) main_menu_screen_->update(graphics, input);
            break;
        case GameScreen::CREATE_ROOM:
            if (create_room_screen_) create_room_screen_->update(graphics, input);
            break;
        case GameScreen::BROWSE_ROOMS:
            if (browse_rooms_screen_) browse_rooms_screen_->update(graphics, input);
            break;
        case GameScreen::ROOM_LOBBY:
            if (room_lobby_screen_) {
                room_lobby_screen_->update(graphics, input);

                // Periodic room list refresh (only if no countdown)
                if (room_lobby_screen_->get_room_id() != 0) {
                    room_list_refresh_timer_ += 0.016f;  // ~60 FPS
                    if (room_list_refresh_timer_ >= 1.0f) {  // Every 1 second
                        room_list_refresh_timer_ = 0.0f;
                        network_client_.send_request_room_list();
                    }
                }
            }
            break;
        default:
            break;
    }
}

void MenuManager::draw(engine::IGraphicsPlugin* graphics) {
    // Draw current screen
    switch (current_screen_) {
        case GameScreen::MAIN_MENU:
            if (main_menu_screen_) main_menu_screen_->draw(graphics);
            break;
        case GameScreen::CREATE_ROOM:
            if (create_room_screen_) create_room_screen_->draw(graphics);
            break;
        case GameScreen::BROWSE_ROOMS:
            if (browse_rooms_screen_) browse_rooms_screen_->draw(graphics);
            break;
        case GameScreen::ROOM_LOBBY:
            if (room_lobby_screen_) room_lobby_screen_->draw(graphics);
            break;
        default:
            break;
    }

    // Draw password dialog on top if visible
    if (password_dialog_ && password_dialog_->is_visible()) {
        password_dialog_->draw(graphics);
    }
}

// =============================================================================
// NETWORK CALLBACKS
// =============================================================================

void MenuManager::on_room_created(const protocol::ServerRoomCreatedPayload& payload) {
    std::cout << "[MenuManager] Room created: " << payload.room_name << "\n";

    // Get configured max players from create room screen
    uint8_t max_players = create_room_screen_ ? create_room_screen_->get_configured_max_players() : 4;

    // Update room lobby screen
    if (room_lobby_screen_) {
        room_lobby_screen_->set_room_info(
            payload.room_id,
            std::string(payload.room_name),
            1,  // Just the creator
            max_players,
            true  // Creator is the host
        );
        room_lobby_screen_->set_countdown(0);
    }

    // Request room list to get fresh data
    network_client_.send_request_room_list();

    set_screen(GameScreen::ROOM_LOBBY);
}

void MenuManager::on_room_joined(const protocol::ServerRoomJoinedPayload& payload) {
    std::cout << "[MenuManager] Joined room: " << payload.room_id << "\n";

    // Room info will be updated when we receive the room list
    // For now, set basic info
    if (room_lobby_screen_) {
        room_lobby_screen_->set_room_info(
            payload.room_id,
            "Room",  // Will be updated from room list
            1,       // Will be updated from room list
            4,       // Will be updated from room list
            false    // Joiner is not the host
        );
        room_lobby_screen_->set_countdown(0);
    }

    // Request updated room list
    network_client_.send_request_room_list();

    set_screen(GameScreen::ROOM_LOBBY);
}

void MenuManager::on_room_list(const std::vector<protocol::RoomInfo>& rooms) {
    std::cout << "[MenuManager] Received " << rooms.size() << " rooms\n";

    // Update browse rooms screen
    if (browse_rooms_screen_) {
        browse_rooms_screen_->set_room_list(rooms);
    }

    // Update room lobby screen if we're in a room
    if (current_screen_ == GameScreen::ROOM_LOBBY && room_lobby_screen_) {
        uint32_t current_room_id = room_lobby_screen_->get_room_id();
        if (current_room_id != 0) {
            for (const auto& room : rooms) {
                if (room.room_id == current_room_id) {
                    // Update only player counts and name, preserve host status
                    room_lobby_screen_->set_room_info(
                        room.room_id,
                        std::string(room.room_name),
                        room.current_players,
                        room.max_players,
                        room_lobby_screen_->get_room_id() == current_room_id  // Keep existing host status
                    );
                    std::cout << "[MenuManager] Updated room info: " << static_cast<int>(room.current_players)
                              << "/" << static_cast<int>(room.max_players) << " players\n";
                    break;
                }
            }
        }
    }
}

void MenuManager::on_room_error(const protocol::ServerRoomErrorPayload& payload) {
    std::cout << "[MenuManager] Room error: " << payload.error_message << "\n";

    // Show error in room lobby if we're there
    if (current_screen_ == GameScreen::ROOM_LOBBY && room_lobby_screen_) {
        room_lobby_screen_->set_error_message(std::string(payload.error_message));
    }

    // Otherwise go back to main menu
    if (current_screen_ != GameScreen::ROOM_LOBBY) {
        set_screen(GameScreen::MAIN_MENU);
    }
}

}  // namespace rtype::client
