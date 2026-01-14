#include "network/BagarioClientNetworkHandler.hpp"
#include <iostream>

namespace bagario::client {

BagarioClientNetworkHandler::BagarioClientNetworkHandler(engine::INetworkPlugin* network)
    : m_network(network) {}

void BagarioClientNetworkHandler::set_callbacks(const ClientNetworkCallbacks& callbacks) {
    m_callbacks = callbacks;
}

void BagarioClientNetworkHandler::process_packets() {
    if (!m_network)
        return;

    auto packets = m_network->receive();
    for (const auto& packet : packets) {
        handle_packet(packet);
    }
}

void BagarioClientNetworkHandler::handle_packet(const engine::NetworkPacket& packet) {
    if (packet.data.empty())
        return;

    auto packet_type = static_cast<protocol::PacketType>(packet.data[0]);

    switch (packet_type) {
        case protocol::PacketType::SERVER_ACCEPT: {
            protocol::ServerAcceptPayload payload;
            if (parse_payload(packet, payload) && m_callbacks.on_accept) {
                m_callbacks.on_accept(payload);
            }
            break;
        }

        case protocol::PacketType::SERVER_REJECT: {
            protocol::ServerRejectPayload payload;
            if (parse_payload(packet, payload) && m_callbacks.on_reject) {
                m_callbacks.on_reject(payload);
            }
            break;
        }

        case protocol::PacketType::SERVER_PONG: {
            protocol::ServerPongPayload payload;
            if (parse_payload(packet, payload) && m_callbacks.on_pong) {
                m_callbacks.on_pong(payload);
            }
            break;
        }

        case protocol::PacketType::SERVER_SNAPSHOT: {
            parse_snapshot(packet);
            break;
        }

        case protocol::PacketType::SERVER_ENTITY_SPAWN: {
            protocol::ServerEntitySpawnPayload payload;
            if (parse_payload(packet, payload) && m_callbacks.on_entity_spawn) {
                m_callbacks.on_entity_spawn(payload);
            }
            break;
        }

        case protocol::PacketType::SERVER_ENTITY_DESTROY: {
            protocol::ServerEntityDestroyPayload payload;
            if (parse_payload(packet, payload) && m_callbacks.on_entity_destroy) {
                m_callbacks.on_entity_destroy(payload);
            }
            break;
        }

        case protocol::PacketType::SERVER_PLAYER_EATEN: {
            protocol::ServerPlayerEatenPayload payload;
            if (parse_payload(packet, payload) && m_callbacks.on_player_eaten) {
                m_callbacks.on_player_eaten(payload);
            }
            break;
        }

        case protocol::PacketType::SERVER_LEADERBOARD: {
            parse_leaderboard(packet);
            break;
        }

        case protocol::PacketType::SERVER_PLAYER_SKIN: {
            parse_player_skin(packet);
            break;
        }

        default:
            std::cerr << "[ClientNetworkHandler] Unknown packet type: 0x"
                      << std::hex << static_cast<int>(packet_type) << std::dec << "\n";
            break;
    }
}

void BagarioClientNetworkHandler::parse_snapshot(const engine::NetworkPacket& packet) {
    // Minimum size: 1 (type) + 6 (header)
    if (packet.data.size() < 1 + sizeof(protocol::ServerSnapshotPayload)) {
        return;
    }

    protocol::ServerSnapshotPayload header;
    std::memcpy(&header, packet.data.data() + 1, sizeof(header));

    // Calculate expected size
    size_t expected_size = 1 + sizeof(header) + header.entity_count * sizeof(protocol::EntityState);
    if (packet.data.size() < expected_size) {
        std::cerr << "[ClientNetworkHandler] Snapshot packet too small\n";
        return;
    }

    // Parse entities
    std::vector<protocol::EntityState> entities;
    entities.resize(header.entity_count);

    const uint8_t* entity_data = packet.data.data() + 1 + sizeof(header);
    std::memcpy(entities.data(), entity_data, header.entity_count * sizeof(protocol::EntityState));

    if (m_callbacks.on_snapshot) {
        m_callbacks.on_snapshot(header, entities);
    }
}

void BagarioClientNetworkHandler::parse_leaderboard(const engine::NetworkPacket& packet) {
    // Minimum size: 1 (type) + 1 (header)
    if (packet.data.size() < 1 + sizeof(protocol::ServerLeaderboardPayload)) {
        return;
    }

    protocol::ServerLeaderboardPayload header;
    std::memcpy(&header, packet.data.data() + 1, sizeof(header));

    // Calculate expected size
    size_t expected_size = 1 + sizeof(header) + header.entry_count * sizeof(protocol::LeaderboardEntry);
    if (packet.data.size() < expected_size) {
        std::cerr << "[ClientNetworkHandler] Leaderboard packet too small\n";
        return;
    }

    // Parse entries
    std::vector<protocol::LeaderboardEntry> entries;
    entries.resize(header.entry_count);

    const uint8_t* entry_data = packet.data.data() + 1 + sizeof(header);
    std::memcpy(entries.data(), entry_data, header.entry_count * sizeof(protocol::LeaderboardEntry));

    if (m_callbacks.on_leaderboard) {
        m_callbacks.on_leaderboard(header, entries);
    }
}

void BagarioClientNetworkHandler::parse_player_skin(const engine::NetworkPacket& packet) {
    // Minimum size: 1 (type) + 4 (player_id) + 17 (min skin header)
    constexpr size_t MIN_SKIN_HEADER = 17;  // From PlayerSkin::HEADER_SIZE
    if (packet.data.size() < 1 + sizeof(protocol::ServerPlayerSkinPayload) + MIN_SKIN_HEADER) {
        return;
    }

    protocol::ServerPlayerSkinPayload header;
    std::memcpy(&header, packet.data.data() + 1, sizeof(header));

    // Extract skin data (everything after the player_id)
    size_t skin_data_offset = 1 + sizeof(header);
    size_t skin_data_size = packet.data.size() - skin_data_offset;

    std::vector<uint8_t> skin_data(
        packet.data.begin() + skin_data_offset,
        packet.data.end()
    );

    if (m_callbacks.on_player_skin) {
        m_callbacks.on_player_skin(header.player_id, skin_data);
    }
}

}  // namespace bagario::client
