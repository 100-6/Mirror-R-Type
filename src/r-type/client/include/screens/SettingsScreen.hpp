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
    CONTROLS
};

/**
 * @brief Audio settings structure for callback
 */
struct AudioSettings {
    float master;    // 0.0 - 1.0
    float music;     // 0.0 - 1.0
    float sfx;       // 0.0 - 1.0
    float ambiance;  // 0.0 - 1.0
};

/**
 * @brief Settings screen
 */
class SettingsScreen : public BaseScreen {
public:
    using ScreenChangeCallback = std::function<void(GameScreen)>;
    using AudioSettingsCallback = std::function<void(const AudioSettings&)>;

    SettingsScreen(NetworkClient& network_client, int screen_width, int screen_height);

    void initialize() override;
    void update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) override;
    void draw(engine::IGraphicsPlugin* graphics) override;

    void set_screen_change_callback(ScreenChangeCallback callback) {
        on_screen_change_ = callback;
    }

    void set_audio_settings_callback(AudioSettingsCallback callback) {
        on_audio_settings_change_ = callback;
    }

    /**
     * @brief Set initial volume values from AudioSystem
     */
    void set_initial_volumes(float master, float music, float sfx, float ambiance);

    KeyBindings& get_key_bindings() { return key_bindings_; }

private:
    void update_volume_labels();
    void notify_audio_change();
    void switch_tab(SettingsTab tab);
    void rebuild_ui();

    std::vector<std::unique_ptr<UILabel>> labels_;
    std::vector<std::unique_ptr<UIButton>> buttons_;
    ScreenChangeCallback on_screen_change_;
    AudioSettingsCallback on_audio_settings_change_;
    engine::TextureHandle background_texture_ = engine::INVALID_HANDLE;
    bool background_loaded_ = false;

    // Current tab
    SettingsTab current_tab_ = SettingsTab::AUDIO;

    // Audio settings (0-100 for display, converted to 0.0-1.0 for AudioSystem)
    int master_volume_ = 100;
    int music_volume_ = 70;
    int sfx_volume_ = 100;
    int ambiance_volume_ = 50;
    UILabel* master_value_label_ = nullptr;
    UILabel* music_value_label_ = nullptr;
    UILabel* sfx_value_label_ = nullptr;
    UILabel* ambiance_value_label_ = nullptr;

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