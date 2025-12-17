/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** NetworkClient - Implementation
*/

#include "NetworkClient.hpp"
#include "protocol/ProtocolEncoder.hpp"
#include <iostream>
#include <chrono>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

namespace rtype::client {

NetworkClient::NetworkClient(engine::INetworkPlugin& plugin)
    : network_plugin_(plugin) {
}

NetworkClient::~NetworkClient() {
    disconnect();
}

// ============== Connection ==============

bool NetworkClient::connect(const std::string& host, uint16_t port) {
    server_host_ = host;
    tcp_port_ = port;

    std::cout << "[NetworkClient] Connecting to " << host << ":" << port << " via TCP...\n";

    if (!network_plugin_.connect_tcp(host, port)) {
        std::cerr << "[NetworkClient] Failed to connect to server\n";
        return false;
    }

    network_plugin_.set_on_disconnected([this]() {
        std::cout << "[NetworkClient] Disconnected from server\n";
        in_lobby_ = false;
        in_game_ = false;
        if (on_disconnected_)
            on_disconnected_();
    });

    std::cout << "[NetworkClient] TCP connected successfully\n";
    return true;
}

void NetworkClient::disconnect() {
    if (network_plugin_.is_tcp_connected()) {
        send_disconnect();
        network_plugin_.disconnect();
    }
    player_id_ = 0;
    session_id_ = 0;
    lobby_id_ = 0;
    in_lobby_ = false;
    in_game_ = false;
}

bool NetworkClient::is_tcp_connected() const {
    return network_plugin_.is_tcp_connected();
}

bool NetworkClient::is_udp_connected() const {
    return network_plugin_.is_udp_connected();
}

// ============== Sending ==============

void NetworkClient::send_connect(const std::string& player_name) {
    protocol::ClientConnectPayload payload;
    payload.client_version = 0x01;
    payload.set_player_name(player_name);

    std::cout << "[NetworkClient] Sending connect request as '" << player_name << "'\n";
    send_tcp_packet(protocol::PacketType::CLIENT_CONNECT, serialize_payload(&payload, sizeof(payload)));
}

void NetworkClient::send_disconnect() {
    protocol::ClientDisconnectPayload payload;
    payload.player_id = htonl(player_id_);
    payload.reason = protocol::DisconnectReason::USER_QUIT;

    send_tcp_packet(protocol::PacketType::CLIENT_DISCONNECT, serialize_payload(&payload, sizeof(payload)));
}

void NetworkClient::send_ping() {
    protocol::ClientPingPayload payload;
    payload.player_id = htonl(player_id_);

    last_ping_timestamp_ = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
    payload.client_timestamp = htonl(last_ping_timestamp_);

    send_tcp_packet(protocol::PacketType::CLIENT_PING, serialize_payload(&payload, sizeof(payload)));
}

void NetworkClient::send_join_lobby(protocol::GameMode mode, protocol::Difficulty difficulty) {
    protocol::ClientJoinLobbyPayload payload;
    payload.player_id = htonl(player_id_);
    payload.game_mode = mode;
    payload.difficulty = difficulty;

    std::cout << "[NetworkClient] Requesting to join lobby (mode=" << static_cast<int>(mode)
              << ", difficulty=" << static_cast<int>(difficulty) << ")\n";
    send_tcp_packet(protocol::PacketType::CLIENT_JOIN_LOBBY, serialize_payload(&payload, sizeof(payload)));
}

void NetworkClient::send_leave_lobby() {
    protocol::ClientLeaveLobbyPayload payload;
    payload.player_id = htonl(player_id_);
    payload.lobby_id = htonl(lobby_id_);

    std::cout << "[NetworkClient] Leaving lobby " << lobby_id_ << "\n";
    send_tcp_packet(protocol::PacketType::CLIENT_LEAVE_LOBBY, serialize_payload(&payload, sizeof(payload)));
}

void NetworkClient::send_input(uint16_t input_flags, uint32_t client_tick) {
    protocol::ClientInputPayload payload;
    payload.player_id = htonl(player_id_);
    payload.input_flags = htons(input_flags);
    payload.client_tick = htonl(client_tick);

    // Send via UDP if connected, otherwise TCP
    if (network_plugin_.is_udp_connected()) {
        send_udp_packet(protocol::PacketType::CLIENT_INPUT, serialize_payload(&payload, sizeof(payload)));
    } else {
        send_tcp_packet(protocol::PacketType::CLIENT_INPUT, serialize_payload(&payload, sizeof(payload)));
    }
}

// ============== Update ==============

void NetworkClient::update() {
    auto packets = network_plugin_.receive();

    static int packet_count = 0;
    static int log_counter = 0;
    packet_count += packets.size();
    log_counter++;

    if (log_counter % 60 == 0 && packet_count > 0) {
        std::cout << "[NetworkClient] Received " << packet_count << " packets in last 60 updates\n";
        packet_count = 0;
    }

    for (const auto& packet : packets) {
        handle_packet(packet);
    }
}

void NetworkClient::handle_packet(const engine::NetworkPacket& packet) {
    if (packet.data.size() < protocol::HEADER_SIZE) {
        std::cerr << "[NetworkClient] Packet too small\n";
        return;
    }

    protocol::PacketHeader header = protocol::ProtocolEncoder::decode_header(
        packet.data.data(), packet.data.size());

    if (header.version != 0x01) {
        std::cerr << "[NetworkClient] Invalid protocol version: " << static_cast<int>(header.version) << "\n";
        return;
    }

    std::vector<uint8_t> payload(
        packet.data.begin() + protocol::HEADER_SIZE,
        packet.data.begin() + protocol::HEADER_SIZE + header.payload_length
    );

    auto packet_type = static_cast<protocol::PacketType>(header.type);

    // Log all non-snapshot packets to debug spawn issues
    if (packet_type != protocol::PacketType::SERVER_SNAPSHOT &&
        packet_type != protocol::PacketType::SERVER_DELTA_SNAPSHOT) {
        std::cout << "[NetworkClient] Processing packet type: " << (int)packet_type << "\n";
    }

    switch (packet_type) {
        case protocol::PacketType::SERVER_ACCEPT:
            handle_server_accept(payload);
            break;
        case protocol::PacketType::SERVER_REJECT:
            handle_server_reject(payload);
            break;
        case protocol::PacketType::SERVER_PONG:
            handle_server_pong(payload);
            break;
        case protocol::PacketType::SERVER_LOBBY_STATE:
            handle_lobby_state(payload);
            break;
        case protocol::PacketType::SERVER_GAME_START_COUNTDOWN:
            handle_countdown(payload);
            break;
        case protocol::PacketType::SERVER_GAME_START:
            handle_game_start(payload);
            break;
        case protocol::PacketType::SERVER_ENTITY_SPAWN:
            handle_entity_spawn(payload);
            break;
        case protocol::PacketType::SERVER_ENTITY_DESTROY:
            handle_entity_destroy(payload);
            break;
        case protocol::PacketType::SERVER_PROJECTILE_SPAWN:
            handle_projectile_spawn(payload);
            break;
        case protocol::PacketType::SERVER_WAVE_START:
            handle_wave_start(payload);
            break;
        case protocol::PacketType::SERVER_WAVE_COMPLETE:
            handle_wave_complete(payload);
            break;
        case protocol::PacketType::SERVER_GAME_OVER:
            handle_game_over(payload);
            break;
        case protocol::PacketType::SERVER_SNAPSHOT:
        case protocol::PacketType::SERVER_DELTA_SNAPSHOT:
            if (in_game_) {
                handle_snapshot(payload);
            }
            break;
        default:
            std::cout << "[NetworkClient] Unhandled packet type: 0x" << std::hex
                      << static_cast<int>(header.type) << std::dec << "\n";
            break;
    }
}

// ============== Packet Handlers ==============

void NetworkClient::handle_server_accept(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerAcceptPayload)) {
        std::cerr << "[NetworkClient] Invalid SERVER_ACCEPT payload\n";
        return;
    }

    protocol::ServerAcceptPayload accept;
    std::memcpy(&accept, payload.data(), sizeof(accept));

    player_id_ = ntohl(accept.assigned_player_id);
    std::cout << "[NetworkClient] Connection accepted! Player ID: " << player_id_ << "\n";
    std::cout << "[NetworkClient] Server tick rate: " << static_cast<int>(accept.server_tick_rate)
              << ", Max players: " << static_cast<int>(accept.max_players) << "\n";

    if (on_accepted_)
        on_accepted_(player_id_);
}

