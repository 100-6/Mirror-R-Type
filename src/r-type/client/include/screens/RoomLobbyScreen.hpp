#pragma once

#include "BaseScreen.hpp"
#include "protocol/Payloads.hpp"
#include "ScreenManager.hpp"
#include "SpaceshipManager.hpp"
#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace rtype::client {

/**
 * @brief Player information for lobby display
 */
struct LobbyPlayer {
    uint32_t player_id = 0;
    std::string name = "";
    uint8_t ship_type = 0;  // 0-14 for different ship types (3 colors × 5 types from Spaceships.png)
    int slot_index = -1;    // Assigned slot in lobby (0 = host, 1+ = others in join order)
    bool is_ready = false;
    bool is_connected = false;
};

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
    void add_player(uint32_t player_id, const std::string& name, uint8_t ship_type = 0);
    void remove_player(uint32_t player_id);
    void set_player_ready(uint32_t player_id, bool ready);

    // Room state getters
    uint32_t get_room_id() const { return room_id_; }
    uint8_t get_min_players() const { return min_players_to_start_; }

    // Update player name (called when server broadcasts name change)
    void update_player_name(uint32_t player_id, const std::string& new_name);

private:
    void draw_player_slot(engine::IGraphicsPlugin* graphics, int slot_index,
                         float x, float y, float width, float height);
    void draw_name_input(engine::IGraphicsPlugin* graphics);
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

    // Player slots
    std::vector<LobbyPlayer> players_;

    // Error display
    std::string error_message_ = "";
    float error_timer_ = 0.0f;

    ScreenChangeCallback on_screen_change_;

    // Textures
    engine::TextureHandle background_texture_ = engine::INVALID_HANDLE;
    std::unique_ptr<SpaceshipManager> spaceship_manager_;
    bool textures_loaded_ = false;

    // Edit mode for button positioning
    bool edit_mode_ = false;  // Set to true to enable edit mode for buttons
    float move_speed_ = 5.0f;  // Pixels to move per key press

    // Button positions (finalized from edit mode)
    float leave_button_x_ = 30.0f;
    float leave_button_y_ = 45.0f;
    float leave_button_width_ = 200.0f;  // Increased from 150
    float leave_button_height_ = 60.0f;  // Increased from 40

    float decrease_button_x_ = 850.0f;
    float decrease_button_y_ = 820.0f;
    float increase_button_x_ = 1030.0f;
    float increase_button_y_ = 820.0f;
    float plus_minus_button_size_ = 50.0f;  // Reduced for smaller slider

    float start_button_x_ = 805.0f;
    float start_button_y_ = 965.0f;
    float start_button_width_ = 300.0f;  // Increased from 200
    float start_button_height_ = 70.0f;  // Increased from 50

    // Name change button
    float change_name_button_x_ = 1650.0f;
    float change_name_button_y_ = 45.0f;
    float change_name_button_width_ = 240.0f;
    float change_name_button_height_ = 60.0f;

    int selected_button_ = 0;  // 0=leave, 1=decrease, 2=increase, 3=start, 4=change_name

    // Name editing state
    bool editing_name_ = false;
    std::string name_input_buffer_ = "";
    float cursor_blink_timer_ = 0.0f;
    bool cursor_visible_ = true;
};

}  // namespace rtype::client
