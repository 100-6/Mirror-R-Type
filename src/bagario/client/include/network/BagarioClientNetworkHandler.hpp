#pragma once

#include <vector>
#include <cstdint>
#include <cstring>

#include "plugin_manager/INetworkPlugin.hpp"
#include "PacketTypes.hpp"
#include "Payloads.hpp"
#include "ClientNetworkCallbacks.hpp"

namespace bagario::client {

/**
 * @brief Handles receiving and parsing server packets on the client
 *
 * Mirrors the server-side BagarioNetworkHandler pattern but processes
 * server-to-client packets instead.
 */
class BagarioClientNetworkHandler {
public:
    explicit BagarioClientNetworkHandler(engine::INetworkPlugin* network);
    ~BagarioClientNetworkHandler() = default;

    /**
     * @brief Set callbacks for packet handling
     */
    void set_callbacks(const ClientNetworkCallbacks& callbacks);

    /**
     * @brief Process all pending packets from the server
     * Call this each frame
     */
    void process_packets();

private:
    void handle_packet(const engine::NetworkPacket& packet);

    template<typename T>
    bool parse_payload(const engine::NetworkPacket& packet, T& payload) const {
        if (packet.data.size() < sizeof(T) + 1)
            return false;
        std::memcpy(&payload, packet.data.data() + 1, sizeof(T));
        return true;
    }

    // Variable-size packet parsing
    void parse_snapshot(const engine::NetworkPacket& packet);
    void parse_leaderboard(const engine::NetworkPacket& packet);
    void parse_player_skin(const engine::NetworkPacket& packet);

    engine::INetworkPlugin* m_network;
    ClientNetworkCallbacks m_callbacks;
};

}  // namespace bagario::client
