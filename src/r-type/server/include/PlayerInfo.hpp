#pragma once

#include <string>
#include <cstdint>
#include "ecs/Registry.hpp"
#include "Entity.hpp"

namespace rtype::server {

/**
 * @brief Information about a connected player
 */
struct PlayerInfo {
    uint32_t client_id;
    uint32_t player_id;
    std::string player_name;
    Entity entity;
    bool in_lobby;
    uint32_t lobby_id;
    bool in_game;
    uint32_t session_id;

    PlayerInfo()
        : client_id(0)
        , player_id(0)
        , entity(0)
        , in_lobby(false)
        , lobby_id(0)
        , in_game(false)
        , session_id(0) {}

    PlayerInfo(uint32_t cid, uint32_t pid, const std::string& name)
        : client_id(cid)
        , player_id(pid)
        , player_name(name)
        , entity(0)
        , in_lobby(false)
        , lobby_id(0)
        , in_game(false)
        , session_id(0) {}
};

}
