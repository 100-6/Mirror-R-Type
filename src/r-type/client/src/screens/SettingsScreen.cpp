#include "screens/SettingsScreen.hpp"
#include "NetworkClient.hpp"
#include "ScreenManager.hpp"
#include "AssetsPaths.hpp"
#include <iostream>

namespace rtype::client {

SettingsScreen::SettingsScreen(NetworkClient& network_client, int screen_width, int screen_height)
    : BaseScreen(network_client, screen_width, screen_height) {
}

void SettingsScreen::initialize() {
    current_tab_ = SettingsTab::AUDIO;
    rebuild_ui();
}

void SettingsScreen::set_initial_volumes(float master, float music, float sfx, float ambiance) {
    master_volume_ = static_cast<int>(master * 100.0f);
    music_volume_ = static_cast<int>(music * 100.0f);
    sfx_volume_ = static_cast<int>(sfx * 100.0f);
    ambiance_volume_ = static_cast<int>(ambiance * 100.0f);
    update_volume_labels();
}

void SettingsScreen::notify_audio_change() {
    if (on_audio_settings_change_) {
        AudioSettings settings;
        settings.master = master_volume_ / 100.0f;
        settings.music = music_volume_ / 100.0f;
        settings.sfx = sfx_volume_ / 100.0f;
        settings.ambiance = ambiance_volume_ / 100.0f;
        on_audio_settings_change_(settings);
    }
}

void SettingsScreen::rebuild_ui() {
    labels_.clear();
    buttons_.clear();
    master_value_label_ = nullptr;
    music_value_label_ = nullptr;
    sfx_value_label_ = nullptr;
    ambiance_value_label_ = nullptr;

    float center_x = screen_width_ / 2.0f;

    // Font sizes
    int title_size = 70;
    int label_size = 32;
    int button_text_size = 30;

    // Title label - centered
    auto title = std::make_unique<UILabel>(
        center_x, 80.0f, "SETTINGS", title_size);
    title->set_color(engine::Color{150, 100, 255, 255});
    title->set_alignment(UILabel::Alignment::CENTER);
    labels_.push_back(std::move(title));

    // Tab buttons
    float tab_y = 150.0f;
    float tab_width = 150.0f;
    float tab_height = 40.0f;
    float tab_spacing = 20.0f;
    
    // Calculate total width for 3 tabs
    float total_width = (tab_width * 3) + (tab_spacing * 2);
    float tabs_start_x = center_x - (total_width / 2.0f);

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
    
    auto display_tab = std::make_unique<UIButton>(
        tabs_start_x + (tab_width + tab_spacing) * 2, tab_y, tab_width, tab_height, "DISPLAY");
    display_tab->set_on_click([this]() {
        switch_tab(SettingsTab::DISPLAY);
    });
    display_tab->set_selected(current_tab_ == SettingsTab::DISPLAY);
    buttons_.push_back(std::move(display_tab));

    float content_start_y = 220.0f;

    // Display content based on current tab
    if (current_tab_ == SettingsTab::AUDIO) {
        // Audio settings section
        auto audio_title = std::make_unique<UILabel>(
            center_x, content_start_y, "AUDIO", 40);
        audio_title->set_color(engine::Color{200, 200, 200, 255});
        audio_title->set_alignment(UILabel::Alignment::CENTER);
        labels_.push_back(std::move(audio_title));

        float row_height = 65.0f;
        float label_x = center_x - 250.0f;
        float minus_x = center_x + 50.0f;
        float value_x = center_x + 125.0f;
        float plus_x = center_x + 200.0f;

        // Master Volume section
        float master_y = content_start_y + 60.0f;

        auto master_label = std::make_unique<UILabel>(
            label_x, master_y, "Master Volume", label_size);
        master_label->set_color(engine::Color{255, 220, 100, 255});  // Gold for master
        labels_.push_back(std::move(master_label));

        auto master_minus = std::make_unique<UIButton>(
            minus_x, master_y - 10.0f, 60.0f, 50.0f, "-");
        master_minus->set_on_click([this]() {
            if (master_volume_ > 0) {
                master_volume_ -= 10;
                if (master_volume_ < 0) master_volume_ = 0;
                update_volume_labels();
                notify_audio_change();
            }
        });
        buttons_.push_back(std::move(master_minus));

        auto master_value = std::make_unique<UILabel>(
            value_x, master_y, std::to_string(master_volume_) + "%", label_size);
        master_value->set_color(engine::Color{255, 220, 100, 255});
        master_value_label_ = master_value.get();
        labels_.push_back(std::move(master_value));

        auto master_plus = std::make_unique<UIButton>(
            plus_x, master_y - 10.0f, 60.0f, 50.0f, "+");
        master_plus->set_on_click([this]() {
            if (master_volume_ < 100) {
                master_volume_ += 10;
                if (master_volume_ > 100) master_volume_ = 100;
                update_volume_labels();
                notify_audio_change();
            }
        });
        buttons_.push_back(std::move(master_plus));

        // Music Volume section
        float music_y = master_y + row_height;

        auto music_label = std::make_unique<UILabel>(
            label_x, music_y, "Music Volume", label_size);
        music_label->set_color(engine::Color{255, 255, 255, 255});
        labels_.push_back(std::move(music_label));

        auto music_minus = std::make_unique<UIButton>(
            minus_x, music_y - 10.0f, 60.0f, 50.0f, "-");
        music_minus->set_on_click([this]() {
            if (music_volume_ > 0) {
                music_volume_ -= 10;
                if (music_volume_ < 0) music_volume_ = 0;
                update_volume_labels();
                notify_audio_change();
            }
        });
        buttons_.push_back(std::move(music_minus));

        auto music_value = std::make_unique<UILabel>(
            value_x, music_y, std::to_string(music_volume_) + "%", label_size);
        music_value->set_color(engine::Color{150, 255, 150, 255});
        music_value_label_ = music_value.get();
        labels_.push_back(std::move(music_value));

        auto music_plus = std::make_unique<UIButton>(
            plus_x, music_y - 10.0f, 60.0f, 50.0f, "+");
        music_plus->set_on_click([this]() {
            if (music_volume_ < 100) {
                music_volume_ += 10;
                if (music_volume_ > 100) music_volume_ = 100;
                update_volume_labels();
                notify_audio_change();
            }
        });
        buttons_.push_back(std::move(music_plus));

        // SFX Volume section
        float sfx_y = music_y + row_height;

        auto sfx_label = std::make_unique<UILabel>(
            label_x, sfx_y, "SFX Volume", label_size);
        sfx_label->set_color(engine::Color{255, 255, 255, 255});
        labels_.push_back(std::move(sfx_label));

        auto sfx_minus = std::make_unique<UIButton>(
            minus_x, sfx_y - 10.0f, 60.0f, 50.0f, "-");
        sfx_minus->set_on_click([this]() {
            if (sfx_volume_ > 0) {
                sfx_volume_ -= 10;
                if (sfx_volume_ < 0) sfx_volume_ = 0;
                update_volume_labels();
                notify_audio_change();
            }
        });
        buttons_.push_back(std::move(sfx_minus));

        auto sfx_value = std::make_unique<UILabel>(
            value_x, sfx_y, std::to_string(sfx_volume_) + "%", label_size);
        sfx_value->set_color(engine::Color{150, 255, 150, 255});
        sfx_value_label_ = sfx_value.get();
        labels_.push_back(std::move(sfx_value));

        auto sfx_plus = std::make_unique<UIButton>(
            plus_x, sfx_y - 10.0f, 60.0f, 50.0f, "+");
        sfx_plus->set_on_click([this]() {
            if (sfx_volume_ < 100) {
                sfx_volume_ += 10;
                if (sfx_volume_ > 100) sfx_volume_ = 100;
                update_volume_labels();
                notify_audio_change();
            }
        });
        buttons_.push_back(std::move(sfx_plus));

        // Ambiance Volume section
        float ambiance_y = sfx_y + row_height;

        auto ambiance_label = std::make_unique<UILabel>(
            label_x, ambiance_y, "Ambiance Volume", label_size);
        ambiance_label->set_color(engine::Color{255, 255, 255, 255});
        labels_.push_back(std::move(ambiance_label));

        auto ambiance_minus = std::make_unique<UIButton>(
            minus_x, ambiance_y - 10.0f, 60.0f, 50.0f, "-");
        ambiance_minus->set_on_click([this]() {
            if (ambiance_volume_ > 0) {
                ambiance_volume_ -= 10;
                if (ambiance_volume_ < 0) ambiance_volume_ = 0;
                update_volume_labels();
                notify_audio_change();
            }
        });
        buttons_.push_back(std::move(ambiance_minus));

        auto ambiance_value = std::make_unique<UILabel>(
            value_x, ambiance_y, std::to_string(ambiance_volume_) + "%", label_size);
        ambiance_value->set_color(engine::Color{150, 255, 150, 255});
        ambiance_value_label_ = ambiance_value.get();
        labels_.push_back(std::move(ambiance_value));

        auto ambiance_plus = std::make_unique<UIButton>(
            plus_x, ambiance_y - 10.0f, 60.0f, 50.0f, "+");
        ambiance_plus->set_on_click([this]() {
            if (ambiance_volume_ < 100) {
                ambiance_volume_ += 10;
                if (ambiance_volume_ > 100) ambiance_volume_ = 100;
                update_volume_labels();
                notify_audio_change();
            }
        });
        buttons_.push_back(std::move(ambiance_plus));
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
            // SWITCH_WEAPON removed - weapon is now determined by player level
            // GameAction::TOGGLE_HITBOX, // REmoved from menu
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
    } else if (current_tab_ == SettingsTab::DISPLAY) {
        // Display settings section
        auto display_title = std::make_unique<UILabel>(
            center_x, content_start_y, "DISPLAY", 40);
        display_title->set_color(engine::Color{200, 200, 200, 255});
        display_title->set_alignment(UILabel::Alignment::CENTER);
        labels_.push_back(std::move(display_title));

        float label_size = 32;
        float label_x = center_x - 200.0f;
        float minus_x = center_x + 50.0f;
        float value_x = center_x + 150.0f;
        float plus_x = center_x + 250.0f;

        // Colorblind Mode
        float cb_y = content_start_y + 80.0f;

        auto cb_label = std::make_unique<UILabel>(
            label_x, cb_y, "Colorblind Mode", label_size);
        cb_label->set_color(engine::Color{255, 255, 255, 255});
        labels_.push_back(std::move(cb_label));

        auto cb_minus = std::make_unique<UIButton>(
            minus_x, cb_y - 10.0f, 60.0f, 50.0f, "<");
        cb_minus->set_on_click([this]() {
            int mode = static_cast<int>(colorblind_mode_);
            mode = (mode - 1 + 4) % 4; // Cycle backwards
            colorblind_mode_ = static_cast<engine::ColorBlindMode>(mode);
            update_colorblind_label();
            colorblind_needs_update_ = true;
        });
        buttons_.push_back(std::move(cb_minus));

        auto cb_value = std::make_unique<UILabel>(
            value_x, cb_y, "", label_size); 
        cb_value->set_color(engine::Color{100, 255, 255, 255});
        cb_value->set_alignment(UILabel::Alignment::CENTER);
        colorblind_value_label_ = cb_value.get();
        labels_.push_back(std::move(cb_value));
        
        // Initial update
        update_colorblind_label();

        auto cb_plus = std::make_unique<UIButton>(
            plus_x, cb_y - 10.0f, 60.0f, 50.0f, ">");
        cb_plus->set_on_click([this]() {
            int mode = static_cast<int>(colorblind_mode_);
            mode = (mode + 1) % 4; // Cycle forwards
            colorblind_mode_ = static_cast<engine::ColorBlindMode>(mode);
            update_colorblind_label();
            colorblind_needs_update_ = true;
        });
        buttons_.push_back(std::move(cb_plus));
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

void SettingsScreen::update_colorblind_label() {
    if (!colorblind_value_label_) return;
    
    std::string text;
    switch (colorblind_mode_) {
        case engine::ColorBlindMode::None: text = "None"; break;
        case engine::ColorBlindMode::Protanopia: text = "Protanopia"; break;
        case engine::ColorBlindMode::Deuteranopia: text = "Deuteranopia"; break;
        case engine::ColorBlindMode::Tritanopia: text = "Tritanopia"; break;
    }
    colorblind_value_label_->set_text(text);
}

void SettingsScreen::update_volume_labels() {
    if (master_value_label_) {
        master_value_label_->set_text(std::to_string(master_volume_) + "%");
    }
    if (music_value_label_) {
        music_value_label_->set_text(std::to_string(music_volume_) + "%");
    }
    if (sfx_value_label_) {
        sfx_value_label_->set_text(std::to_string(sfx_volume_) + "%");
    }
    if (ambiance_value_label_) {
        ambiance_value_label_->set_text(std::to_string(ambiance_volume_) + "%");
    }
}

void SettingsScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    if (colorblind_needs_update_) {
        graphics->set_colorblind_mode(colorblind_mode_);
        colorblind_needs_update_ = false;
    }

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
