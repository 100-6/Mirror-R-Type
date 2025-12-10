#include <gtest/gtest.h>
#include "../src/common/protocol/PacketHeader.hpp"
#include "../src/common/protocol/PacketTypes.hpp"
#include "../src/common/protocol/Payloads.hpp"
#include "../src/common/protocol/ProtocolEncoder.hpp"
#include <cstring>

using namespace rtype::protocol;

// ========== PacketHeader Tests ==========

TEST(PacketHeaderTest, DefaultConstructor) {
    PacketHeader header;
    EXPECT_EQ(header.version, PROTOCOL_VERSION);
    EXPECT_EQ(header.type, 0);
    EXPECT_EQ(header.payload_length, 0);
    EXPECT_EQ(header.sequence_number, 0);
}

TEST(PacketHeaderTest, ParameterizedConstructor) {
    PacketHeader header(0x10, 100, 42);
    EXPECT_EQ(header.version, PROTOCOL_VERSION);
    EXPECT_EQ(header.type, 0x10);
    EXPECT_EQ(header.payload_length, 100);
    EXPECT_EQ(header.sequence_number, 42);
}

TEST(PacketHeaderTest, IsValid) {
    PacketHeader valid_header(0x10, 100, 1);
    EXPECT_TRUE(valid_header.is_valid());

    PacketHeader oversized_header(0x10, MAX_PAYLOAD_SIZE + 1, 1);
    EXPECT_FALSE(oversized_header.is_valid());

    PacketHeader wrong_version;
    wrong_version.version = 0x99;
    EXPECT_FALSE(wrong_version.is_valid());
}

TEST(PacketHeaderTest, TotalSize) {
    PacketHeader header(0x10, 100, 1);
    EXPECT_EQ(header.total_size(), HEADER_SIZE + 100);
}

TEST(PacketHeaderTest, SizeAssertion) {
    EXPECT_EQ(sizeof(PacketHeader), 8);
}

// ========== Encoder/Decoder Tests ==========

TEST(ProtocolEncoderTest, EncodeDecodeHeader) {
    PacketHeader original(0x10, 256, 123456);

    uint8_t buffer[HEADER_SIZE];
    ProtocolEncoder::encode_header(original, buffer);

    PacketHeader decoded = ProtocolEncoder::decode_header(buffer, HEADER_SIZE);

    EXPECT_EQ(decoded.version, original.version);
    EXPECT_EQ(decoded.type, original.type);
    EXPECT_EQ(decoded.payload_length, original.payload_length);
    EXPECT_EQ(decoded.sequence_number, original.sequence_number);
}

TEST(ProtocolEncoderTest, ValidatePacket) {
    std::vector<uint8_t> packet = ProtocolEncoder::encode_packet(
        PacketType::CLIENT_PING, nullptr, 0, 1);

    EXPECT_TRUE(ProtocolEncoder::validate_packet(packet.data(), packet.size()));
}

TEST(ProtocolEncoderTest, ValidateInvalidPacket) {
    uint8_t invalid_buffer[4] = {0x99, 0x01, 0x00, 0x00}; // Wrong version
    EXPECT_FALSE(ProtocolEncoder::validate_packet(invalid_buffer, 4));

    uint8_t too_small[2] = {0x01, 0x01};
    EXPECT_FALSE(ProtocolEncoder::validate_packet(too_small, 2));
}

TEST(ProtocolEncoderTest, EncodePacketTooLarge) {
    std::vector<uint8_t> huge_payload(MAX_PAYLOAD_SIZE + 1);
    EXPECT_THROW(
        ProtocolEncoder::encode_packet(PacketType::CLIENT_CONNECT, huge_payload.data(),
                                        huge_payload.size(), 1),
        std::invalid_argument);
}

// ========== Payload Tests ==========

TEST(PayloadTest, ClientConnectSize) {
    EXPECT_EQ(sizeof(ClientConnectPayload), 33);
}

TEST(PayloadTest, ClientConnectEncodeDecode) {
    ClientConnectPayload original;
    original.set_player_name("TestPlayer");

    uint8_t buffer[sizeof(ClientConnectPayload)];
    ProtocolEncoder::encode_client_connect(original, buffer);

    ClientConnectPayload decoded = ProtocolEncoder::decode_client_connect(buffer);

    EXPECT_EQ(decoded.client_version, original.client_version);
    EXPECT_STREQ(decoded.player_name, "TestPlayer");
}