void NetworkClient::handle_server_reject(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerRejectPayload)) {
        std::cerr << "[NetworkClient] Invalid SERVER_REJECT payload\n";
        return;
    }

    protocol::ServerRejectPayload reject;
    std::memcpy(&reject, payload.data(), sizeof(reject));

    std::string message(reject.reason_message);
    std::cerr << "[NetworkClient] Connection rejected: " << message << "\n";

    if (on_rejected_)
        on_rejected_(static_cast<uint8_t>(reject.reason_code), message);
}

void NetworkClient::handle_server_pong(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerPongPayload)) {
        return;
    }

    protocol::ServerPongPayload pong;
    std::memcpy(&pong, payload.data(), sizeof(pong));

    uint32_t now = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );

    uint32_t client_timestamp = ntohl(pong.client_timestamp);
    server_ping_ms_ = static_cast<int>(now - client_timestamp);
}

void NetworkClient::handle_lobby_state(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerLobbyStatePayload)) {
        std::cerr << "[NetworkClient] Invalid SERVER_LOBBY_STATE payload\n";
        return;
    }

    protocol::ServerLobbyStatePayload state;
    std::memcpy(&state, payload.data(), sizeof(state));
    state.lobby_id = ntohl(state.lobby_id);

    lobby_id_ = state.lobby_id;
    in_lobby_ = true;

    std::vector<protocol::PlayerLobbyEntry> players;
    size_t offset = sizeof(protocol::ServerLobbyStatePayload);
    size_t remaining = 0;
    if (payload.size() > offset)
        remaining = payload.size() - offset;
    size_t max_entries = remaining / sizeof(protocol::PlayerLobbyEntry);
    size_t expected = std::min(static_cast<size_t>(state.current_player_count), max_entries);

    for (size_t i = 0; i < expected; ++i) {
        protocol::PlayerLobbyEntry entry;
        std::memcpy(&entry, payload.data() + offset + i * sizeof(protocol::PlayerLobbyEntry),
                    sizeof(protocol::PlayerLobbyEntry));
        entry.player_id = ntohl(entry.player_id);
        entry.player_level = ntohs(entry.player_level);
        players.push_back(entry);
    }

    std::cout << "[NetworkClient] Lobby state - ID: " << lobby_id_
              << ", Players: " << static_cast<int>(state.current_player_count)
              << "/" << static_cast<int>(state.required_player_count) << "\n";

    if (on_lobby_state_)
        on_lobby_state_(state, players);
}

