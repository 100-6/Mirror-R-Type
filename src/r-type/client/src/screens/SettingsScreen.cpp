#include "screens/SettingsScreen.hpp"
#include "NetworkClient.hpp"
#include "ScreenManager.hpp"
#include "AssetsPaths.hpp"
#include "DebugSettings.hpp"
#include <iostream>

namespace rtype::client {

SettingsScreen::SettingsScreen(NetworkClient& network_client, int screen_width, int screen_height)
    : BaseScreen(network_client, screen_width, screen_height) {
}

void SettingsScreen::initialize() {
    current_tab_ = SettingsTab::AUDIO;
    rebuild_ui();
}

void SettingsScreen::rebuild_ui() {
    labels_.clear();
    buttons_.clear();
    music_value_label_ = nullptr;
    sfx_value_label_ = nullptr;

    float center_x = screen_width_ / 2.0f;

    // Font sizes
    int title_size = 70;
    int label_size = 35;
    int button_text_size = 30;

    // Title label - centered
    auto title = std::make_unique<UILabel>(
        center_x, 80.0f, "SETTINGS", title_size);
    title->set_color(engine::Color{150, 100, 255, 255});
    title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(title));

    // Tab buttons (3 tabs: AUDIO, CONTROLS, DEBUG)
    float tab_y = 150.0f;
    float tab_width = 150.0f;
    float tab_height = 40.0f;
    float tab_spacing = 20.0f;
    float tabs_total_width = tab_width * 3.0f + tab_spacing * 2.0f;
    float tabs_start_x = center_x - tabs_total_width / 2.0f;

    auto audio_tab = std::make_unique<UIButton>(
        tabs_start_x, tab_y, tab_width, tab_height, "AUDIO");
    audio_tab->set_on_click([this]() {
        switch_tab(SettingsTab::AUDIO);
    });
    audio_tab->set_selected(current_tab_ == SettingsTab::AUDIO);
    buttons_.push_back(std::move(audio_tab));

    auto controls_tab = std::make_unique<UIButton>(
        tabs_start_x + tab_width + tab_spacing, tab_y, tab_width, tab_height, "CONTROLS");
    controls_tab->set_on_click([this]() {
        switch_tab(SettingsTab::CONTROLS);
    });
    controls_tab->set_selected(current_tab_ == SettingsTab::CONTROLS);
    buttons_.push_back(std::move(controls_tab));

    auto debug_tab = std::make_unique<UIButton>(
        tabs_start_x + (tab_width + tab_spacing) * 2.0f, tab_y, tab_width, tab_height, "DEBUG");
    debug_tab->set_on_click([this]() {
        switch_tab(SettingsTab::DEBUG);
    });
    debug_tab->set_selected(current_tab_ == SettingsTab::DEBUG);
    buttons_.push_back(std::move(debug_tab));

    float content_start_y = 220.0f;