TEST(PayloadTest, ClientInputSize) {
    EXPECT_EQ(sizeof(ClientInputPayload), 10);
}

TEST(PayloadTest, ClientInputEncodeDecode) {
    ClientInputPayload original;
    original.player_id = 42;
    original.input_flags = INPUT_UP | INPUT_SHOOT;
    original.client_tick = 1000;

    uint8_t buffer[sizeof(ClientInputPayload)];
    ProtocolEncoder::encode_client_input(original, buffer);

    ClientInputPayload decoded = ProtocolEncoder::decode_client_input(buffer);

    EXPECT_EQ(decoded.player_id, 42);
    EXPECT_EQ(decoded.input_flags, INPUT_UP | INPUT_SHOOT);
    EXPECT_EQ(decoded.client_tick, 1000);
    EXPECT_TRUE(decoded.is_up_pressed());
    EXPECT_TRUE(decoded.is_shoot_pressed());
    EXPECT_FALSE(decoded.is_down_pressed());
}

TEST(PayloadTest, ServerAcceptSize) {
    EXPECT_EQ(sizeof(ServerAcceptPayload), 8);
}

TEST(PayloadTest, ServerAcceptEncodeDecode) {
    ServerAcceptPayload original;
    original.assigned_player_id = 123;
    original.server_tick_rate = 60;
    original.max_players = 4;
    original.map_id = 1;

    uint8_t buffer[sizeof(ServerAcceptPayload)];
    ProtocolEncoder::encode_server_accept(original, buffer);

    ServerAcceptPayload decoded = ProtocolEncoder::decode_server_accept(buffer);

    EXPECT_EQ(decoded.assigned_player_id, 123);
    EXPECT_EQ(decoded.server_tick_rate, 60);
    EXPECT_EQ(decoded.max_players, 4);
    EXPECT_EQ(decoded.map_id, 1);
}

TEST(PayloadTest, ClientJoinLobbySize) {
    EXPECT_EQ(sizeof(ClientJoinLobbyPayload), 6);
}

TEST(PayloadTest, ClientJoinLobbyEncodeDecode) {
    ClientJoinLobbyPayload original;
    original.player_id = 42;
    original.game_mode = GameMode::SQUAD;
    original.difficulty = Difficulty::HARD;

    uint8_t buffer[sizeof(ClientJoinLobbyPayload)];
    ProtocolEncoder::encode_client_join_lobby(original, buffer);

    ClientJoinLobbyPayload decoded = ProtocolEncoder::decode_client_join_lobby(buffer);

    EXPECT_EQ(decoded.player_id, 42);
    EXPECT_EQ(decoded.game_mode, GameMode::SQUAD);
    EXPECT_EQ(decoded.difficulty, Difficulty::HARD);
}

TEST(PayloadTest, EntityStateSize) {
    EXPECT_EQ(sizeof(EntityState), 21);
}

TEST(PayloadTest, EntityStateEncodeDecode) {
    EntityState original;
    original.entity_id = 999;
    original.entity_type = EntityType::PLAYER;
    original.position_x = 123.456f;
    original.position_y = 789.012f;
    original.velocity_x = 100;
    original.velocity_y = -50;
    original.health = 100;
    original.flags = ENTITY_INVULNERABLE | ENTITY_CHARGING;

    uint8_t buffer[sizeof(EntityState)];
    ProtocolEncoder::encode_entity_state(original, buffer);

    EntityState decoded = ProtocolEncoder::decode_entity_state(buffer);

    EXPECT_EQ(decoded.entity_id, 999);
    EXPECT_EQ(decoded.entity_type, EntityType::PLAYER);
    EXPECT_FLOAT_EQ(decoded.position_x, 123.456f);
    EXPECT_FLOAT_EQ(decoded.position_y, 789.012f);
    EXPECT_EQ(decoded.velocity_x, 100);
    EXPECT_EQ(decoded.velocity_y, -50);
    EXPECT_EQ(decoded.health, 100);
    EXPECT_EQ(decoded.flags, ENTITY_INVULNERABLE | ENTITY_CHARGING);
    EXPECT_TRUE(decoded.is_invulnerable());
    EXPECT_TRUE(decoded.is_charging());
}

