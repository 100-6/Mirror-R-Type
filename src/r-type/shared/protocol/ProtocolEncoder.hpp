#pragma once

#include "PacketHeader.hpp"
#include "PacketTypes.hpp"
#include "Payloads.hpp"
#include "compression/PacketCompressor.hpp"
#include "compression/CompressionStats.hpp"
#include <vector>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
#endif

namespace rtype::protocol {

/**
 * @brief Protocol encoding/decoding utilities
 *
 * Handles conversion between host and network byte order (big-endian),
 * packet serialization, and validation.
 *
 * Cross-platform compatible (Windows, Linux, macOS).
 */
class ProtocolEncoder {
public:
    /**
     * @brief Encode a packet with header and payload (with optional compression)
     *
     * Automatically compresses payload if:
     * - Packet type is compressible
     * - Payload size meets minimum threshold
     * - Compression provides sufficient gain
     *
     * Records compression statistics for monitoring.
     *
     * @param type Packet type
     * @param payload Pointer to payload data
     * @param payload_size Size of payload in bytes
     * @param sequence_number Sequence number for the packet
     * @return Encoded packet as byte vector (header + possibly compressed payload)
     * @throws std::invalid_argument if payload is too large
     */
    static std::vector<uint8_t> encode_packet(PacketType type, const void* payload,
                                               size_t payload_size, uint32_t sequence_number) {
        if (payload_size > MAX_PAYLOAD_SIZE)
            throw std::invalid_argument("Payload size exceeds maximum allowed size");

        // Handle empty payload case (avoid nullptr dereference)
        std::vector<uint8_t> payload_vec;
        if (payload_size > 0 && payload != nullptr) {
            payload_vec.assign(
                static_cast<const uint8_t*>(payload),
                static_cast<const uint8_t*>(payload) + payload_size
            );
        }

        bool used_compression = false;
        uint32_t original_size = static_cast<uint32_t>(payload_size);
        std::vector<uint8_t> final_payload = payload_vec;

        if (PacketCompressor::should_compress(type, payload_size)) {
            auto compression_result = PacketCompressor::compress(payload_vec);
            if (compression_result.used_compression) {
                final_payload = std::move(compression_result.data);
                used_compression = true;
                CompressionStats::record_compression(
                    compression_result.original_size,
                    compression_result.compressed_size,
                    compression_result.compression_time,
                    true
                );
            } else {
                CompressionStats::record_compression(
                    compression_result.original_size,
                    compression_result.original_size,
                    compression_result.compression_time,
                    false
                );
            }
        } else
            CompressionStats::record_compression(original_size, original_size, std::chrono::microseconds(0), false);
        uint8_t flags = used_compression ? PACKET_FLAG_COMPRESSED : 0;
        PacketHeader header(
            static_cast<uint8_t>(type),
            static_cast<uint16_t>(final_payload.size()),
            sequence_number,
            flags
        );
        if (used_compression)
            header.uncompressed_size = original_size;
        size_t total_size = header.get_header_size() + final_payload.size();
        std::vector<uint8_t> buffer(total_size);

        // DEBUG: Log packet encoding details
        #ifdef DEBUG_PROTOCOL_ENCODING
        std::cout << "[ProtocolEncoder] Encoding packet: type=" << static_cast<int>(type)
                  << ", header_size=" << header.get_header_size()
                  << ", payload_size=" << final_payload.size()
                  << ", total_size=" << total_size
                  << ", flags=" << static_cast<int>(flags) << "\n";
        #endif

        encode_header(header, buffer.data());
        if (!final_payload.empty())
            std::memcpy(buffer.data() + header.get_header_size(), final_payload.data(), final_payload.size());
        return buffer;
    }