    // Display content based on current tab
    if (current_tab_ == SettingsTab::AUDIO) {
        // Audio settings section
        auto audio_title = std::make_unique<UILabel>(
            center_x, content_start_y, "AUDIO", 40);
        audio_title->set_color(engine::Color{200, 200, 200, 255});
        audio_title->set_alignment(UILabel::Alignment::CENTER);
        labels_.push_back(std::move(audio_title));

        // Music Volume section
        float music_y = content_start_y + 60.0f;

        auto music_label = std::make_unique<UILabel>(
            center_x - 250.0f, music_y, "Music Volume", label_size);
        music_label->set_color(engine::Color{255, 255, 255, 255});
        labels_.push_back(std::move(music_label));

        auto music_minus = std::make_unique<UIButton>(
            center_x + 50.0f, music_y - 10.0f, 60.0f, 50.0f, "-");
        music_minus->set_on_click([this]() {
            if (music_volume_ > 0) {
                music_volume_ -= 10;
                if (music_volume_ < 0) music_volume_ = 0;
                update_volume_labels();
            }
        });
        buttons_.push_back(std::move(music_minus));

        auto music_value = std::make_unique<UILabel>(
            center_x + 125.0f, music_y, std::to_string(music_volume_) + "%", label_size);
        music_value->set_color(engine::Color{150, 255, 150, 255});
        music_value_label_ = music_value.get();
        labels_.push_back(std::move(music_value));

        auto music_plus = std::make_unique<UIButton>(
            center_x + 200.0f, music_y - 10.0f, 60.0f, 50.0f, "+");
        music_plus->set_on_click([this]() {
            if (music_volume_ < 100) {
                music_volume_ += 10;
                if (music_volume_ > 100) music_volume_ = 100;
                update_volume_labels();
            }
        });
        buttons_.push_back(std::move(music_plus));

        // SFX Volume section
        float sfx_y = music_y + 80.0f;

        auto sfx_label = std::make_unique<UILabel>(
            center_x - 250.0f, sfx_y, "SFX Volume", label_size);
        sfx_label->set_color(engine::Color{255, 255, 255, 255});
        labels_.push_back(std::move(sfx_label));

        auto sfx_minus = std::make_unique<UIButton>(
            center_x + 50.0f, sfx_y - 10.0f, 60.0f, 50.0f, "-");
        sfx_minus->set_on_click([this]() {
            if (sfx_volume_ > 0) {
                sfx_volume_ -= 10;
                if (sfx_volume_ < 0) sfx_volume_ = 0;
                update_volume_labels();
            }
        });
        buttons_.push_back(std::move(sfx_minus));

        auto sfx_value = std::make_unique<UILabel>(
            center_x + 125.0f, sfx_y, std::to_string(sfx_volume_) + "%", label_size);
        sfx_value->set_color(engine::Color{150, 255, 150, 255});
        sfx_value_label_ = sfx_value.get();
        labels_.push_back(std::move(sfx_value));

        auto sfx_plus = std::make_unique<UIButton>(
            center_x + 200.0f, sfx_y - 10.0f, 60.0f, 50.0f, "+");
        sfx_plus->set_on_click([this]() {
            if (sfx_volume_ < 100) {
                sfx_volume_ += 10;
                if (sfx_volume_ > 100) sfx_volume_ = 100;
                update_volume_labels();
            }
        });
        buttons_.push_back(std::move(sfx_plus));
    } else if (current_tab_ == SettingsTab::CONTROLS) {
        // Controls settings section
        std::cout << "[SettingsScreen] Building CONTROLS tab UI" << std::endl;

        auto controls_title = std::make_unique<UILabel>(
            center_x, content_start_y, "KEY BINDINGS", 40);
        controls_title->set_color(engine::Color{200, 200, 200, 255});
        controls_title->set_alignment(UILabel::Alignment::CENTER);
        labels_.push_back(std::move(controls_title));

        std::cout << "[SettingsScreen] Added title label" << std::endl;

        // List of all game actions
        std::vector<GameAction> actions = {
            GameAction::MOVE_UP,
            GameAction::MOVE_DOWN,
            GameAction::MOVE_LEFT,
            GameAction::MOVE_RIGHT,
            GameAction::SHOOT,
            GameAction::CHARGE,
            GameAction::SPECIAL,
            GameAction::SWITCH_WEAPON,
            GameAction::TOGGLE_HITBOX,
            GameAction::TOGGLE_NETWORK_DEBUG
        };

        float key_y = content_start_y + 50.0f;
        int small_label_size = 25;

        std::cout << "[SettingsScreen] Starting action loop with " << actions.size() << " actions" << std::endl;

        for (size_t i = 0; i < actions.size(); ++i) {
            GameAction action = actions[i];

            std::cout << "[SettingsScreen] Processing action " << i << std::endl;

            // Get action name safely
            std::string action_name = KeyBindings::get_action_name(action);
            std::cout << "[SettingsScreen] Action name: " << action_name << std::endl;

            // Action name label
            auto action_label = std::make_unique<UILabel>(
                center_x - 300.0f, key_y, action_name, small_label_size);
            action_label->set_color(engine::Color{255, 255, 255, 255});
            labels_.push_back(std::move(action_label));

            std::cout << "[SettingsScreen] Added action label" << std::endl;

            // Get key names safely
            engine::Key primary_key = key_bindings_.get_key(action);
            std::string primary_key_name = KeyBindings::get_key_name(primary_key);
            std::cout << "[SettingsScreen] Primary key: " << primary_key_name << std::endl;

            // Primary key button
            auto primary_key_btn = std::make_unique<UIButton>(
                center_x - 50.0f, key_y - 10.0f, 120.0f, 40.0f,
                primary_key_name);
            UIButton* primary_btn_ptr = primary_key_btn.get();
            primary_key_btn->set_on_click([this, action, primary_btn_ptr]() {
                start_key_rebind(action, true, primary_btn_ptr);
            });
            buttons_.push_back(std::move(primary_key_btn));

            std::cout << "[SettingsScreen] Added primary button" << std::endl;

            // Get alt key name safely
            engine::Key alt_key = key_bindings_.get_alt_key(action);
            std::string alt_key_name = KeyBindings::get_key_name(alt_key);
            std::cout << "[SettingsScreen] Alt key: " << alt_key_name << std::endl;

            // Secondary key button
            auto alt_key_btn = std::make_unique<UIButton>(
                center_x + 90.0f, key_y - 10.0f, 120.0f, 40.0f,
                alt_key_name);
            UIButton* alt_btn_ptr = alt_key_btn.get();
            alt_key_btn->set_on_click([this, action, alt_btn_ptr]() {
                start_key_rebind(action, false, alt_btn_ptr);
            });
            buttons_.push_back(std::move(alt_key_btn));

            std::cout << "[SettingsScreen] Added alt button" << std::endl;

            key_y += 50.0f;
        }

        std::cout << "[SettingsScreen] Finished action loop" << std::endl;

        // Reset to defaults button
        float reset_button_width = 250.0f;
        float reset_button_height = 50.0f;
        auto reset_btn = std::make_unique<UIButton>(
            center_x - reset_button_width / 2.0f, key_y + 10.0f,
            reset_button_width, reset_button_height, "Reset to Defaults");
        reset_btn->set_on_click([this]() {
            key_bindings_.reset_to_defaults();
            rebuild_ui();
        });
        buttons_.push_back(std::move(reset_btn));

        std::cout << "[SettingsScreen] CONTROLS tab UI complete" << std::endl;
    } else if (current_tab_ == SettingsTab::DEBUG) {
        // Debug settings section
        auto debug_title = std::make_unique<UILabel>(
            center_x, content_start_y, "DEBUG OPTIONS", 40);
        debug_title->set_color(engine::Color{200, 200, 200, 255});
        debug_title->set_alignment(UILabel::Alignment::CENTER);
        labels_.push_back(std::move(debug_title));

        // Hitbox visualization toggle section
        float debug_y = content_start_y + 80.0f;

        auto debug_label = std::make_unique<UILabel>(
            center_x - 250.0f, debug_y, "Hitbox Visualization", label_size);
        debug_label->set_color(engine::Color{255, 255, 255, 255});
        labels_.push_back(std::move(debug_label));

        // Get current state from DebugSettings singleton
        bool current_debug_state = DebugSettings::instance().is_hitbox_debug_enabled();

        // Toggle button for hitbox debug
        auto debug_toggle = std::make_unique<UIButton>(
            center_x + 100.0f, debug_y - 10.0f, 150.0f, 50.0f,
            current_debug_state ? "ENABLED" : "DISABLED");
        debug_toggle->set_selected(current_debug_state);

        // Store pointer before moving to capture it in lambda
        UIButton* debug_toggle_ptr = debug_toggle.get();

        debug_toggle->set_on_click([this, debug_toggle_ptr]() {
            // Toggle debug mode via DebugSettings singleton
            bool new_state = DebugSettings::instance().toggle_hitbox_debug();

            std::cout << "[SettingsScreen] Hitbox debug toggled to: "
                      << (new_state ? "ENABLED" : "DISABLED") << std::endl;

            // Update button text and selected state immediately
            debug_toggle_ptr->set_text(new_state ? "ENABLED" : "DISABLED");
            debug_toggle_ptr->set_selected(new_state);
        });
        buttons_.push_back(std::move(debug_toggle));

        // Info label about debug mode
        auto info_label = std::make_unique<UILabel>(
            center_x, debug_y + 70.0f,
            "Press H in-game to toggle hitbox display",
            20);
        info_label->set_color(engine::Color{180, 180, 180, 255});
        info_label->set_alignment(UILabel::Alignment::CENTER);
        labels_.push_back(std::move(info_label));

        std::cout << "[SettingsScreen] DEBUG tab UI complete" << std::endl;
    }

