#include "BagarioPacketSender.hpp"

#include <iostream>

namespace bagario::server {

BagarioPacketSender::BagarioPacketSender(engine::INetworkPlugin* network)
    : m_network(network) {}

void BagarioPacketSender::send_accept(uint32_t client_id, const protocol::ServerAcceptPayload& payload) {
    auto data = serialize_packet(protocol::PacketType::SERVER_ACCEPT, payload);
    send_tcp(client_id, data);
}

void BagarioPacketSender::send_reject(uint32_t client_id, const protocol::ServerRejectPayload& payload) {
    auto data = serialize_packet(protocol::PacketType::SERVER_REJECT, payload);
    send_tcp(client_id, data);
}

void BagarioPacketSender::send_pong(uint32_t client_id, const protocol::ServerPongPayload& payload) {
    auto data = serialize_packet(protocol::PacketType::SERVER_PONG, payload);
    send_tcp(client_id, data);
}

void BagarioPacketSender::broadcast_snapshot(const protocol::ServerSnapshotPayload& header,
                                              const std::vector<protocol::EntityState>& entities) {
    auto data = serialize_snapshot(header, entities);
    broadcast_udp(data);
}

void BagarioPacketSender::send_snapshot(uint32_t client_id, const protocol::ServerSnapshotPayload& header,
                                        const std::vector<protocol::EntityState>& entities) {
    auto data = serialize_snapshot(header, entities);
    send_udp(client_id, data);
}

void BagarioPacketSender::broadcast_entity_spawn(const protocol::ServerEntitySpawnPayload& payload) {
    auto data = serialize_packet(protocol::PacketType::SERVER_ENTITY_SPAWN, payload);
    broadcast_udp(data);
}

void BagarioPacketSender::broadcast_entity_destroy(const protocol::ServerEntityDestroyPayload& payload) {
    auto data = serialize_packet(protocol::PacketType::SERVER_ENTITY_DESTROY, payload);
    broadcast_udp(data);
}

void BagarioPacketSender::send_player_eaten(uint32_t client_id, const protocol::ServerPlayerEatenPayload& payload) {
    auto data = serialize_packet(protocol::PacketType::SERVER_PLAYER_EATEN, payload);
    send_tcp(client_id, data);
}

void BagarioPacketSender::broadcast_leaderboard(const protocol::ServerLeaderboardPayload& header,
                                                const std::vector<protocol::LeaderboardEntry>& entries) {
    auto data = serialize_leaderboard(header, entries);
    broadcast_udp(data);
}

std::vector<uint8_t> BagarioPacketSender::serialize_snapshot(
    const protocol::ServerSnapshotPayload& header,
    const std::vector<protocol::EntityState>& entities
) const {
    size_t total_size = 1 + sizeof(header) + entities.size() * sizeof(protocol::EntityState);
    std::vector<uint8_t> data(total_size);

    data[0] = static_cast<uint8_t>(protocol::PacketType::SERVER_SNAPSHOT);
    std::memcpy(data.data() + 1, &header, sizeof(header));
    std::memcpy(data.data() + 1 + sizeof(header), entities.data(),
                entities.size() * sizeof(protocol::EntityState));

    return data;
}

std::vector<uint8_t> BagarioPacketSender::serialize_leaderboard(
    const protocol::ServerLeaderboardPayload& header,
    const std::vector<protocol::LeaderboardEntry>& entries
) const {
    size_t entry_count = std::min(entries.size(), static_cast<size_t>(header.entry_count));
    size_t total_size = 1 + sizeof(header) + entry_count * sizeof(protocol::LeaderboardEntry);
    std::vector<uint8_t> data(total_size);

    data[0] = static_cast<uint8_t>(protocol::PacketType::SERVER_LEADERBOARD);
    std::memcpy(data.data() + 1, &header, sizeof(header));
    std::memcpy(data.data() + 1 + sizeof(header), entries.data(),
                entry_count * sizeof(protocol::LeaderboardEntry));

    return data;
}

void BagarioPacketSender::send_tcp(uint32_t client_id, const std::vector<uint8_t>& data) {
    engine::NetworkPacket packet(data);

    if (!m_network->send_tcp_to(packet, client_id))
        std::cerr << "[BagarioPacketSender] Failed to send TCP to client " << client_id << std::endl;
}

void BagarioPacketSender::send_udp(uint32_t client_id, const std::vector<uint8_t>& data) {
    engine::NetworkPacket packet(data);

    if (!m_network->send_udp_to(packet, client_id))
        std::cerr << "[BagarioPacketSender] Failed to send UDP to client " << client_id << std::endl;
}

void BagarioPacketSender::broadcast_udp(const std::vector<uint8_t>& data) {
    engine::NetworkPacket packet(data);
    m_network->broadcast_udp(packet);
}

void BagarioPacketSender::broadcast_tcp(const std::vector<uint8_t>& data) {
    engine::NetworkPacket packet(data);
    m_network->broadcast_tcp(packet);
}

}
