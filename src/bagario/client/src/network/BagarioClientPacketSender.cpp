#include "network/BagarioClientPacketSender.hpp"
#include "BagarioConfig.hpp"

namespace bagario::client {

BagarioClientPacketSender::BagarioClientPacketSender(engine::INetworkPlugin* network)
    : m_network(network) {}

void BagarioClientPacketSender::send_connect(const std::string& player_name) {
    protocol::ClientConnectPayload payload;
    payload.client_version = config::PROTOCOL_VERSION;
    payload.set_player_name(player_name);

    auto data = serialize_packet(protocol::PacketType::CLIENT_CONNECT, payload);
    send_tcp(data);
}

void BagarioClientPacketSender::send_disconnect(uint32_t player_id, protocol::DisconnectReason reason) {
    protocol::ClientDisconnectPayload payload;
    payload.player_id = player_id;
    payload.reason = reason;

    auto data = serialize_packet(protocol::PacketType::CLIENT_DISCONNECT, payload);
    send_tcp(data);
}

void BagarioClientPacketSender::send_ping(uint32_t player_id, uint32_t timestamp) {
    protocol::ClientPingPayload payload;
    payload.player_id = player_id;
    payload.client_timestamp = timestamp;

    auto data = serialize_packet(protocol::PacketType::CLIENT_PING, payload);
    send_tcp(data);
}

void BagarioClientPacketSender::send_input(uint32_t player_id, float target_x, float target_y, uint32_t sequence) {
    protocol::ClientInputPayload payload;
    payload.player_id = player_id;
    payload.target_x = target_x;
    payload.target_y = target_y;
    payload.sequence = sequence;

    auto data = serialize_packet(protocol::PacketType::CLIENT_INPUT, payload);
    send_udp(data);  // UDP for faster input response
}

void BagarioClientPacketSender::send_split(uint32_t player_id) {
    protocol::ClientSplitPayload payload;
    payload.player_id = player_id;

    auto data = serialize_packet(protocol::PacketType::CLIENT_SPLIT, payload);
    send_tcp(data);  // TCP for reliability
}

void BagarioClientPacketSender::send_eject_mass(uint32_t player_id, float direction_x, float direction_y) {
    protocol::ClientEjectMassPayload payload;
    payload.player_id = player_id;
    payload.direction_x = direction_x;
    payload.direction_y = direction_y;

    auto data = serialize_packet(protocol::PacketType::CLIENT_EJECT_MASS, payload);
    send_tcp(data);  // TCP for reliability
}

void BagarioClientPacketSender::send_skin(uint32_t player_id, const std::vector<uint8_t>& skin_data) {
    // Build packet: [type][player_id][skin_data...]
    std::vector<uint8_t> data;
    data.reserve(1 + sizeof(protocol::ClientSetSkinPayload) + skin_data.size());

    // Packet type
    data.push_back(static_cast<uint8_t>(protocol::PacketType::CLIENT_SET_SKIN));

    // Header (player_id)
    protocol::ClientSetSkinPayload header;
    header.player_id = player_id;
    const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&header);
    data.insert(data.end(), header_bytes, header_bytes + sizeof(header));

    // Skin data
    data.insert(data.end(), skin_data.begin(), skin_data.end());

    send_tcp(data);  // TCP for reliability (skin data can be large)
}

void BagarioClientPacketSender::send_tcp(const std::vector<uint8_t>& data) {
    if (m_network) {
        engine::NetworkPacket packet(data);
        m_network->send_tcp(packet);
    }
}

void BagarioClientPacketSender::send_udp(const std::vector<uint8_t>& data) {
    if (m_network) {
        engine::NetworkPacket packet(data);
        m_network->send_udp(packet);
    }
}

}  // namespace bagario::client
