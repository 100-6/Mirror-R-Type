#pragma once

#include <vector>
#include <cstdint>

#include "plugin_manager/INetworkPlugin.hpp"
#include "PacketTypes.hpp"
#include "Payloads.hpp"

namespace bagario::server {

/**
 * @brief Handles sending packets to clients
 *
 * Responsibilities:
 * - Serialize payloads
 * - Send packets via network plugin
 * - Support unicast and broadcast
 */
class BagarioPacketSender {
public:
    explicit BagarioPacketSender(engine::INetworkPlugin* network);
    ~BagarioPacketSender() = default;

    // Connection responses
    void send_accept(uint32_t client_id, const protocol::ServerAcceptPayload& payload);
    void send_reject(uint32_t client_id, const protocol::ServerRejectPayload& payload);
    void send_pong(uint32_t client_id, const protocol::ServerPongPayload& payload);

    // Game state
    void broadcast_snapshot(const protocol::ServerSnapshotPayload& header,
                           const std::vector<protocol::EntityState>& entities);
    void send_snapshot(uint32_t client_id, const protocol::ServerSnapshotPayload& header,
                      const std::vector<protocol::EntityState>& entities);

    // Entity events
    void broadcast_entity_spawn(const protocol::ServerEntitySpawnPayload& payload);
    void broadcast_entity_destroy(const protocol::ServerEntityDestroyPayload& payload);

    // Game events
    void send_player_eaten(uint32_t client_id, const protocol::ServerPlayerEatenPayload& payload);
    void broadcast_leaderboard(const protocol::ServerLeaderboardPayload& header,
                               const std::vector<protocol::LeaderboardEntry>& entries);

    // Skin sync
    void broadcast_player_skin(uint32_t player_id, const std::vector<uint8_t>& skin_data);

private:
    template<typename T>
    std::vector<uint8_t> serialize_packet(protocol::PacketType type, const T& payload) const {
        std::vector<uint8_t> data(1 + sizeof(T));
        data[0] = static_cast<uint8_t>(type);
        std::memcpy(data.data() + 1, &payload, sizeof(T));
        return data;
    }

    std::vector<uint8_t> serialize_snapshot(
        const protocol::ServerSnapshotPayload& header,
        const std::vector<protocol::EntityState>& entities
    ) const;

    std::vector<uint8_t> serialize_leaderboard(
        const protocol::ServerLeaderboardPayload& header,
        const std::vector<protocol::LeaderboardEntry>& entries
    ) const;

    void send_tcp(uint32_t client_id, const std::vector<uint8_t>& data);
    void send_udp(uint32_t client_id, const std::vector<uint8_t>& data);
    void broadcast_udp(const std::vector<uint8_t>& data);
    void broadcast_tcp(const std::vector<uint8_t>& data);

    engine::INetworkPlugin* m_network;
};

}