void NetworkClient::handle_countdown(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerGameStartCountdownPayload)) {
        return;
    }

    protocol::ServerGameStartCountdownPayload countdown;
    std::memcpy(&countdown, payload.data(), sizeof(countdown));

    std::cout << "[NetworkClient] Game starting in " << static_cast<int>(countdown.countdown_value) << "s\n";

    if (on_countdown_)
        on_countdown_(countdown.countdown_value);
}

void NetworkClient::handle_game_start(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerGameStartPayload)) {
        std::cerr << "[NetworkClient] Invalid SERVER_GAME_START payload\n";
        return;
    }

    protocol::ServerGameStartPayload game_start;
    std::memcpy(&game_start, payload.data(), sizeof(game_start));

    session_id_ = ntohl(game_start.game_session_id);
    udp_port_ = ntohs(game_start.udp_port);
    in_lobby_ = false;
    in_game_ = true;

    std::cout << "[NetworkClient] Game started! Session: " << session_id_
              << ", UDP port: " << udp_port_ << "\n";

    // Connect UDP for gameplay
    connect_udp(udp_port_);

    if (on_game_start_)
        on_game_start_(session_id_, udp_port_);
}

void NetworkClient::handle_entity_spawn(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerEntitySpawnPayload)) {
        return;
    }

    protocol::ServerEntitySpawnPayload spawn;
    std::memcpy(&spawn, payload.data(), sizeof(spawn));

    if (on_entity_spawn_)
        on_entity_spawn_(spawn);
}

