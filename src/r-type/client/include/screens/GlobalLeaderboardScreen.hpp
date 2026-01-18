/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GlobalLeaderboardScreen - Displays all-time top 10 scores
*/

#pragma once

#include "BaseScreen.hpp"
#include "ScreenManager.hpp"
#include "protocol/Payloads.hpp"
#include <functional>
#include <vector>

namespace rtype::client {

/**
 * @brief Screen displaying the global all-time leaderboard (top 10)
 */
class GlobalLeaderboardScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;

    GlobalLeaderboardScreen(NetworkClient& network_client, int screen_width, int screen_height);

    void initialize() override;
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) override;
    void draw(engine::IGraphicsPlugin* graphics) override;
    void on_enter() override;

    void set_screen_change_callback(ScreenChangeCallback callback) {
        on_screen_change_ = callback;
    }

    /**
     * @brief Called when global leaderboard data is received from server
     */
    void on_leaderboard_received(const protocol::ServerGlobalLeaderboardPayload& header,
                                  const std::vector<protocol::GlobalLeaderboardEntry>& entries);

private:
    void rebuild_ui();

    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UIButton>> buttons_;
    ScreenChangeCallback on_screen_change_;
    engine::TextureHandle background_texture_ = engine::INVALID_HANDLE;
    bool background_loaded_ = false;

    // Leaderboard data
    std::vector<protocol::GlobalLeaderboardEntry> entries_;
    bool loading_ = true;
};

}  // namespace rtype::client