TEST(PayloadTest, ServerSnapshotEncodeDecode) {
    std::vector<EntityState> entities;

    EntityState e1;
    e1.entity_id = 1;
    e1.entity_type = EntityType::PLAYER;
    e1.position_x = 10.0f;
    e1.position_y = 20.0f;
    entities.push_back(e1);

    EntityState e2;
    e2.entity_id = 2;
    e2.entity_type = EntityType::ENEMY_BASIC;
    e2.position_x = 30.0f;
    e2.position_y = 40.0f;
    entities.push_back(e2);

    std::vector<uint8_t> packet =
        ProtocolEncoder::encode_server_snapshot(12345, entities, 1);

    EXPECT_TRUE(ProtocolEncoder::validate_packet(packet.data(), packet.size()));

    const uint8_t* payload = ProtocolEncoder::get_payload(packet.data(), packet.size());
    size_t payload_size = packet.size() - HEADER_SIZE;

    auto [header, decoded_entities] =
        ProtocolEncoder::decode_server_snapshot(payload, payload_size);

    EXPECT_EQ(header.server_tick, 12345);
    EXPECT_EQ(header.entity_count, 2);
    EXPECT_EQ(decoded_entities.size(), 2);
    EXPECT_EQ(decoded_entities[0].entity_id, 1);
    EXPECT_EQ(decoded_entities[1].entity_id, 2);
    EXPECT_FLOAT_EQ(decoded_entities[0].position_x, 10.0f);
    EXPECT_FLOAT_EQ(decoded_entities[1].position_y, 40.0f);
}

TEST(PayloadTest, ServerSnapshotTooManyEntities) {
    // Max is 66 entities: (1392 - 6) / 21 = 66
    std::vector<EntityState> entities(67); // One too many
    EXPECT_THROW(ProtocolEncoder::encode_server_snapshot(1, entities, 1),
                 std::invalid_argument);
}

TEST(PayloadTest, ServerLobbyStateEncodeDecode) {
    ServerLobbyStatePayload header;
    header.lobby_id = 42;
    header.game_mode = GameMode::TRIO;
    header.difficulty = Difficulty::NORMAL;
    header.current_player_count = 2;
    header.required_player_count = 3;

    std::vector<PlayerLobbyEntry> players;
    PlayerLobbyEntry p1;
    p1.player_id = 1;
    p1.set_name("Player1");
    p1.player_level = 10;
    players.push_back(p1);

    PlayerLobbyEntry p2;
    p2.player_id = 2;
    p2.set_name("Player2");
    p2.player_level = 20;
    players.push_back(p2);

    std::vector<uint8_t> packet =
        ProtocolEncoder::encode_server_lobby_state(header, players, 1);

    EXPECT_TRUE(ProtocolEncoder::validate_packet(packet.data(), packet.size()));

    // Verify packet size
    size_t expected_size = HEADER_SIZE + sizeof(ServerLobbyStatePayload) +
                           players.size() * sizeof(PlayerLobbyEntry);
    EXPECT_EQ(packet.size(), expected_size);
}

// ========== Packet Type Tests ==========

TEST(PacketTypeTest, GetRequiredPlayerCount) {
    EXPECT_EQ(get_required_player_count(GameMode::DUO), 2);
    EXPECT_EQ(get_required_player_count(GameMode::TRIO), 3);
    EXPECT_EQ(get_required_player_count(GameMode::SQUAD), 4);
}

TEST(PacketTypeTest, GameModeToString) {
    EXPECT_EQ(game_mode_to_string(GameMode::DUO), "DUO");
    EXPECT_EQ(game_mode_to_string(GameMode::TRIO), "TRIO");
    EXPECT_EQ(game_mode_to_string(GameMode::SQUAD), "SQUAD");
}

TEST(PacketTypeTest, DifficultyToString) {
    EXPECT_EQ(difficulty_to_string(Difficulty::EASY), "EASY");
    EXPECT_EQ(difficulty_to_string(Difficulty::NORMAL), "NORMAL");
    EXPECT_EQ(difficulty_to_string(Difficulty::HARD), "HARD");
}

