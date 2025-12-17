#include "PacketSender.hpp"
#include "protocol/ProtocolEncoder.hpp"
#include "LobbyManager.hpp"
#include "PlayerInfo.hpp"

namespace rtype::server {

PacketSender::PacketSender(engine::INetworkPlugin* network_plugin)
    : network_plugin_(network_plugin) {
}

std::vector<uint8_t> PacketSender::create_packet(protocol::PacketType type,
                                                 const std::vector<uint8_t>& payload) {
    protocol::PacketHeader header;
    header.version = protocol::PROTOCOL_VERSION;
    header.type = static_cast<uint8_t>(type);
    header.payload_length = static_cast<uint16_t>(payload.size());
    header.sequence_number = 0;

    std::vector<uint8_t> packet_data(protocol::HEADER_SIZE);
    protocol::ProtocolEncoder::encode_header(header, packet_data.data());
    packet_data.insert(packet_data.end(), payload.begin(), payload.end());

    return packet_data;
}

// ============== TCP Sending ==============

void PacketSender::send_tcp_packet(uint32_t client_id, protocol::PacketType type,
                                   const std::vector<uint8_t>& payload) {
    auto packet_data = create_packet(type, payload);

    engine::NetworkPacket packet;
    packet.data = packet_data;
    network_plugin_->send_tcp_to(packet, client_id);
}

void PacketSender::broadcast_tcp_packet(protocol::PacketType type,
                                       const std::vector<uint8_t>& payload) {
    auto packet_data = create_packet(type, payload);

    engine::NetworkPacket packet;
    packet.data = packet_data;
    network_plugin_->broadcast_tcp(packet);
}

void PacketSender::broadcast_tcp_to_lobby(uint32_t lobby_id, protocol::PacketType type,
                                         const std::vector<uint8_t>& payload,
                                         LobbyManager& lobby_manager,
                                         const std::unordered_map<uint32_t, PlayerInfo>& connected_clients) {
    auto player_ids = lobby_manager.get_lobby_players(lobby_id);

    for (uint32_t player_id : player_ids) {
        for (const auto& [client_id, player_info] : connected_clients) {
            if (player_info.player_id == player_id) {
                send_tcp_packet(client_id, type, payload);
                break;
            }
        }
    }
}

// ============== UDP Sending ==============

void PacketSender::send_udp_packet(uint32_t client_id, protocol::PacketType type,
                                   const std::vector<uint8_t>& payload) {
    auto packet_data = create_packet(type, payload);

    engine::NetworkPacket packet;
    packet.data = packet_data;
    network_plugin_->send_udp_to(packet, client_id);
}

void PacketSender::broadcast_udp_to_session(uint32_t session_id, protocol::PacketType type,
                                           const std::vector<uint8_t>& payload,
                                           const std::vector<uint32_t>& player_ids,
                                           const std::unordered_map<uint32_t, PlayerInfo>& connected_clients) {
    auto packet_data = create_packet(type, payload);

    engine::NetworkPacket packet;
    packet.data = packet_data;

    for (uint32_t player_id : player_ids) {
        for (const auto& [client_id, player_info] : connected_clients) {
            if (player_info.player_id == player_id && player_info.has_udp_connection()) {
                network_plugin_->send_udp_to(packet, client_id);
                break;
            }
        }
    }
}

}
