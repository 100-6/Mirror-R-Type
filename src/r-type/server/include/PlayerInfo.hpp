#pragma once

#include <string>
#include <cstdint>
#include "ecs/Registry.hpp"
#include "Entity.hpp"

namespace rtype::server {

/**
 * @brief Information about a connected player
 *
 * Network IDs:
 * - client_id: TCP client ID (assigned when TCP connects)
 * - udp_client_id: UDP client ID (assigned after UDP handshake)
 */
struct PlayerInfo {
    uint32_t client_id;      // TCP client ID
    uint32_t udp_client_id;  // UDP client ID (0 if not yet associated)
    uint32_t player_id;
    std::string player_name;
    uint8_t skin_id;         // Player skin (0-14: 3 colors x 5 ship types)
    Entity entity;
    bool in_lobby;
    uint32_t lobby_id;
    bool in_game;
    uint32_t session_id;

    PlayerInfo()
        : client_id(0)
        , udp_client_id(0)
        , player_id(0)
        , skin_id(0)
        , entity(0)
        , in_lobby(false)
        , lobby_id(0)
        , in_game(false)
        , session_id(0) {}

    PlayerInfo(uint32_t cid, uint32_t pid, const std::string& name)
        : client_id(cid)
        , udp_client_id(0)
        , player_id(pid)
        , player_name(name)
        , skin_id(0)
        , entity(0)
        , in_lobby(false)
        , lobby_id(0)
        , in_game(false)
        , session_id(0) {}

    bool has_udp_connection() const { return udp_client_id != 0; }
};

}
