#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include "protocol/PacketHeader.hpp"
#include "protocol/PacketTypes.hpp"
#include "plugin_manager/INetworkPlugin.hpp"

namespace rtype::server {

// Forward declarations
class LobbyManager;
struct PlayerInfo;

/**
 * @brief Handles sending packets via TCP and UDP
 *
 * This class encapsulates all packet sending logic:
 * - TCP packet sending (reliable, ordered)
 * - UDP packet sending (fast, unreliable)
 * - Broadcasting to lobbies and game sessions
 */
class PacketSender {
public:
    /**
     * @brief Construct a new PacketSender
     * @param network_plugin Pointer to the network plugin for sending packets
     */
    explicit PacketSender(engine::INetworkPlugin* network_plugin);

    // ============== TCP Sending ==============

    /**
     * @brief Send a TCP packet to a specific client
     * @param client_id Target client ID
     * @param type Packet type
     * @param payload Packet payload data
     */
    void send_tcp_packet(uint32_t client_id, protocol::PacketType type,
                        const std::vector<uint8_t>& payload);

    /**
     * @brief Broadcast a TCP packet to all connected clients
     * @param type Packet type
     * @param payload Packet payload data
     */
    void broadcast_tcp_packet(protocol::PacketType type, const std::vector<uint8_t>& payload);

    /**
     * @brief Broadcast a TCP packet to all players in a lobby
     * @param lobby_id Lobby ID
     * @param type Packet type
     * @param payload Packet payload data
     * @param lobby_manager Reference to the lobby manager for player lookup
     * @param connected_clients Map of connected clients for client ID lookup
     */
    void broadcast_tcp_to_lobby(uint32_t lobby_id, protocol::PacketType type,
                               const std::vector<uint8_t>& payload,
                               LobbyManager& lobby_manager,
                               const std::unordered_map<uint32_t, PlayerInfo>& connected_clients);

    // ============== UDP Sending ==============

    /**
     * @brief Send a UDP packet to a specific client
     * @param client_id Target client ID
     * @param type Packet type
     * @param payload Packet payload data
     */
    void send_udp_packet(uint32_t client_id, protocol::PacketType type,
                        const std::vector<uint8_t>& payload);

    /**
     * @brief Broadcast a UDP packet to all players in a game session
     * @param session_id Game session ID
     * @param type Packet type
     * @param payload Packet payload data
     * @param player_ids List of player IDs in the session
     * @param connected_clients Map of connected clients for UDP connection lookup
     */
    void broadcast_udp_to_session(uint32_t session_id, protocol::PacketType type,
                                  const std::vector<uint8_t>& payload,
                                  const std::vector<uint32_t>& player_ids,
                                  const std::unordered_map<uint32_t, PlayerInfo>& connected_clients);

private:
    engine::INetworkPlugin* network_plugin_;

    /**
     * @brief Create a packet with header and payload
     * @param type Packet type
     * @param payload Packet payload data
     * @return Complete packet data (header + payload)
     */
    std::vector<uint8_t> create_packet(protocol::PacketType type,
                                       const std::vector<uint8_t>& payload);
};

}
