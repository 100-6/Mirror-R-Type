#pragma once

#include "BaseScreen.hpp"
#include "ScreenManager.hpp"
#include "protocol/Payloads.hpp"
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
    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UITextField>> fields_;
    std::vector<std::unique_ptr<UIButton>> buttons_;
    std::vector<std::unique_ptr<UIButton>> mode_buttons_;       // DUO, TRIO, SQUAD buttons
    std::vector<std::unique_ptr<UIButton>> map_buttons_;        // Map selection buttons
    std::vector<std::unique_ptr<UIButton>> difficulty_buttons_; // Difficulty selection buttons

    protocol::GameMode game_mode_ = protocol::GameMode::SQUAD;  // Default to SQUAD
    protocol::Difficulty difficulty_ = protocol::Difficulty::NORMAL;  // Default to NORMAL
    MapId map_id_ = MapId::NEBULA_OUTPOST;  // Default to first map

    ScreenChangeCallback on_screen_change_;
    RoomCreatedCallback on_room_created_;

    // Helper to get map name for display
    static const char* get_map_name(MapId id);
};

}  // namespace rtype::client