void NetworkClient::handle_entity_destroy(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerEntityDestroyPayload)) {
        return;
    }

    protocol::ServerEntityDestroyPayload destroy;
    std::memcpy(&destroy, payload.data(), sizeof(destroy));

    if (on_entity_destroy_)
        on_entity_destroy_(destroy);
}

void NetworkClient::handle_projectile_spawn(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerProjectileSpawnPayload)) {
        return;
    }

    protocol::ServerProjectileSpawnPayload proj_spawn;
    std::memcpy(&proj_spawn, payload.data(), sizeof(proj_spawn));

    if (on_projectile_spawn_)
        on_projectile_spawn_(proj_spawn);
}

void NetworkClient::handle_snapshot(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerSnapshotPayload)) {
        return;
    }

    protocol::ServerSnapshotPayload snapshot;
    std::memcpy(&snapshot, payload.data(), sizeof(snapshot));

    // Calculate how many entities are in this snapshot
    size_t header_size = sizeof(protocol::ServerSnapshotPayload);
    size_t remaining = payload.size() - header_size;
    size_t entity_count = remaining / sizeof(protocol::EntityState);

    // Extract entity states
    std::vector<protocol::EntityState> entities;
    entities.reserve(entity_count);

    const uint8_t* entity_data = payload.data() + header_size;
    for (size_t i = 0; i < entity_count; ++i) {
        protocol::EntityState state;
        std::memcpy(&state, entity_data + (i * sizeof(protocol::EntityState)), sizeof(protocol::EntityState));
        entities.push_back(state);
    }

    if (on_snapshot_)
        on_snapshot_(snapshot, entities);
}

void NetworkClient::handle_game_over(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerGameOverPayload)) {
        return;
    }

    protocol::ServerGameOverPayload game_over;
    std::memcpy(&game_over, payload.data(), sizeof(game_over));

    in_game_ = false;
    std::cout << "[NetworkClient] Game over! Result: "
              << (game_over.result == protocol::GameResult::VICTORY ? "Victory" : "Defeat") << "\n";

    if (on_game_over_)
        on_game_over_(game_over);
}

void NetworkClient::handle_wave_start(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerWaveStartPayload)) {
        return;
    }

    protocol::ServerWaveStartPayload wave_start;
    std::memcpy(&wave_start, payload.data(), sizeof(wave_start));

    if (on_wave_start_)
        on_wave_start_(wave_start);
}

void NetworkClient::handle_wave_complete(const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(protocol::ServerWaveCompletePayload)) {
        return;
    }

    protocol::ServerWaveCompletePayload wave_complete;
    std::memcpy(&wave_complete, payload.data(), sizeof(wave_complete));

    if (on_wave_complete_)
        on_wave_complete_(wave_complete);
}

// ============== UDP Connection ==============

void NetworkClient::connect_udp(uint16_t udp_port) {
    std::cout << "[NetworkClient] Connecting UDP to " << server_host_ << ":" << udp_port << "...\n";

    if (!network_plugin_.connect_udp(server_host_, udp_port)) {
        std::cerr << "[NetworkClient] Failed to connect UDP\n";
        return;
    }

    std::cout << "[NetworkClient] UDP connected, sending handshake...\n";
    send_udp_handshake();
}

void NetworkClient::send_udp_handshake() {
    protocol::ClientUdpHandshakePayload payload;
    payload.player_id = htonl(player_id_);
    payload.session_id = htonl(session_id_);

    send_udp_packet(protocol::PacketType::CLIENT_UDP_HANDSHAKE,
                    serialize_payload(&payload, sizeof(payload)));

    std::cout << "[NetworkClient] UDP handshake sent (player=" << player_id_
              << ", session=" << session_id_ << ")\n";
}

