#pragma once

#include <string>
#include <sstream>
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"

namespace rtype::client {

/**
 * @brief Manages the status overlay displayed on screen
 */
class StatusOverlay {
public:
    explicit StatusOverlay(Registry& registry, Entity status_entity);

    /**
     * @brief Update connection status
     */
    void set_connection(const std::string& status);

    /**
     * @brief Update lobby status
     */
    void set_lobby(const std::string& status);

    /**
     * @brief Update session/game status
     */
    void set_session(const std::string& status);

    /**
     * @brief Update ping in milliseconds
     */
    void set_ping(int ping_ms);

    /**
     * @brief Refresh the displayed overlay text
     */
    void refresh();

private:
    Registry& registry_;
    Entity status_entity_;

    std::string connection_;
    std::string lobby_;
    std::string session_;
    int ping_ms_;
};

}
