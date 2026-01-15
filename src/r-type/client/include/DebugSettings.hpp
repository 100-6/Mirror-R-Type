#pragma once

#include <functional>

namespace rtype::client {

/**
 * @brief Singleton class to manage debug settings
 *
 * This class provides a centralized way to manage debug features
 * such as hitbox visualization and other debug tools.
 */
class DebugSettings {
public:
    using DebugStateChangeCallback = std::function<void(bool)>;

    /**
     * @brief Get the singleton instance
     */
    static DebugSettings& instance() {
        static DebugSettings instance;
        return instance;
    }

    // Delete copy constructor and assignment operator
    DebugSettings(const DebugSettings&) = delete;
    DebugSettings& operator=(const DebugSettings&) = delete;

    /**
     * @brief Check if hitbox debug mode is enabled
     */
    bool is_hitbox_debug_enabled() const {
        return hitbox_debug_enabled_;
    }

    /**
     * @brief Enable or disable hitbox debug mode
     */
    void set_hitbox_debug_enabled(bool enabled) {
        if (hitbox_debug_enabled_ != enabled) {
            hitbox_debug_enabled_ = enabled;
            notify_callback(enabled);
        }
    }

    /**
     * @brief Toggle hitbox debug mode
     * @return New state after toggle
     */
    bool toggle_hitbox_debug() {
        hitbox_debug_enabled_ = !hitbox_debug_enabled_;
        notify_callback(hitbox_debug_enabled_);
        return hitbox_debug_enabled_;
    }

    /**
     * @brief Set callback for when debug state changes
     */
    void set_state_change_callback(DebugStateChangeCallback callback) {
        callback_ = callback;
    }

private:
    DebugSettings() : hitbox_debug_enabled_(false), callback_(nullptr) {}

    void notify_callback(bool new_state) {
        if (callback_) {
            callback_(new_state);
        }
    }

    bool hitbox_debug_enabled_;
    DebugStateChangeCallback callback_;
};

}  // namespace rtype::client