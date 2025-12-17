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

    // === Connection Events ===

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

    // === Lobby Events ===

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

    // === Game Events ===

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
};

}