TEST(PacketTypeTest, PacketTypeToString) {
    EXPECT_EQ(packet_type_to_string(PacketType::CLIENT_CONNECT), "CLIENT_CONNECT");
    EXPECT_EQ(packet_type_to_string(PacketType::SERVER_ACCEPT), "SERVER_ACCEPT");
    EXPECT_EQ(packet_type_to_string(PacketType::CLIENT_INPUT), "CLIENT_INPUT");
    EXPECT_EQ(packet_type_to_string(PacketType::SERVER_SNAPSHOT), "SERVER_SNAPSHOT");
}

// ========== MTU Safety Tests ==========

TEST(MTUSafetyTest, MaxSnapshotSize) {
    // Calculate max entities that fit in one packet
    // MAX_PAYLOAD_SIZE (1392) = ServerSnapshotPayload (6) + N * EntityState (21)
    // N = (1392 - 6) / 21 = 66 entities max
    const size_t max_entities = (MAX_PAYLOAD_SIZE - sizeof(ServerSnapshotPayload)) / sizeof(EntityState);

    std::vector<EntityState> entities(max_entities);
    for (size_t i = 0; i < max_entities; ++i) {
        entities[i].entity_id = i;
    }

    std::vector<uint8_t> packet = ProtocolEncoder::encode_server_snapshot(1, entities, 1);

    EXPECT_LE(packet.size(), MAX_PACKET_SIZE);

    // Also verify the calculated max is correct
    EXPECT_EQ(max_entities, 66); // (1392 - 6) / 21 = 66
}

TEST(MTUSafetyTest, MaxLobbyStateSize) {
    ServerLobbyStatePayload header;
    header.lobby_id = 1;
    header.game_mode = GameMode::SQUAD;
    header.difficulty = Difficulty::NORMAL;
    header.current_player_count = 4;
    header.required_player_count = 4;

    std::vector<PlayerLobbyEntry> players(4);
    for (size_t i = 0; i < 4; ++i) {
        players[i].player_id = i;
        players[i].set_name("LongPlayerName123456789");
        players[i].player_level = 100;
    }

    std::vector<uint8_t> packet =
        ProtocolEncoder::encode_server_lobby_state(header, players, 1);

    EXPECT_LE(packet.size(), MAX_PACKET_SIZE);
}

// ========== Round-Trip Tests ==========

TEST(RoundTripTest, ClientConnect) {
    ClientConnectPayload original;
    original.set_player_name("Alice");

    std::vector<uint8_t> packet = ProtocolEncoder::encode_packet(
        PacketType::CLIENT_CONNECT, &original, sizeof(original), 1);

    EXPECT_TRUE(ProtocolEncoder::validate_packet(packet.data(), packet.size()));

    PacketHeader header = ProtocolEncoder::decode_header(packet.data(), packet.size());
    EXPECT_EQ(header.type, static_cast<uint8_t>(PacketType::CLIENT_CONNECT));

    const uint8_t* payload = ProtocolEncoder::get_payload(packet.data(), packet.size());
    ClientConnectPayload decoded = ProtocolEncoder::decode_client_connect(payload);

    EXPECT_STREQ(decoded.player_name, "Alice");
}

TEST(RoundTripTest, ClientInput) {
    ClientInputPayload original;
    original.player_id = 42;
    original.input_flags = INPUT_RIGHT | INPUT_SHOOT | INPUT_CHARGE;
    original.client_tick = 5000;

    uint8_t buffer[sizeof(ClientInputPayload)];
    ProtocolEncoder::encode_client_input(original, buffer);

    std::vector<uint8_t> packet = ProtocolEncoder::encode_packet(
        PacketType::CLIENT_INPUT, buffer, sizeof(buffer), 100);

    EXPECT_TRUE(ProtocolEncoder::validate_packet(packet.data(), packet.size()));

    const uint8_t* payload = ProtocolEncoder::get_payload(packet.data(), packet.size());
    ClientInputPayload decoded = ProtocolEncoder::decode_client_input(payload);

    EXPECT_EQ(decoded.player_id, 42);
    EXPECT_EQ(decoded.input_flags, INPUT_RIGHT | INPUT_SHOOT | INPUT_CHARGE);
    EXPECT_EQ(decoded.client_tick, 5000);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