    /**
     * @brief Encode header to network byte order
     *
     * @param header Header structure in host byte order
     * @param buffer Output buffer (must be at least get_header_size() bytes)
     */
    static void encode_header(const PacketHeader& header, uint8_t* buffer) {
        buffer[0] = header.version;
        buffer[1] = header.type;
        buffer[2] = header.flags;
        uint16_t payload_length_be = htons(header.payload_length);
        uint32_t sequence_number_be = htonl(header.sequence_number);

        std::memcpy(buffer + 3, &payload_length_be, sizeof(uint16_t));
        std::memcpy(buffer + 5, &sequence_number_be, sizeof(uint32_t));
        if (header.flags & PACKET_FLAG_COMPRESSED) {
            uint32_t uncompressed_size_be = htonl(header.uncompressed_size);
            std::memcpy(buffer + 9, &uncompressed_size_be, sizeof(uint32_t));
        }
    }

    /**
     * @brief Decode header from network byte order
     *
     * @param buffer Input buffer containing header
     * @param buffer_size Size of input buffer
     * @return Decoded header in host byte order
     * @throws std::invalid_argument if buffer is too small
     */
    static PacketHeader decode_header(const uint8_t* buffer, size_t buffer_size) {
        if (buffer_size < HEADER_SIZE)
            throw std::invalid_argument("Buffer too small to contain header");
        PacketHeader header;
        header.version = buffer[0];
        header.type = buffer[1];
        header.flags = buffer[2];
        uint16_t payload_length_be;
        uint32_t sequence_number_be;

        std::memcpy(&payload_length_be, buffer + 3, sizeof(uint16_t));
        std::memcpy(&sequence_number_be, buffer + 5, sizeof(uint32_t));
        header.payload_length = ntohs(payload_length_be);
        header.sequence_number = ntohl(sequence_number_be);
        if (header.flags & PACKET_FLAG_COMPRESSED) {
            if (buffer_size < HEADER_SIZE + COMPRESSED_HEADER_EXTRA)
                throw std::invalid_argument("Buffer too small for compressed header");
            uint32_t uncompressed_size_be;
            std::memcpy(&uncompressed_size_be, buffer + 9, sizeof(uint32_t));
            header.uncompressed_size = ntohl(uncompressed_size_be);
        } else
            header.uncompressed_size = 0;
        return header;
    }

    /**
     * @brief Validate a received packet
     *
     * @param buffer Packet buffer
     * @param buffer_size Size of buffer
     * @return true if packet is valid
     */
    static bool validate_packet(const uint8_t* buffer, size_t buffer_size) {
        if (buffer_size < HEADER_SIZE)
            return false;
        PacketHeader header = decode_header(buffer, buffer_size);

        if (header.version != PROTOCOL_VERSION)
            return false;
        if (header.total_size() > MAX_PACKET_SIZE)
            return false;
        if (buffer_size < header.total_size())
            return false;
        return true;
    }

    /**
     * @brief Get payload pointer from packet buffer
     *
     * @param buffer Packet buffer
     * @param buffer_size Size of buffer
     * @return Pointer to payload data (after header)
     * @throws std::invalid_argument if packet is invalid
     */
    static const uint8_t* get_payload(const uint8_t* buffer, size_t buffer_size) {
        if (!validate_packet(buffer, buffer_size))
            throw std::invalid_argument("Invalid packet");
        PacketHeader header = decode_header(buffer, buffer_size);

        return buffer + header.get_header_size();
    }

    /**
     * @brief Get decompressed payload from packet buffer
     *
     * If packet is compressed, decompresses it. Otherwise returns raw payload.
     *
     * @param buffer Packet buffer
     * @param buffer_size Size of buffer
     * @return Decompressed payload as byte vector
     * @throws std::invalid_argument if packet is invalid
     * @throws std::runtime_error if decompression fails
     */
    static std::vector<uint8_t> get_decompressed_payload(const uint8_t* buffer, size_t buffer_size) {
        if (!validate_packet(buffer, buffer_size))
            throw std::invalid_argument("Invalid packet");
        PacketHeader header = decode_header(buffer, buffer_size);
        const uint8_t* payload_start = buffer + header.get_header_size();
        size_t payload_size = header.payload_length;

        if (header.is_compressed())
            return PacketCompressor::decompress(payload_start, payload_size, header.uncompressed_size);
        return std::vector<uint8_t>(payload_start, payload_start + payload_size);
    }

