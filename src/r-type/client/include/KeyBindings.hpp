#pragma once

#include "plugin_manager/IInputPlugin.hpp"
#include <unordered_map>
#include <string>

namespace rtype::client {

/**
 * @brief Action types for key bindings
 */
enum class GameAction {
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    SHOOT,
    CHARGE,
    SPECIAL,
    // SWITCH_WEAPON removed - weapon is now determined by player level
    TOGGLE_HITBOX,
    TOGGLE_NETWORK_DEBUG,
    SHOW_SCOREBOARD
};

/**
 * @brief Manages customizable key bindings
 */
class KeyBindings {
public:
    KeyBindings();

    /**
     * @brief Get the primary key for an action
     */
    engine::Key get_key(GameAction action) const;

    /**
     * @brief Get the secondary (alternative) key for an action
     */
    engine::Key get_alt_key(GameAction action) const;

    /**
     * @brief Set a new key binding for an action
     */
    void set_key(GameAction action, engine::Key key);

    /**
     * @brief Set a secondary key binding for an action
     */
    void set_alt_key(GameAction action, engine::Key key);

    /**
     * @brief Reset to default key bindings
     */
    void reset_to_defaults();

    /**
     * @brief Get human-readable name for a key
     */
    static std::string get_key_name(engine::Key key);

    /**
     * @brief Get human-readable name for an action
     */
    static std::string get_action_name(GameAction action);

    /**
     * @brief Load key bindings from config file
     */
    bool load_from_file(const std::string& filepath);

    /**
     * @brief Save key bindings to config file
     */
    bool save_to_file(const std::string& filepath) const;

private:
    std::unordered_map<GameAction, engine::Key> primary_bindings_;
    std::unordered_map<GameAction, engine::Key> alt_bindings_;

    void init_defaults();
};

}
