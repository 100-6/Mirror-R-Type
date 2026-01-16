#include "BagarioNetworkHandler.hpp"

#include <iostream>

namespace bagario::server {

BagarioNetworkHandler::BagarioNetworkHandler(engine::INetworkPlugin* network)
    : m_network(network) {}

void BagarioNetworkHandler::set_callbacks(const NetworkCallbacks& callbacks) {
    m_callbacks = callbacks;
}

void BagarioNetworkHandler::process_packets() {
    auto packets = m_network->receive();

    for (const auto& packet : packets) {
        handle_packet(packet.sender_id, packet);
    }
}

void BagarioNetworkHandler::handle_packet(uint32_t client_id, const engine::NetworkPacket& packet) {
    if (packet.data.empty())
        return;
    auto packet_type = static_cast<protocol::PacketType>(packet.data[0]);

    switch (packet_type) {
        case protocol::PacketType::CLIENT_CONNECT: {
            protocol::ClientConnectPayload payload;
            if (parse_payload(packet, payload) && m_callbacks.on_connect)
                m_callbacks.on_connect(client_id, payload);
            break;
        }
        case protocol::PacketType::CLIENT_DISCONNECT: {
            protocol::ClientDisconnectPayload payload;
            if (parse_payload(packet, payload) && m_callbacks.on_disconnect)
                m_callbacks.on_disconnect(client_id, payload);
            break;
        }
        case protocol::PacketType::CLIENT_PING: {
            protocol::ClientPingPayload payload;
            if (parse_payload(packet, payload) && m_callbacks.on_ping)
                m_callbacks.on_ping(client_id, payload);
            break;
        }
        case protocol::PacketType::CLIENT_INPUT: {
            protocol::ClientInputPayload payload;
            if (parse_payload(packet, payload) && m_callbacks.on_input)
                m_callbacks.on_input(client_id, payload);
            break;
        }
        case protocol::PacketType::CLIENT_SPLIT: {
            protocol::ClientSplitPayload payload;
            if (parse_payload(packet, payload) && m_callbacks.on_split)
                m_callbacks.on_split(client_id, payload);
            break;
        }
        case protocol::PacketType::CLIENT_EJECT_MASS: {
            protocol::ClientEjectMassPayload payload;
            if (parse_payload(packet, payload) && m_callbacks.on_eject_mass)
                m_callbacks.on_eject_mass(client_id, payload);
            break;
        }
        case protocol::PacketType::CLIENT_SET_SKIN: {
            // Variable-size packet: [type][player_id][skin_data...]
            constexpr size_t MIN_SIZE = 1 + sizeof(protocol::ClientSetSkinPayload) + 17;  // 17 = min skin header
            if (packet.data.size() >= MIN_SIZE && m_callbacks.on_set_skin) {
                protocol::ClientSetSkinPayload header;
                std::memcpy(&header, packet.data.data() + 1, sizeof(header));

                // Extract skin data (everything after player_id)
                size_t skin_offset = 1 + sizeof(header);
                std::vector<uint8_t> skin_data(
                    packet.data.begin() + skin_offset,
                    packet.data.end()
                );

                m_callbacks.on_set_skin(client_id, header.player_id, skin_data);
            }
            break;
        }
        default:
            std::cerr << "[BagarioNetworkHandler] Unknown packet type: 0x"
                      << std::hex << static_cast<int>(packet_type) << std::dec << std::endl;
            break;
    }
}

bool BagarioNetworkHandler::validate_packet(const engine::NetworkPacket& packet,
                                            protocol::PacketType expected_type,
                                            size_t expected_size) const {
    if (packet.data.empty())
        return false;
    if (static_cast<protocol::PacketType>(packet.data[0]) != expected_type)
        return false;
    if (packet.data.size() < expected_size + 1)
        return false;
    return true;
}

}