    /**
     * @brief Encode ClientConnectPayload with byte order conversion
     */
    static void encode_client_connect(const ClientConnectPayload& payload, uint8_t* buffer) {
        std::memcpy(buffer, &payload, sizeof(ClientConnectPayload));
    }

    /**
     * @brief Decode ClientConnectPayload with byte order conversion
     */
    static ClientConnectPayload decode_client_connect(const uint8_t* buffer) {
        ClientConnectPayload payload;
        std::memcpy(&payload, buffer, sizeof(ClientConnectPayload));

        return payload;
    }

    /**
     * @brief Encode ClientInputPayload with byte order conversion
     */
    static void encode_client_input(const ClientInputPayload& payload, uint8_t* buffer) {
        uint32_t player_id_be = htonl(payload.player_id);
        uint16_t input_flags_be = htons(payload.input_flags);
        uint32_t client_tick_be = htonl(payload.client_tick);

        std::memcpy(buffer, &player_id_be, sizeof(uint32_t));
        std::memcpy(buffer + 4, &input_flags_be, sizeof(uint16_t));
        std::memcpy(buffer + 6, &client_tick_be, sizeof(uint32_t));
    }

    /**
     * @brief Decode ClientInputPayload with byte order conversion
     */
    static ClientInputPayload decode_client_input(const uint8_t* buffer) {
        ClientInputPayload payload;
        uint32_t player_id_be;
        uint16_t input_flags_be;
        uint32_t client_tick_be;

        std::memcpy(&player_id_be, buffer, sizeof(uint32_t));
        std::memcpy(&input_flags_be, buffer + 4, sizeof(uint16_t));
        std::memcpy(&client_tick_be, buffer + 6, sizeof(uint32_t));
        payload.player_id = ntohl(player_id_be);
        payload.input_flags = ntohs(input_flags_be);
        payload.client_tick = ntohl(client_tick_be);
        return payload;
    }

    /**
     * @brief Encode ServerAcceptPayload with byte order conversion
     */
    static void encode_server_accept(const ServerAcceptPayload& payload, uint8_t* buffer) {
        uint32_t player_id_be = htonl(payload.assigned_player_id);
        uint16_t map_id_be = htons(payload.map_id);

        std::memcpy(buffer, &player_id_be, sizeof(uint32_t));
        buffer[4] = payload.server_tick_rate;
        buffer[5] = payload.max_players;
        std::memcpy(buffer + 6, &map_id_be, sizeof(uint16_t));
    }

    /**
     * @brief Decode ServerAcceptPayload with byte order conversion
     */
    static ServerAcceptPayload decode_server_accept(const uint8_t* buffer) {
        ServerAcceptPayload payload;
        uint32_t player_id_be;
        uint16_t map_id_be;

        std::memcpy(&player_id_be, buffer, sizeof(uint32_t));
        payload.server_tick_rate = buffer[4];
        payload.max_players = buffer[5];
        std::memcpy(&map_id_be, buffer + 6, sizeof(uint16_t));
        payload.assigned_player_id = ntohl(player_id_be);
        payload.map_id = ntohs(map_id_be);
        return payload;
    }

