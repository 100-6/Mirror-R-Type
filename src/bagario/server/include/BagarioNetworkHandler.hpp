#pragma once

#include <functional>
#include <vector>
#include <cstdint>

#include "plugin_manager/INetworkPlugin.hpp"
#include "PacketTypes.hpp"
#include "Payloads.hpp"

namespace bagario::server {

/**
 * @brief Callback interface for handling parsed packets
 */
struct NetworkCallbacks {
    std::function<void(uint32_t, const protocol::ClientConnectPayload&)> on_connect;
    std::function<void(uint32_t, const protocol::ClientDisconnectPayload&)> on_disconnect;
    std::function<void(uint32_t, const protocol::ClientPingPayload&)> on_ping;
    std::function<void(uint32_t, const protocol::ClientInputPayload&)> on_input;
    std::function<void(uint32_t, const protocol::ClientSplitPayload&)> on_split;
    std::function<void(uint32_t, const protocol::ClientEjectMassPayload&)> on_eject_mass;
};

/**
 * @brief Handles receiving and parsing network packets
 *
 * Responsibilities:
 * - Receive packets from network plugin
 * - Validate packet headers
 * - Deserialize payloads
 * - Route to appropriate callbacks
 */
class BagarioNetworkHandler {
public:
    explicit BagarioNetworkHandler(engine::INetworkPlugin* network);
    ~BagarioNetworkHandler() = default;

    /**
     * @brief Set callbacks for packet handling
     */
    void set_callbacks(const NetworkCallbacks& callbacks);

    /**
     * @brief Process all pending packets
     * Call this each frame/tick
     */
    void process_packets();

private:
    void handle_packet(uint32_t client_id, const engine::NetworkPacket& packet);
    bool validate_packet(const engine::NetworkPacket& packet, protocol::PacketType expected_type,
                         size_t expected_size) const;

    template<typename T>
    bool parse_payload(const engine::NetworkPacket& packet, T& payload) const {
        if (packet.data.size() < sizeof(T) + 1)
            return false;
        std::memcpy(&payload, packet.data.data() + 1, sizeof(T));
        return true;
    }

    engine::INetworkPlugin* m_network;
    NetworkCallbacks m_callbacks;
};

}
