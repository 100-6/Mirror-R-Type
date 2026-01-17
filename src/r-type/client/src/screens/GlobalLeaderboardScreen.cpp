/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** GlobalLeaderboardScreen implementation
*/

#include "screens/GlobalLeaderboardScreen.hpp"
#include "NetworkClient.hpp"
#include "AssetsPaths.hpp"
#include <iostream>
#include <cstring>

namespace rtype::client {

GlobalLeaderboardScreen::GlobalLeaderboardScreen(NetworkClient& network_client, int screen_width, int screen_height)
    : BaseScreen(network_client, screen_width, screen_height) {
}

void GlobalLeaderboardScreen::initialize() {
    loading_ = true;
    entries_.clear();
    rebuild_ui();
}

void GlobalLeaderboardScreen::on_enter() {
    loading_ = true;
    entries_.clear();
    rebuild_ui();
}

void GlobalLeaderboardScreen::on_leaderboard_received(const protocol::ServerGlobalLeaderboardPayload& header,
                                                       const std::vector<protocol::GlobalLeaderboardEntry>& entries) {
    entries_ = entries;
    loading_ = false;
    rebuild_ui();
}

void GlobalLeaderboardScreen::rebuild_ui() {
    labels_.clear();
    buttons_.clear();

    float center_x = screen_width_ / 2.0f;

    // Font sizes
    int title_size = 60;
    int header_size = 28;
    int entry_size = 24;

    // Title label - centered
    auto title = std::make_unique<UILabel>(
        center_x, 60.0f, "GLOBAL LEADERBOARD", title_size);
    title->set_color(engine::Color{255, 215, 0, 255});  // Gold color
    title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(title));

    // Subtitle
    auto subtitle = std::make_unique<UILabel>(
        center_x, 115.0f, "All-Time Top 10", 24);
    subtitle->set_color(engine::Color{200, 200, 200, 255});
    subtitle->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(subtitle));

    float content_start_y = 170.0f;

    if (loading_) {
        // Show loading message
        auto loading_label = std::make_unique<UILabel>(
            center_x, content_start_y + 100.0f, "Loading...", 36);
        loading_label->set_color(engine::Color{150, 150, 255, 255});
        loading_label->set_alignment(UILabel::Alignment::CENTER);
        labels_.push_back(std::move(loading_label));
    } else if (entries_.empty()) {
        // Show empty message
        auto empty_label = std::make_unique<UILabel>(
            center_x, content_start_y + 100.0f, "No scores yet. Be the first!", 30);
        empty_label->set_color(engine::Color{180, 180, 180, 255});
        empty_label->set_alignment(UILabel::Alignment::CENTER);
        labels_.push_back(std::move(empty_label));
    } else {
        // Column headers
        float rank_x = center_x - 280.0f;
        float name_x = center_x - 150.0f;
        float score_x = center_x + 150.0f;

        auto rank_header = std::make_unique<UILabel>(
            rank_x, content_start_y, "RANK", header_size);
        rank_header->set_color(engine::Color{150, 150, 150, 255});
        labels_.push_back(std::move(rank_header));

        auto name_header = std::make_unique<UILabel>(
            name_x, content_start_y, "PLAYER", header_size);
        name_header->set_color(engine::Color{150, 150, 150, 255});
        labels_.push_back(std::move(name_header));

        auto score_header = std::make_unique<UILabel>(
            score_x, content_start_y, "SCORE", header_size);
        score_header->set_color(engine::Color{150, 150, 150, 255});
        labels_.push_back(std::move(score_header));

        // Display entries
        float row_y = content_start_y + 45.0f;
        float row_spacing = 40.0f;

        for (size_t i = 0; i < entries_.size() && i < 10; ++i) {
            const auto& entry = entries_[i];

            // Determine rank color
            engine::Color rank_color;
            std::string rank_text;
            if (i == 0) {
                rank_color = {255, 215, 0, 255};   // Gold
                rank_text = "1st";
            } else if (i == 1) {
                rank_color = {192, 192, 192, 255}; // Silver
                rank_text = "2nd";
            } else if (i == 2) {
                rank_color = {205, 127, 50, 255};  // Bronze
                rank_text = "3rd";
            } else {
                rank_color = {255, 255, 255, 255}; // White
                rank_text = std::to_string(i + 1) + "th";
            }

            // Rank label
            auto rank_label = std::make_unique<UILabel>(
                rank_x, row_y, rank_text, entry_size);
            rank_label->set_color(rank_color);
            labels_.push_back(std::move(rank_label));

            // Player name
            std::string player_name(entry.player_name, strnlen(entry.player_name, sizeof(entry.player_name)));
            auto name_label = std::make_unique<UILabel>(
                name_x, row_y, player_name, entry_size);
            name_label->set_color(engine::Color{255, 255, 255, 255});
            labels_.push_back(std::move(name_label));

            // Score
            auto score_label = std::make_unique<UILabel>(
                score_x, row_y, std::to_string(entry.score), entry_size);
            score_label->set_color(engine::Color{100, 255, 100, 255});  // Green for score
            labels_.push_back(std::move(score_label));

            row_y += row_spacing;
        }
    }

    // Back button - centered at bottom
    float button_width = 200.0f;
    float button_height = 50.0f;
    float button_x = center_x - button_width / 2.0f;
    float button_y = screen_height_ - 100.0f;

    auto back_btn = std::make_unique<UIButton>(
        button_x, button_y, button_width, button_height, "Back");
    back_btn->set_on_click([this]() {
        if (on_screen_change_) {
            on_screen_change_(GameScreen::MAIN_MENU);
        }
    });
    buttons_.push_back(std::move(back_btn));
}

void GlobalLeaderboardScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    if (!input) {
        return;
    }

    for (auto& button : buttons_) {
        if (button) {
            button->update(graphics, input);
        }
    }
}

void GlobalLeaderboardScreen::draw(engine::IGraphicsPlugin* graphics) {
    // Load background texture on first draw
    if (!background_loaded_) {
        background_texture_ = graphics->load_texture(assets::paths::UI_BACKGROUNDV1);
        background_loaded_ = true;
    }

    // Clear with dark background as fallback
    graphics->clear(engine::Color{15, 15, 25, 255});

    // Draw background image stretched to fill screen
    engine::Sprite background_sprite;
    background_sprite.texture_handle = background_texture_;
    background_sprite.size = {static_cast<float>(screen_width_), static_cast<float>(screen_height_)};
    background_sprite.origin = {0.0f, 0.0f};
    background_sprite.tint = {255, 255, 255, 255};

    graphics->draw_sprite(background_sprite, {0.0f, 0.0f});

    // Draw semi-transparent panel behind leaderboard content
    float panel_width = 600.0f;
    float panel_height = 500.0f;
    float panel_x = (screen_width_ - panel_width) / 2.0f;
    float panel_y = 40.0f;

    engine::Rectangle panel_rect{panel_x, panel_y, panel_width, panel_height};
    graphics->draw_rectangle(panel_rect, engine::Color{0, 0, 0, 180});  // Dark semi-transparent

    // Draw a subtle border around the panel
    graphics->draw_rectangle_outline(panel_rect, engine::Color{100, 100, 150, 200}, 2.0f);

    // Draw labels
    for (auto& label : labels_) {
        if (label) {
            label->draw(graphics);
        }
    }

    // Draw buttons
    for (auto& button : buttons_) {
        if (button) {
            button->draw(graphics);
        }
    }
}

}  // namespace rtype::client
