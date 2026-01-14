#pragma once

#include "BaseScreen.hpp"
#include "ScreenManager.hpp"
#include "KeyBindings.hpp"
#include <functional>

namespace rtype::client {

/**
 * @brief Settings menu tabs
 */
enum class SettingsTab {
    AUDIO,
    CONTROLS,
    DEBUG
};

/**
 * @brief Settings screen
 */
class SettingsScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;

    SettingsScreen(NetworkClient& network_client, int screen_width, int screen_height);

    void initialize() override;
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) override;
    void draw(engine::IGraphicsPlugin* graphics) override;

    void set_screen_change_callback(ScreenChangeCallback callback) {
        on_screen_change_ = callback;
    }

    KeyBindings& get_key_bindings() { return key_bindings_; }

private:
    void update_volume_labels();
    void switch_tab(SettingsTab tab);
    void rebuild_ui();

    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UIButton>> buttons_;
    ScreenChangeCallback on_screen_change_;
    engine::TextureHandle background_texture_ = engine::INVALID_HANDLE;
    bool background_loaded_ = false;

    // Current tab
    SettingsTab current_tab_ = SettingsTab::AUDIO;

    // Audio settings
    int music_volume_ = 70;
    int sfx_volume_ = 80;
    UILabel* music_value_label_ = nullptr;
    UILabel* sfx_value_label_ = nullptr;

    // Control settings
    KeyBindings key_bindings_;
    bool waiting_for_key_ = false;
    GameAction waiting_action_;
    bool waiting_for_primary_ = true;
    UIButton* waiting_button_ = nullptr;

    void start_key_rebind(GameAction action, bool is_primary, UIButton* button);
    void cancel_key_rebind();
};

}  // namespace rtype::client