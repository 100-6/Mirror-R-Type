/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** INetworkListener - Interface for network packet events
*/

#pragma once

#include <cstdint>
#include "protocol/Payloads.hpp"

namespace rtype::server {

/**
 * @brief Interface for receiving network packet events
 *
 * Implement this interface to handle incoming packets:
 * - Client connections/disconnections
 * - Lobby requests
 * - Player inputs
 * - UDP handshakes
 */
class INetworkListener {
public:
    virtual ~INetworkListener() = default;

    /**
     * @brief Called when a client sends CONNECTION request
     * @param client_id TCP client ID
     * @param payload Connection data (player name)
     */
    virtual void on_client_connect(uint32_t client_id, const protocol::ClientConnectPayload& payload) = 0;

    /**
     * @brief Called when a client sends DISCONNECT request
     * @param client_id TCP client ID
     * @param payload Disconnect data
     */
    virtual void on_client_disconnect(uint32_t client_id, const protocol::ClientDisconnectPayload& payload) = 0;

    /**
     * @brief Called when a client sends PING
     * @param client_id TCP client ID
     * @param payload Ping data (timestamp)
     */
    virtual void on_client_ping(uint32_t client_id, const protocol::ClientPingPayload& payload) = 0;

    /**
     * @brief Called when a client wants to join a lobby
     * @param client_id TCP client ID
     * @param payload Lobby join data (mode, difficulty)
     */
    virtual void on_client_join_lobby(uint32_t client_id, const protocol::ClientJoinLobbyPayload& payload) = 0;

    /**
     * @brief Called when a client wants to leave a lobby
     * @param client_id TCP client ID
     * @param payload Lobby leave data
     */
    virtual void on_client_leave_lobby(uint32_t client_id, const protocol::ClientLeaveLobbyPayload& payload) = 0;

    /**
     * @brief Called when a client wants to create a custom room
     * @param client_id TCP client ID
     * @param payload Room creation data
     */
    virtual void on_client_create_room(uint32_t client_id, const protocol::ClientCreateRoomPayload& payload) = 0;

    /**
     * @brief Called when a client wants to join a custom room
     * @param client_id TCP client ID
     * @param payload Room join data
     */
    virtual void on_client_join_room(uint32_t client_id, const protocol::ClientJoinRoomPayload& payload) = 0;

    /**
     * @brief Called when a client wants to leave a custom room
     * @param client_id TCP client ID
     * @param payload Room leave data
     */
    virtual void on_client_leave_room(uint32_t client_id, const protocol::ClientLeaveRoomPayload& payload) = 0;

    /**
     * @brief Called when a client requests the list of available rooms
     * @param client_id TCP client ID
     */
    virtual void on_client_request_room_list(uint32_t client_id) = 0;

    /**
     * @brief Called when a client (host) wants to start the game
     * @param client_id TCP client ID
     * @param payload Start game data
     */
    virtual void on_client_start_game(uint32_t client_id, const protocol::ClientStartGamePayload& payload) = 0;

    /**
     * @brief Called when a client sends UDP handshake
     * @param udp_client_id UDP client ID
     * @param payload Handshake data (player_id, session_id)
     */
    virtual void on_udp_handshake(uint32_t udp_client_id, const protocol::ClientUdpHandshakePayload& payload) = 0;

    /**
     * @brief Called when a client sends input
     * @param client_id Client ID (TCP or UDP)
     * @param payload Input data (movement, shooting)
     */
    virtual void on_client_input(uint32_t client_id, const protocol::ClientInputPayload& payload) = 0;

    /**
     * @brief Called when a client wants to change their player name
     * @param client_id TCP client ID
     * @param payload Name change data
     */
    virtual void on_client_set_player_name(uint32_t client_id, const protocol::ClientSetPlayerNamePayload& payload) = 0;

    /**
     * @brief Called when a client wants to change their player skin
     * @param client_id TCP client ID
     * @param payload Skin change data
     */
    virtual void on_client_set_player_skin(uint32_t client_id, const protocol::ClientSetPlayerSkinPayload& payload) = 0;

    /**
     * @brief Called when a client sends admin authentication
     * @param client_id TCP client ID
     * @param payload Admin auth data (password hash, username)
     */
    virtual void on_admin_auth(uint32_t client_id, const protocol::ClientAdminAuthPayload& payload) = 0;

    /**
     * @brief Called when a client sends admin command
     * @param client_id TCP client ID
     * @param payload Admin command data (command string)
     */
    virtual void on_admin_command(uint32_t client_id, const protocol::ClientAdminCommandPayload& payload) = 0;

    /**
     * @brief Called when a client requests the global leaderboard
     * @param client_id TCP client ID
     */
    virtual void on_client_request_global_leaderboard(uint32_t client_id) = 0;
};

}
