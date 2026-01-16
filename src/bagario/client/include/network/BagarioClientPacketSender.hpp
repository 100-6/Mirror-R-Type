#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <cstring>

#include "plugin_manager/INetworkPlugin.hpp"
#include "PacketTypes.hpp"
#include "Payloads.hpp"

namespace bagario::client {

/**
 * @brief Handles sending packets from client to server
 *
 * Mirrors the server-side BagarioPacketSender pattern but sends
 * client-to-server packets instead.
 */
class BagarioClientPacketSender {
public:
    explicit BagarioClientPacketSender(engine::INetworkPlugin* network);
    ~BagarioClientPacketSender() = default;

    // Connection
    void send_connect(const std::string& player_name);
    void send_disconnect(uint32_t player_id, protocol::DisconnectReason reason);
    void send_ping(uint32_t player_id, uint32_t timestamp);

    // Gameplay input (UDP for speed)
    void send_input(uint32_t player_id, float target_x, float target_y, uint32_t sequence);
    void send_split(uint32_t player_id);
    void send_eject_mass(uint32_t player_id, float direction_x, float direction_y);

    // Skin customization
    void send_skin(uint32_t player_id, const std::vector<uint8_t>& skin_data);

private:
    template<typename T>
    std::vector<uint8_t> serialize_packet(protocol::PacketType type, const T& payload) const {
        std::vector<uint8_t> data(1 + sizeof(T));
        data[0] = static_cast<uint8_t>(type);
        std::memcpy(data.data() + 1, &payload, sizeof(T));
        return data;
    }

    void send_tcp(const std::vector<uint8_t>& data);
    void send_udp(const std::vector<uint8_t>& data);

    engine::INetworkPlugin* m_network;
};

}  // namespace bagario::client