    /**
     * @brief Encode EntityState with byte order conversion
     */
    static void encode_entity_state(const EntityState& entity, uint8_t* buffer) {
        uint32_t entity_id_be = htonl(entity.entity_id);
        int16_t velocity_x_be = htons(entity.velocity_x);
        int16_t velocity_y_be = htons(entity.velocity_y);
        uint16_t health_be = htons(entity.health);
        uint16_t flags_be = htons(entity.flags);

        std::memcpy(buffer, &entity_id_be, sizeof(uint32_t));
        buffer[4] = static_cast<uint8_t>(entity.entity_type);
        uint32_t pos_x_bits;
        uint32_t pos_y_bits;
        std::memcpy(&pos_x_bits, &entity.position_x, sizeof(float));
        std::memcpy(&pos_y_bits, &entity.position_y, sizeof(float));
        uint32_t pos_x_be = htonl(pos_x_bits);
        uint32_t pos_y_be = htonl(pos_y_bits);
        std::memcpy(buffer + 5, &pos_x_be, sizeof(uint32_t));
        std::memcpy(buffer + 9, &pos_y_be, sizeof(uint32_t));
        std::memcpy(buffer + 13, &velocity_x_be, sizeof(int16_t));
        std::memcpy(buffer + 15, &velocity_y_be, sizeof(int16_t));
        std::memcpy(buffer + 17, &health_be, sizeof(uint16_t));
        std::memcpy(buffer + 19, &flags_be, sizeof(uint16_t));
    }

    /**
     * @brief Decode EntityState with byte order conversion
     */
    static EntityState decode_entity_state(const uint8_t* buffer) {
        EntityState entity;
        uint32_t entity_id_be;
        uint32_t pos_x_be, pos_y_be;
        int16_t velocity_x_be, velocity_y_be;
        uint16_t health_be, flags_be;

        std::memcpy(&entity_id_be, buffer, sizeof(uint32_t));
        entity.entity_type = static_cast<EntityType>(buffer[4]);
        std::memcpy(&pos_x_be, buffer + 5, sizeof(uint32_t));
        std::memcpy(&pos_y_be, buffer + 9, sizeof(uint32_t));
        std::memcpy(&velocity_x_be, buffer + 13, sizeof(int16_t));
        std::memcpy(&velocity_y_be, buffer + 15, sizeof(int16_t));
        std::memcpy(&health_be, buffer + 17, sizeof(uint16_t));
        std::memcpy(&flags_be, buffer + 19, sizeof(uint16_t));
        entity.entity_id = ntohl(entity_id_be);
        uint32_t pos_x_bits = ntohl(pos_x_be);
        uint32_t pos_y_bits = ntohl(pos_y_be);
        std::memcpy(&entity.position_x, &pos_x_bits, sizeof(float));
        std::memcpy(&entity.position_y, &pos_y_bits, sizeof(float));
        entity.velocity_x = ntohs(velocity_x_be);
        entity.velocity_y = ntohs(velocity_y_be);
        entity.health = ntohs(health_be);
        entity.flags = ntohs(flags_be);
        return entity;
    }

    /**
     * @brief Encode ServerSnapshotPayload with entity array
     */
    static std::vector<uint8_t> encode_server_snapshot(uint32_t server_tick,
                                                        const std::vector<EntityState>& entities,
                                                        uint32_t sequence_number) {
        constexpr size_t max_entities = (MAX_PAYLOAD_SIZE - sizeof(ServerSnapshotPayload)) / sizeof(EntityState);
        if (entities.size() > max_entities)
            throw std::invalid_argument("Too many entities for single snapshot");
        size_t payload_size = sizeof(ServerSnapshotPayload) + entities.size() * sizeof(EntityState);
        std::vector<uint8_t> payload_buffer(payload_size);
        uint32_t server_tick_be = htonl(server_tick);
        uint16_t entity_count_be = htons(static_cast<uint16_t>(entities.size()));

        std::memcpy(payload_buffer.data(), &server_tick_be, sizeof(uint32_t));
        std::memcpy(payload_buffer.data() + 4, &entity_count_be, sizeof(uint16_t));
        for (size_t i = 0; i < entities.size(); ++i)
            encode_entity_state(entities[i], payload_buffer.data() + 6 + i * sizeof(EntityState));
        return encode_packet(PacketType::SERVER_SNAPSHOT, payload_buffer.data(), payload_size, sequence_number);
    }