    // Back button - centered at bottom
    float button_width = 300.0f;
    float button_height = 60.0f;
    float button_x = center_x - button_width / 2.0f;
    float button_y = screen_height_ - 120.0f;

    auto back_btn = std::make_unique<UIButton>(
        button_x, button_y, button_width, button_height, "Back to Menu");
    back_btn->set_on_click([this]() {
        if (on_screen_change_) {
            on_screen_change_(GameScreen::MAIN_MENU);
        }
    });
    buttons_.push_back(std::move(back_btn));
}

void SettingsScreen::switch_tab(SettingsTab tab) {
    std::cout << "[SettingsScreen] switch_tab called, new tab: " << (int)tab << std::endl;
    if (current_tab_ != tab) {
        std::cout << "[SettingsScreen] Tab changed, rebuilding UI" << std::endl;
        current_tab_ = tab;
        rebuild_ui();
        std::cout << "[SettingsScreen] UI rebuild complete" << std::endl;
    }
}

void SettingsScreen::update_volume_labels() {
    if (music_value_label_) {
        music_value_label_->set_text(std::to_string(music_volume_) + "%");
    }
    if (sfx_value_label_) {
        sfx_value_label_->set_text(std::to_string(sfx_volume_) + "%");
    }
}

void SettingsScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    if (!input) {
        return;
    }

    // If waiting for key rebind, check for any key press
    if (waiting_for_key_) {
        // Check if Escape is pressed to cancel
        if (input->is_key_pressed(engine::Key::Escape)) {
            cancel_key_rebind();
            return;
        }

        // Check all possible keys
        for (int key_int = static_cast<int>(engine::Key::A); key_int <= static_cast<int>(engine::Key::F12); ++key_int) {
            engine::Key key = static_cast<engine::Key>(key_int);
            if (input->is_key_pressed(key)) {
                // Assign the new key
                if (waiting_for_primary_) {
                    key_bindings_.set_key(waiting_action_, key);
                } else {
                    key_bindings_.set_alt_key(waiting_action_, key);
                }

                // Update button text
                if (waiting_button_) {
                    waiting_button_->set_text(KeyBindings::get_key_name(key));
                }

                // Stop waiting
                cancel_key_rebind();
                return;
            }
        }
        return;  // Don't process buttons while waiting for key
    }

    // Normal button updates
    std::cout << "[SettingsScreen] Updating " << buttons_.size() << " buttons" << std::endl;
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (buttons_[i]) {
            buttons_[i]->update(graphics, input);
        } else {
            std::cout << "[SettingsScreen] ERROR: Null button at index " << i << std::endl;
        }
    }
    std::cout << "[SettingsScreen] Update complete" << std::endl;
}

