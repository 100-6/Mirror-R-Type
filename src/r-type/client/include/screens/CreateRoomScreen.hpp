#pragma once

#include "BaseScreen.hpp"
#include "ScreenManager.hpp"
#include "protocol/Payloads.hpp"
#include "components/MapTypes.hpp"
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
    uint16_t get_configured_map_index() const { return selected_map_index_ + 1; }  // 1-based for protocol
    uint8_t get_configured_max_players() const;

private:
    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UITextField>> fields_;
    std::vector<std::unique_ptr<UIButton>> buttons_;
    std::vector<std::unique_ptr<UIButton>> mode_buttons_;       // DUO, TRIO, SQUAD buttons
    std::vector<std::unique_ptr<UIButton>> map_buttons_;        // Map selection buttons (dynamic)
    std::vector<std::unique_ptr<UIButton>> difficulty_buttons_; // Difficulty selection buttons

    // Dynamic map list from index.json
    std::vector<rtype::MapInfo> available_maps_;
    std::string selected_map_id_ = "nebula_outpost";
    size_t selected_map_index_ = 0;

    protocol::GameMode game_mode_ = protocol::GameMode::SQUAD;  // Default to SQUAD
    protocol::Difficulty difficulty_ = protocol::Difficulty::NORMAL;  // Default to NORMAL

    ScreenChangeCallback on_screen_change_;
    RoomCreatedCallback on_room_created_;
};

}  // namespace rtype::client