    /**
     * @brief Decode ServerSnapshotPayload with entity array
     */
    static std::pair<ServerSnapshotPayload, std::vector<EntityState>>
    decode_server_snapshot(const uint8_t* buffer, size_t buffer_size) {
        if (buffer_size < sizeof(ServerSnapshotPayload))
            throw std::invalid_argument("Buffer too small for snapshot payload");
        ServerSnapshotPayload header;
        uint32_t server_tick_be;
        uint16_t entity_count_be;

        std::memcpy(&server_tick_be, buffer, sizeof(uint32_t));
        std::memcpy(&entity_count_be, buffer + 4, sizeof(uint16_t));
        header.server_tick = ntohl(server_tick_be);
        header.entity_count = ntohs(entity_count_be);
        std::vector<EntityState> entities;
        entities.reserve(header.entity_count);
        for (size_t i = 0; i < header.entity_count; ++i) {
            size_t offset = 6 + i * sizeof(EntityState);
            if (offset + sizeof(EntityState) > buffer_size)
                throw std::invalid_argument("Buffer too small for all entities");
            entities.push_back(decode_entity_state(buffer + offset));
        }
        return {header, entities};
    }

    /**
     * @brief Encode ClientJoinLobbyPayload with byte order conversion
     */
    static void encode_client_join_lobby(const ClientJoinLobbyPayload& payload, uint8_t* buffer) {
        uint32_t player_id_be = htonl(payload.player_id);
        std::memcpy(buffer, &player_id_be, sizeof(uint32_t));
        buffer[4] = static_cast<uint8_t>(payload.game_mode);
        buffer[5] = static_cast<uint8_t>(payload.difficulty);
    }

    /**
     * @brief Decode ClientJoinLobbyPayload with byte order conversion
     */
    static ClientJoinLobbyPayload decode_client_join_lobby(const uint8_t* buffer) {
        ClientJoinLobbyPayload payload;
        uint32_t player_id_be;
        std::memcpy(&player_id_be, buffer, sizeof(uint32_t));
        payload.player_id = ntohl(player_id_be);
        payload.game_mode = static_cast<GameMode>(buffer[4]);
        payload.difficulty = static_cast<Difficulty>(buffer[5]);
        return payload;
    }

    /**
     * @brief Encode ServerLobbyStatePayload with player array
     */
    static std::vector<uint8_t>
    encode_server_lobby_state(const ServerLobbyStatePayload& header,
                              const std::vector<PlayerLobbyEntry>& players,
                              uint32_t sequence_number) {
        size_t payload_size = sizeof(ServerLobbyStatePayload) + players.size() * sizeof(PlayerLobbyEntry);
        std::vector<uint8_t> payload_buffer(payload_size);
        uint32_t lobby_id_be = htonl(header.lobby_id);

        std::memcpy(payload_buffer.data(), &lobby_id_be, sizeof(uint32_t));
        payload_buffer[4] = static_cast<uint8_t>(header.game_mode);
        payload_buffer[5] = static_cast<uint8_t>(header.difficulty);
        payload_buffer[6] = header.current_player_count;
        payload_buffer[7] = header.required_player_count;
        for (size_t i = 0; i < players.size(); ++i) {
            size_t offset = 8 + i * sizeof(PlayerLobbyEntry);
            uint32_t player_id_be = htonl(players[i].player_id);
            uint16_t player_level_be = htons(players[i].player_level);
            std::memcpy(payload_buffer.data() + offset, &player_id_be, sizeof(uint32_t));
            std::memcpy(payload_buffer.data() + offset + 4, players[i].player_name, 32);
            std::memcpy(payload_buffer.data() + offset + 36, &player_level_be, sizeof(uint16_t));
            payload_buffer[offset + 38] = players[i].skin_id;
        }
        return encode_packet(PacketType::SERVER_LOBBY_STATE, payload_buffer.data(), payload_size, sequence_number);
    }
};

}
