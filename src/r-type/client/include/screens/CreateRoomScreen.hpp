#pragma once

#include "BaseScreen.hpp"
#include "ScreenManager.hpp"
#include "protocol/Payloads.hpp"
#include "components/MapTypes.hpp"
#include "screens/createroom/CreateRoomRenderer.hpp"
#include <functional>
#include <vector>

namespace rtype::client {

/**
 * @brief Screen for creating a custom room with name, password, game mode, map, and difficulty
 */
class CreateRoomScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;
    using RoomCreatedCallback = std::function<void(protocol::GameMode, uint8_t)>;

    CreateRoomScreen(NetworkClient& network_client, int screen_width, int screen_height);

    void initialize() override;
    void on_enter() override;
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
    std::string get_configured_map_id() const { return selected_map_id_; }
    uint16_t get_configured_map_index() const { return selected_map_index_ + 1; }  // 1-based for protocol (if using index)
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
    std::vector<std::unique_ptr<UIButton>> buttons_; // Legacy?
    std::vector<std::unique_ptr<UIButton>> mode_buttons_;       // DUO, TRIO, SQUAD buttons (Legacy?)
    std::vector<std::unique_ptr<UIButton>> map_buttons_;        // Map selection buttons (dynamic)
    std::vector<std::unique_ptr<UIButton>> difficulty_buttons_; // Difficulty selection buttons (Legacy?)
    std::vector<std::unique_ptr<UIButton>> nav_buttons_;        // Next, Previous, Create buttons

    // Texture pack (handled by createroom module)
    createroom::TexturePack textures_;

    // Dynamic map list from index.json
    std::vector<rtype::MapInfo> available_maps_;
    std::vector<engine::TextureHandle> map_thumbnails_;  // Dynamically loaded thumbnails
    std::string selected_map_id_ = "nebula_outpost";
    size_t selected_map_index_ = 0;
    // MapId map_id_; // Removed in favor of dynamic map selection

    protocol::GameMode game_mode_ = protocol::GameMode::SQUAD;  // Default to SQUAD
    protocol::Difficulty difficulty_ = protocol::Difficulty::NORMAL;  // Default to NORMAL

    ScreenChangeCallback on_screen_change_;
    RoomCreatedCallback on_room_created_;

    // Navigation & State
    const char* get_step_title() const;
    void next_step();
    void previous_step();
    void create_room();

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
