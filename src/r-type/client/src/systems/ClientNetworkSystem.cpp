#include "systems/ClientNetworkSystem.hpp"
#include "network/protocol/ProtocolEncoder.hpp"
#include "network/protocol/PacketTypes.hpp"
#include "network/protocol/Payloads.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/events/InputEvents.hpp"
#include <iostream>
#include <cstring>

namespace rtype::client {

using namespace rtype::protocol;

void ClientNetworkSystem::init(Registry& registry) {
    std::cout << "[ClientNetworkSystem] Connecting to " << server_host_ << ":" << server_port_ << "..." << std::endl;

    // Connect to server
    if (!network_plugin_->connect(server_host_, server_port_)) {
        std::cerr << "[ClientNetworkSystem] Failed to connect to server!" << std::endl;
        return;
    }

    std::cout << "[ClientNetworkSystem] Connected to server!" << std::endl;

    // Subscribe to movement events to track input state
    auto& eventBus = registry.get_event_bus();

    eventBus.subscribe<ecs::PlayerMoveEvent>([this](const ecs::PlayerMoveEvent& event) {
        // Update input flags based on movement
        // Clear movement flags first (but keep other flags like SHOOT)
        current_input_flags_ &= ~(INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT);

        // Set movement flags based on current movement
        if (event.directionY < 0) current_input_flags_ |= INPUT_UP;
        if (event.directionY > 0) current_input_flags_ |= INPUT_DOWN;
        if (event.directionX < 0) current_input_flags_ |= INPUT_LEFT;
        if (event.directionX > 0) current_input_flags_ |= INPUT_RIGHT;
    });

    eventBus.subscribe<ecs::PlayerFireEvent>([this](const ecs::PlayerFireEvent&) {
        current_input_flags_ |= INPUT_SHOOT;
    });

    // Send CLIENT_CONNECT packet
    sendClientConnect();
}

void ClientNetworkSystem::update(Registry& registry, float dt) {
    client_tick_++;

    // Receive and process packets
    handleIncomingPackets(registry);

    // Send input periodically if connected
    if (connected_) {
        input_send_timer_ += dt;
        if (input_send_timer_ >= INPUT_SEND_RATE) {
            sendClientInput(registry);
            input_send_timer_ = 0.0f;
        }
    }
}

void ClientNetworkSystem::shutdown() {
    if (network_plugin_->is_connected()) {
        // TODO: Send CLIENT_DISCONNECT
        network_plugin_->disconnect();
    }
}

void ClientNetworkSystem::handleIncomingPackets(Registry& registry) {
    auto packets = network_plugin_->receive();

    for (const auto& packet : packets) {
        if (packet.data.empty()) {
            continue;
        }

        // Validate packet
        if (!ProtocolEncoder::validate_packet(packet.data.data(), packet.data.size())) {
            std::cerr << "[ClientNetworkSystem] Invalid packet from server" << std::endl;
            continue;
        }

        // Decode header
        PacketHeader header = ProtocolEncoder::decode_header(packet.data.data(), packet.data.size());
        const uint8_t* payload = ProtocolEncoder::get_payload(packet.data.data(), packet.data.size());

        PacketType type = static_cast<PacketType>(header.type);

        switch (type) {
            case PacketType::SERVER_ACCEPT:
                handleServerAccept(registry, payload);
                break;

            case PacketType::SERVER_SNAPSHOT:
                handleServerSnapshot(registry, payload);
                break;

            case PacketType::SERVER_REJECT:
                std::cerr << "[ClientNetworkSystem] Connection rejected by server!" << std::endl;
                break;

            default:
                std::cout << "[ClientNetworkSystem] Unhandled packet type: 0x"
                          << std::hex << static_cast<int>(header.type) << std::dec << std::endl;
                break;
        }
    }
}

void ClientNetworkSystem::handleServerAccept(Registry& registry, const uint8_t* payload) {
    (void)registry;

    ServerAcceptPayload accept_payload;
    std::memcpy(&accept_payload, payload, sizeof(ServerAcceptPayload));

    // Convert from network byte order
    player_id_ = ntohl(accept_payload.assigned_player_id);
    uint8_t tickrate = accept_payload.server_tick_rate;
    uint8_t max_players = accept_payload.max_players;

    connected_ = true;

    std::cout << "[ClientNetworkSystem] SERVER_ACCEPT received!" << std::endl;
    std::cout << "  Assigned Player ID: " << player_id_ << std::endl;
    std::cout << "  Server Tickrate: " << static_cast<int>(tickrate) << " Hz" << std::endl;
    std::cout << "  Max Players: " << static_cast<int>(max_players) << std::endl;
}

void ClientNetworkSystem::handleServerSnapshot(Registry& registry, const uint8_t* payload) {
    ServerSnapshotPayload snapshot_header;
    std::memcpy(&snapshot_header, payload, sizeof(ServerSnapshotPayload));

    uint32_t server_tick = ntohl(snapshot_header.server_tick);
    uint16_t entity_count = ntohs(snapshot_header.entity_count);

    (void)server_tick; // Unused for now

    std::cout << "[ClientNetworkSystem] Snapshot received: " << entity_count << " entities, my player_id=" << player_id_ << std::endl;

    // Get component references
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();

    // Parse entity states
    const uint8_t* entity_data = payload + sizeof(ServerSnapshotPayload);
    for (uint16_t i = 0; i < entity_count; i++) {
        EntityState entity_state;
        std::memcpy(&entity_state, entity_data + (i * sizeof(EntityState)), sizeof(EntityState));

        uint32_t entity_id = ntohl(entity_state.entity_id);
        float pos_x = entity_state.position_x;
        float pos_y = entity_state.position_y;
        int16_t vel_x = entity_state.velocity_x;
        int16_t vel_y = entity_state.velocity_y;

        bool is_own_player = (entity_id == player_id_);
        std::cout << "[ClientNetworkSystem] Entity " << entity_id << " at (" << pos_x << ", " << pos_y << ")";

        if (is_own_player) {
            std::cout << " -> OWN PLAYER" << std::endl;
        } else {
            std::cout << " -> remote player" << std::endl;
        }

        // Check if we already have this player entity
        auto it = remote_players_.find(entity_id);
        if (it == remote_players_.end()) {
            // Create new player entity (either own or remote)
            Entity player_entity = registry.spawn_entity();
            remote_players_[entity_id] = player_entity;

            // Add Position and Velocity components
            registry.add_component(player_entity, Position{pos_x, pos_y});
            registry.add_component(player_entity, Velocity{static_cast<float>(vel_x), static_cast<float>(vel_y)});

            // Add Sprite component with different colors for own vs remote players
            if (remote_player_texture_ != 0) {
                engine::Color sprite_color = is_own_player
                    ? engine::Color{255, 255, 255, 255}  // White for own player
                    : engine::Color{100, 255, 100, 255}; // Green tint for remote players

                registry.add_component(player_entity, Sprite{
                    remote_player_texture_,
                    sprite_width_,
                    sprite_height_,
                    0.0f,  // rotation
                    sprite_color,
                    0.0f,  // origin_x
                    0.0f,  // origin_y
                    1      // layer
                });
            }

            std::cout << "[ClientNetworkSystem] Created "
                      << (is_own_player ? "OWN" : "REMOTE")
                      << " player " << entity_id
                      << " at (" << pos_x << ", " << pos_y << ")" << std::endl;
        } else {
            // Update existing player entity
            Entity player_entity = it->second;

            // Update position and velocity
            try {
                Position& pos = positions[player_entity];
                pos.x = pos_x;
                pos.y = pos_y;

                Velocity& vel = velocities[player_entity];
                vel.x = static_cast<float>(vel_x);
                vel.y = static_cast<float>(vel_y);
            } catch (...) {
                // Entity doesn't have these components
            }
        }
    }
}

void ClientNetworkSystem::sendClientConnect() {
    ClientConnectPayload payload;
    payload.client_version = 0x01;
    std::strncpy(payload.player_name, "Player", sizeof(payload.player_name) - 1);
    payload.player_name[sizeof(payload.player_name) - 1] = '\0';

    uint8_t payload_buffer[sizeof(ClientConnectPayload)];
    std::memcpy(payload_buffer, &payload, sizeof(ClientConnectPayload));

    auto packet_data = ProtocolEncoder::encode_packet(
        PacketType::CLIENT_CONNECT,
        payload_buffer,
        sizeof(ClientConnectPayload),
        sequence_number_++
    );

    ::engine::NetworkPacket packet(packet_data);
    network_plugin_->send(packet);

    std::cout << "[ClientNetworkSystem] Sent CLIENT_CONNECT" << std::endl;
}

void ClientNetworkSystem::sendClientInput(Registry& registry) {
    (void)registry;

    ClientInputPayload payload;
    payload.player_id = htonl(player_id_);
    payload.client_tick = htonl(client_tick_);
    payload.input_flags = htons(current_input_flags_);  // Send captured input

    uint8_t payload_buffer[sizeof(ClientInputPayload)];
    std::memcpy(payload_buffer, &payload, sizeof(ClientInputPayload));

    auto packet_data = ProtocolEncoder::encode_packet(
        PacketType::CLIENT_INPUT,
        payload_buffer,
        sizeof(ClientInputPayload),
        sequence_number_++
    );

    ::engine::NetworkPacket packet(packet_data);
    network_plugin_->send(packet);

    // Reset SHOOT flag after sending (it's a one-time action)
    current_input_flags_ &= ~INPUT_SHOOT;
}

}