// ============== Packet Building ==============

void NetworkClient::send_tcp_packet(protocol::PacketType type, const std::vector<uint8_t>& payload) {
    if (!network_plugin_.is_tcp_connected()) {
        std::cerr << "[NetworkClient] Cannot send TCP: not connected\n";
        return;
    }

    protocol::PacketHeader header;
    header.version = 0x01;
    header.type = static_cast<uint8_t>(type);
    header.payload_length = static_cast<uint16_t>(payload.size());
    header.sequence_number = 0;

    std::vector<uint8_t> packet_data(protocol::HEADER_SIZE);
    protocol::ProtocolEncoder::encode_header(header, packet_data.data());
    packet_data.insert(packet_data.end(), payload.begin(), payload.end());

    engine::NetworkPacket packet;
    packet.data = packet_data;
    network_plugin_.send_tcp(packet);
}

void NetworkClient::send_udp_packet(protocol::PacketType type, const std::vector<uint8_t>& payload) {
    if (!network_plugin_.is_udp_connected()) {
        std::cerr << "[NetworkClient] Cannot send UDP: not connected\n";
        return;
    }

    protocol::PacketHeader header;
    header.version = 0x01;
    header.type = static_cast<uint8_t>(type);
    header.payload_length = static_cast<uint16_t>(payload.size());
    header.sequence_number = 0;

    std::vector<uint8_t> packet_data(protocol::HEADER_SIZE);
    protocol::ProtocolEncoder::encode_header(header, packet_data.data());
    packet_data.insert(packet_data.end(), payload.begin(), payload.end());

    engine::NetworkPacket packet;
    packet.data = packet_data;
    network_plugin_.send_udp(packet);
}

std::vector<uint8_t> NetworkClient::serialize_payload(const void* payload, size_t size) {
    const uint8_t* bytes = static_cast<const uint8_t*>(payload);
    return std::vector<uint8_t>(bytes, bytes + size);
}

// ============== Callbacks ==============

void NetworkClient::set_on_accepted(std::function<void(uint32_t)> callback) {
    on_accepted_ = callback;
}

void NetworkClient::set_on_rejected(std::function<void(uint8_t, const std::string&)> callback) {
    on_rejected_ = callback;
}

void NetworkClient::set_on_lobby_state(std::function<void(const protocol::ServerLobbyStatePayload&,
                                                         const std::vector<protocol::PlayerLobbyEntry>&)> callback) {
    on_lobby_state_ = callback;
}

void NetworkClient::set_on_countdown(std::function<void(uint8_t)> callback) {
    on_countdown_ = callback;
}

void NetworkClient::set_on_game_start(std::function<void(uint32_t, uint16_t)> callback) {
    on_game_start_ = callback;
}

void NetworkClient::set_on_entity_spawn(std::function<void(const protocol::ServerEntitySpawnPayload&)> callback) {
    on_entity_spawn_ = callback;
}

void NetworkClient::set_on_entity_destroy(std::function<void(const protocol::ServerEntityDestroyPayload&)> callback) {
    on_entity_destroy_ = callback;
}

void NetworkClient::set_on_projectile_spawn(std::function<void(const protocol::ServerProjectileSpawnPayload&)> callback) {
    on_projectile_spawn_ = callback;
}

void NetworkClient::set_on_snapshot(std::function<void(const protocol::ServerSnapshotPayload&, const std::vector<protocol::EntityState>&)> callback) {
    on_snapshot_ = callback;
}

void NetworkClient::set_on_game_over(std::function<void(const protocol::ServerGameOverPayload&)> callback) {
    on_game_over_ = callback;
}

void NetworkClient::set_on_disconnected(std::function<void()> callback) {
    on_disconnected_ = callback;
}

void NetworkClient::set_on_wave_start(std::function<void(const protocol::ServerWaveStartPayload&)> callback) {
    on_wave_start_ = callback;
}

void NetworkClient::set_on_wave_complete(std::function<void(const protocol::ServerWaveCompletePayload&)> callback) {
    on_wave_complete_ = callback;
}

}