void SettingsScreen::start_key_rebind(GameAction action, bool is_primary, UIButton* button) {
    waiting_for_key_ = true;
    waiting_action_ = action;
    waiting_for_primary_ = is_primary;
    waiting_button_ = button;

    // Update button text to show waiting state
    if (button) {
        button->set_text("Press key...");
        button->set_selected(true);
    }

    std::cout << "[SettingsScreen] Waiting for key rebind for action " << (int)action
              << " (primary=" << is_primary << ")" << std::endl;
}

void SettingsScreen::cancel_key_rebind() {
    if (waiting_button_) {
        // Restore original key name
        engine::Key current_key = waiting_for_primary_
            ? key_bindings_.get_key(waiting_action_)
            : key_bindings_.get_alt_key(waiting_action_);
        waiting_button_->set_text(KeyBindings::get_key_name(current_key));
        waiting_button_->set_selected(false);
    }

    waiting_for_key_ = false;
    waiting_button_ = nullptr;

    std::cout << "[SettingsScreen] Key rebind cancelled" << std::endl;
}

void SettingsScreen::draw(engine::IGraphicsPlugin* graphics) {
    static int draw_count = 0;
    draw_count++;

    if (draw_count % 60 == 0) {  // Log every 60 frames
        std::cout << "[SettingsScreen] Draw called, frame " << draw_count << std::endl;
    }

    // Load background texture on first draw
    if (!background_loaded_) {
        std::cout << "[SettingsScreen] Loading background texture" << std::endl;
        background_texture_ = graphics->load_texture(assets::paths::UI_BACKGROUNDV1);
        background_loaded_ = true;
        std::cout << "[SettingsScreen] Background texture loaded" << std::endl;
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

    if (draw_count % 60 == 0) {
        std::cout << "[SettingsScreen] Drawing " << labels_.size() << " labels" << std::endl;
    }

    // Draw labels
    for (size_t i = 0; i < labels_.size(); ++i) {
        if (labels_[i]) {
            labels_[i]->draw(graphics);
        } else {
            std::cout << "[SettingsScreen] ERROR: Null label at index " << i << std::endl;
        }
    }

    if (draw_count % 60 == 0) {
        std::cout << "[SettingsScreen] Drawing " << buttons_.size() << " buttons" << std::endl;
    }

    // Draw buttons
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (buttons_[i]) {
            buttons_[i]->draw(graphics);
        } else {
            std::cout << "[SettingsScreen] ERROR: Null button at index " << i << std::endl;
        }
    }

    if (draw_count % 60 == 0) {
        std::cout << "[SettingsScreen] Draw complete" << std::endl;
    }
}

}  // namespace rtype::client
