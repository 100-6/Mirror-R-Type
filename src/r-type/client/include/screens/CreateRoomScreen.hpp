#pragma once

#include "BaseScreen.hpp"
#include "ScreenManager.hpp"
#include "protocol/Payloads.hpp"
#include "screens/createroom/CreateRoomRenderer.hpp"
#include <functional>

namespace rtype::client {

/**
 * @brief Map identifiers for the 3 available maps
 */
enum class MapId : uint16_t {
    NEBULA_OUTPOST = 1,    // Map 1 - Intro level
    ASTEROID_BELT = 2,     // Map 2 - Navigation challenge
    BYDO_MOTHERSHIP = 3    // Map 3 - Final battle
};

/**
 * @brief Screen for creating a custom room with name, password, game mode, map, and difficulty
 */
class CreateRoomScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;
    using RoomCreatedCallback = std::function<void(protocol::GameMode, uint8_t)>;

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

    protocol::GameMode get_configured_game_mode() const { return game_mode_; }
    protocol::Difficulty get_configured_difficulty() const { return difficulty_; }
    uint16_t get_configured_map_id() const { return static_cast<uint16_t>(map_id_); }
    uint8_t get_configured_max_players() const;

private:
    // Stepper state
    enum class Step {
        ROOM_INFO = 0,    // Step 1: Room name and password
        MAP_SELECTION = 1, // Step 2: Choose map
        DIFFICULTY = 2,    // Step 3: Choose difficulty
        GAME_MODE = 3      // Step 4: Choose game mode
    };

    Step current_step_ = Step::ROOM_INFO;
    static constexpr int TOTAL_STEPS = 4;

    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UITextField>> fields_;
    std::vector<std::unique_ptr<UIButton>> buttons_;
    std::vector<std::unique_ptr<UIButton>> mode_buttons_;       // DUO, TRIO, SQUAD buttons
    std::vector<std::unique_ptr<UIButton>> map_buttons_;        // Map selection buttons
    std::vector<std::unique_ptr<UIButton>> difficulty_buttons_; // Difficulty selection buttons
    std::vector<std::unique_ptr<UIButton>> nav_buttons_;        // Next, Previous, Create buttons

    // Texture pack (handled by createroom module)
    createroom::TexturePack textures_;

    protocol::GameMode game_mode_ = protocol::GameMode::SQUAD;  // Default to SQUAD
    protocol::Difficulty difficulty_ = protocol::Difficulty::NORMAL;  // Default to NORMAL
    MapId map_id_ = MapId::NEBULA_OUTPOST;  // Default to first map

    ScreenChangeCallback on_screen_change_;
    RoomCreatedCallback on_room_created_;

    // Navigation & State
    static const char* get_map_name(MapId id);
    void next_step();
    void previous_step();
    void create_room();
    const char* get_step_title() const;

    // Initialization helpers
    void initialize_room_info_step();
    void initialize_map_selection_step();
    void initialize_difficulty_step();
    void initialize_game_mode_step();
    void initialize_navigation_buttons();

    // Update helpers
    void update_room_info_step(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void update_map_selection_step(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void update_difficulty_step(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);
    void update_game_mode_step(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input);

    // Draw helpers
    void draw_background(engine::IGraphicsPlugin* graphics);
    void draw_stepper(engine::IGraphicsPlugin* graphics);
    void draw_room_info_step(engine::IGraphicsPlugin* graphics);
    void draw_map_selection_step(engine::IGraphicsPlugin* graphics);
    void draw_difficulty_step(engine::IGraphicsPlugin* graphics);
    void draw_game_mode_step(engine::IGraphicsPlugin* graphics);
    void draw_navigation_buttons(engine::IGraphicsPlugin* graphics);

    // Edit mode for navigation arrow positioning
    bool edit_mode_ = false;  // Set to true to enable edit mode for arrows
    float move_speed_ = 5.0f;  // Pixels to move per key press
    float button_spacing_ = 250.0f;  // Distance from center
    float button_y_offset_ = 85.0f;  // Distance from bottom
    float button_radius_ = 24.0f;  // Arrow circle radius (refined size)
};

}  // namespace rtype::client
