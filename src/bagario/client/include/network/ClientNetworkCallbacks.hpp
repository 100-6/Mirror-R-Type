#pragma once

#include <functional>
#include <vector>
#include <cstdint>

#include "PacketTypes.hpp"
#include "Payloads.hpp"

namespace bagario::client {

/**
 * @brief Callback interface for handling server packets on the client
 */
struct ClientNetworkCallbacks {
    // Connection responses
    std::function<void(const protocol::ServerAcceptPayload&)> on_accept;
    std::function<void(const protocol::ServerRejectPayload&)> on_reject;
    std::function<void(const protocol::ServerPongPayload&)> on_pong;

    // World state
    std::function<void(const protocol::ServerSnapshotPayload&,
                       const std::vector<protocol::EntityState>&)> on_snapshot;

    // Entity events
    std::function<void(const protocol::ServerEntitySpawnPayload&)> on_entity_spawn;
    std::function<void(const protocol::ServerEntityDestroyPayload&)> on_entity_destroy;

    // Game events
    std::function<void(const protocol::ServerPlayerEatenPayload&)> on_player_eaten;
    std::function<void(const protocol::ServerLeaderboardPayload&,
                       const std::vector<protocol::LeaderboardEntry>&)> on_leaderboard;

    // Skin sync
    std::function<void(uint32_t player_id, const std::vector<uint8_t>& skin_data)> on_player_skin;

    // Connection status
    std::function<void()> on_disconnected;
};

}  // namespace bagario::client
