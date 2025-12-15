#include "systems/ServerNetworkSystem.hpp"
#include "network/protocol/ProtocolEncoder.hpp"
#include "network/protocol/PacketTypes.hpp"
#include "network/protocol/Payloads.hpp"
#include "ecs/events/NetworkEvents.hpp"
#include "ecs/CoreComponents.hpp"
#include <iostream>
#include <chrono>
#include <cmath>

namespace rtype::server {

using namespace rtype::protocol;

void ServerNetworkSystem::init(Registry& registry) {
    (void)registry;

    // Start the server on port 4242 (default R-Type port)
    if (!network_plugin_->start_server(4242)) {
        throw std::runtime_error("[ServerNetworkSystem] Failed to start server on port 4242");
    }

    std::cout << "[ServerNetworkSystem] Initialized and listening on port 4242" << std::endl;
}

void ServerNetworkSystem::update(Registry& registry, float dt) {
    // Increment server tick
    server_tick_++;

    // Poll for incoming packets
    auto packets = network_plugin_->receive();
    for (const auto& packet : packets) {
        if (packet.data.empty()) {
            continue;
        }

        // Store or update the client ID for this sender
        // The sender_id is the network plugin's internal client ID
        // We need to create an address string for mapping (we'll use sender_id as address for now)
        std::string client_address = std::to_string(packet.sender_id);
        address_to_client_id_[client_address] = packet.sender_id;

        // Validate the packet
        if (!ProtocolEncoder::validate_packet(packet.data.data(), packet.data.size())) {
            std::cerr << "[ServerNetworkSystem] Invalid packet from client " << packet.sender_id << std::endl;
            continue;
        }

        // Decode the header
        PacketHeader header = ProtocolEncoder::decode_header(packet.data.data(), packet.data.size());
        const uint8_t* payload = ProtocolEncoder::get_payload(packet.data.data(), packet.data.size());

        // Handle based on packet type
        PacketType type = static_cast<PacketType>(header.type);

        switch (type) {
            case PacketType::CLIENT_CONNECT:
                handleClientConnect(registry, payload, client_address);
                break;
            case PacketType::CLIENT_INPUT:
                handleClientInput(registry, payload, client_address);
                break;
            case PacketType::CLIENT_DISCONNECT:
                handleClientDisconnect(registry, payload, client_address);
                break;
            case PacketType::CLIENT_PING:
                handleClientPing(registry, payload, client_address);
                break;
            case PacketType::CLIENT_JOIN_LOBBY:
                handleClientJoinLobby(registry, payload, client_address);
                break;
            case PacketType::CLIENT_LEAVE_LOBBY:
                handleClientLeaveLobby(registry, payload, client_address);
                break;
            default:
                std::cerr << "[ServerNetworkSystem] Unknown packet type: 0x"
                          << std::hex << static_cast<int>(header.type) << std::dec
                          << " from client " << packet.sender_id << std::endl;
                break;
        }
    }

    // Check for client timeouts
    checkClientTimeouts(registry);

    // Send periodic snapshots to clients
    snapshot_timer_ += dt;
    if (snapshot_timer_ >= SNAPSHOT_SEND_RATE) {
        broadcastSnapshot(registry);
        snapshot_timer_ = 0.0f;
    }
}

void ServerNetworkSystem::shutdown() {
    std::cout << "[ServerNetworkSystem] Shutting down..." << std::endl;

    // Disconnect all clients
    connected_clients_.clear();

    // Stop the server
    if (network_plugin_->is_server_running()) {
        network_plugin_->stop_server();
    }
}

uint32_t ServerNetworkSystem::generatePlayerId() {
    return next_player_id_++;
}

ConnectedClient* ServerNetworkSystem::findClientByAddress(const std::string& address) {
    for (auto& [id, client] : connected_clients_) {
        if (client.address == address) {
            return &client;
        }
    }
    return nullptr;
}

ConnectedClient* ServerNetworkSystem::findClientById(uint32_t player_id) {
    auto it = connected_clients_.find(player_id);
    if (it != connected_clients_.end()) {
        return &it->second;
    }
    return nullptr;
}

void ServerNetworkSystem::checkClientTimeouts(Registry& registry) {
    auto now = std::chrono::steady_clock::now();
    std::vector<uint32_t> timed_out_clients;

    for (auto& [id, client] : connected_clients_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - client.last_seen).count();
        if (elapsed > CLIENT_TIMEOUT_SECONDS) {
            std::cout << "[ServerNetworkSystem] Client timeout - Player ID: " << id
                      << " (" << client.player_name << ") - Last seen: " << elapsed << "s ago"
                      << std::endl;
            timed_out_clients.push_back(id);
        }
    }

    // Remove timed out clients
    for (uint32_t id : timed_out_clients) {
        connected_clients_.erase(id);
        // TODO: Publish ClientDisconnectedEvent to notify other systems
    }
}

void ServerNetworkSystem::handleClientConnect(Registry& registry, const uint8_t* payload, const std::string& client_address) {
    ClientConnectPayload connect_payload = ProtocolEncoder::decode_client_connect(payload);

    std::cout << "[ServerNetworkSystem] Client connect request from " << client_address
              << " - Player: " << connect_payload.player_name
              << " (version: 0x" << std::hex << static_cast<int>(connect_payload.client_version) << std::dec << ")"
              << std::endl;

    // Check if client is already connected
    ConnectedClient* existing_client = findClientByAddress(client_address);
    if (existing_client) {
        std::cout << "[ServerNetworkSystem] Client already connected - Player ID: " << existing_client->player_id << std::endl;
        // Update last seen and resend accept
        existing_client->last_seen = std::chrono::steady_clock::now();
        sendServerAccept(client_address, existing_client->player_id);
        return;
    }

    // Check if server is full
    if (connected_clients_.size() >= max_players_) {
        std::cout << "[ServerNetworkSystem] Server full - Rejecting connection" << std::endl;
        sendServerReject(client_address, "Server is full");
        return;
    }

    // Assign new player ID and track client
    uint32_t new_player_id = generatePlayerId();

    // Get the network client ID from the address mapping
    ClientId network_client_id = 0;
    auto it = address_to_client_id_.find(client_address);
    if (it != address_to_client_id_.end()) {
        network_client_id = it->second;
    }

    ConnectedClient new_client(new_player_id, client_address, network_client_id);
    new_client.player_name = connect_payload.player_name;
    new_client.authenticated = true;

    // Create player entity
    Entity player_entity = registry.spawn_entity();
    new_client.entity_id = player_entity;

    // Add components - spawn at different positions based on player count
    float spawn_x = 100.0f + (connected_clients_.size() * 50.0f);
    float spawn_y = 300.0f + (connected_clients_.size() * 100.0f);

    registry.add_component(player_entity, Position{spawn_x, spawn_y});
    registry.add_component(player_entity, Velocity{0.0f, 0.0f});
    registry.add_component(player_entity, Controllable{200.0f});
    registry.add_component(player_entity, Collider{32.0f, 32.0f});

    // Track the client
    connected_clients_[new_player_id] = new_client;

    std::cout << "[ServerNetworkSystem] Client accepted - Assigned Player ID: " << new_player_id
              << " - Entity ID: " << player_entity
              << " (" << connected_clients_.size() << "/" << static_cast<int>(max_players_) << " players)"
              << std::endl;

    // Send acceptance to client
    sendServerAccept(client_address, new_player_id);

    // TODO: Publish ClientConnectedEvent to notify other systems
}

void ServerNetworkSystem::handleClientInput(Registry& registry, const uint8_t* payload, const std::string& client_address) {
    ClientInputPayload input_payload = ProtocolEncoder::decode_client_input(payload);

    // Find and validate client
    ConnectedClient* client = findClientById(input_payload.player_id);
    if (!client) {
        std::cerr << "[ServerNetworkSystem] Input from unknown client - Player ID: " << input_payload.player_id << std::endl;
        return;
    }

    // Update last seen timestamp
    client->last_seen = std::chrono::steady_clock::now();

    // Get player entity
    Entity player_entity = client->entity_id;
    if (player_entity == UINT32_MAX) {
        std::cerr << "[ServerNetworkSystem] WARNING: Player " << input_payload.player_id << " has no entity!" << std::endl;
        return;  // No entity for this client yet
    }

    // Get velocity component to apply movement
    auto& velocities = registry.get_components<Velocity>();
    auto& controllables = registry.get_components<Controllable>();

    try {
        Velocity& vel = velocities[player_entity];
        const Controllable& ctrl = controllables[player_entity];

        // Calculate movement direction from input flags
        float dirX = 0.0f;
        float dirY = 0.0f;

        if (input_payload.input_flags & INPUT_UP) dirY -= 1.0f;
        if (input_payload.input_flags & INPUT_DOWN) dirY += 1.0f;
        if (input_payload.input_flags & INPUT_LEFT) dirX -= 1.0f;
        if (input_payload.input_flags & INPUT_RIGHT) dirX += 1.0f;

        // Normalize diagonal movement
        if (dirX != 0.0f && dirY != 0.0f) {
            float length = std::sqrt(dirX * dirX + dirY * dirY);
            dirX /= length;
            dirY /= length;
        }

        // Apply velocity
        vel.x = dirX * ctrl.speed;
        vel.y = dirY * ctrl.speed;

        std::cout << "[ServerNetworkSystem] Applied input for player " << input_payload.player_id
                  << " (entity " << player_entity << "): flags=0x" << std::hex << input_payload.input_flags << std::dec
                  << " -> vel=(" << vel.x << ", " << vel.y << ")" << std::endl;

    } catch (...) {
        std::cerr << "[ServerNetworkSystem] ERROR: Player " << input_payload.player_id
                  << " (entity " << player_entity << ") missing components!" << std::endl;
    }
}

void ServerNetworkSystem::handleClientDisconnect(Registry& registry, const uint8_t* payload, const std::string& client_address) {
    ClientDisconnectPayload disconnect_payload;
    std::memcpy(&disconnect_payload, payload, sizeof(ClientDisconnectPayload));

    // Convert to host byte order
    disconnect_payload.player_id = ntohl(disconnect_payload.player_id);

    // Find client
    ConnectedClient* client = findClientById(disconnect_payload.player_id);
    if (!client) {
        std::cerr << "[ServerNetworkSystem] Disconnect from unknown client - Player ID: " << disconnect_payload.player_id << std::endl;
        return;
    }

    std::cout << "[ServerNetworkSystem] Client disconnect from " << client->player_name
              << " (ID: " << disconnect_payload.player_id << ")"
              << " - Reason: " << static_cast<int>(disconnect_payload.reason)
              << std::endl;

    // Remove client from tracking
    connected_clients_.erase(disconnect_payload.player_id);

    std::cout << "[ServerNetworkSystem] Client removed - " << connected_clients_.size()
              << "/" << static_cast<int>(max_players_) << " players remaining"
              << std::endl;

    // TODO: Remove player entity, notify other clients, publish ClientDisconnectedEvent
}

void ServerNetworkSystem::handleClientPing(Registry& registry, const uint8_t* payload, const std::string& client_address) {
    ClientPingPayload ping_payload;
    std::memcpy(&ping_payload, payload, sizeof(ClientPingPayload));

    // Convert to host byte order
    ping_payload.player_id = ntohl(ping_payload.player_id);
    ping_payload.client_timestamp = ntohl(ping_payload.client_timestamp);

    // Find and validate client
    ConnectedClient* client = findClientById(ping_payload.player_id);
    if (!client) {
        std::cerr << "[ServerNetworkSystem] Ping from unknown client - Player ID: " << ping_payload.player_id << std::endl;
        return;
    }

    // Update last seen timestamp
    client->last_seen = std::chrono::steady_clock::now();

    // Send PONG response
    sendServerPong(client->address, ping_payload.client_timestamp);
}

void ServerNetworkSystem::handleClientJoinLobby(Registry& registry, const uint8_t* payload, const std::string& client_address) {
    ClientJoinLobbyPayload lobby_payload = ProtocolEncoder::decode_client_join_lobby(payload);

    // Find and validate client
    ConnectedClient* client = findClientById(lobby_payload.player_id);
    if (!client) {
        std::cerr << "[ServerNetworkSystem] Join lobby from unknown client - Player ID: " << lobby_payload.player_id << std::endl;
        return;
    }

    // Update last seen timestamp
    client->last_seen = std::chrono::steady_clock::now();

    std::cout << "[ServerNetworkSystem] Join lobby from " << client->player_name
              << " (ID: " << lobby_payload.player_id << ")"
              << " - Mode: " << game_mode_to_string(lobby_payload.game_mode)
              << " - Difficulty: " << difficulty_to_string(lobby_payload.difficulty)
              << std::endl;

    // TODO: Add player to lobby, send SERVER_LOBBY_STATE
}

void ServerNetworkSystem::handleClientLeaveLobby(Registry& registry, const uint8_t* payload, const std::string& client_address) {
    ClientLeaveLobbyPayload leave_payload;
    std::memcpy(&leave_payload, payload, sizeof(ClientLeaveLobbyPayload));

    // Convert to host byte order
    leave_payload.player_id = ntohl(leave_payload.player_id);
    leave_payload.lobby_id = ntohl(leave_payload.lobby_id);

    // Find and validate client
    ConnectedClient* client = findClientById(leave_payload.player_id);
    if (!client) {
        std::cerr << "[ServerNetworkSystem] Leave lobby from unknown client - Player ID: " << leave_payload.player_id << std::endl;
        return;
    }

    // Update last seen timestamp
    client->last_seen = std::chrono::steady_clock::now();

    std::cout << "[ServerNetworkSystem] Leave lobby from " << client->player_name
              << " (ID: " << leave_payload.player_id << ")"
              << " - Player ID: " << leave_payload.player_id
              << " - Lobby ID: " << leave_payload.lobby_id
              << std::endl;

    // TODO: Remove player from lobby, notify other lobby members
}

void ServerNetworkSystem::sendServerAccept(const std::string& client_address, uint32_t player_id) {
    ServerAcceptPayload payload;
    payload.assigned_player_id = htonl(player_id);
    payload.server_tick_rate = 60;  // TODO: Get from config
    payload.max_players = max_players_;
    payload.map_id = htons(0);  // Default map

    uint8_t payload_buffer[sizeof(ServerAcceptPayload)];
    std::memcpy(payload_buffer, &payload, sizeof(ServerAcceptPayload));

    auto packet_data = ProtocolEncoder::encode_packet(
        PacketType::SERVER_ACCEPT,
        payload_buffer,
        sizeof(ServerAcceptPayload),
        sequence_number_++
    );

    ::engine::NetworkPacket packet(packet_data);

    // Look up the network client ID for this address
    auto it = address_to_client_id_.find(client_address);
    if (it != address_to_client_id_.end()) {
        network_plugin_->send_to(packet, it->second);
        std::cout << "[ServerNetworkSystem] Sent SERVER_ACCEPT to client " << it->second
                  << " - Player ID: " << player_id << std::endl;
    } else {
        std::cerr << "[ServerNetworkSystem] Cannot send SERVER_ACCEPT - unknown address: " << client_address << std::endl;
    }
}

void ServerNetworkSystem::sendServerReject(const std::string& client_address, const std::string& reason) {
    ServerRejectPayload payload;
    payload.reason_code = RejectReason::SERVER_FULL;
    payload.set_message(reason);

    uint8_t payload_buffer[sizeof(ServerRejectPayload)];
    std::memcpy(payload_buffer, &payload, sizeof(ServerRejectPayload));

    auto packet_data = ProtocolEncoder::encode_packet(
        PacketType::SERVER_REJECT,
        payload_buffer,
        sizeof(ServerRejectPayload),
        sequence_number_++
    );

    ::engine::NetworkPacket packet(packet_data);

    // Look up the network client ID for this address
    auto it = address_to_client_id_.find(client_address);
    if (it != address_to_client_id_.end()) {
        network_plugin_->send_to(packet, it->second);
    }

    std::cout << "[ServerNetworkSystem] Sent SERVER_REJECT to " << client_address
              << " - Reason: " << reason << std::endl;
}

void ServerNetworkSystem::sendServerPong(const std::string& client_address, uint32_t client_timestamp) {
    ServerPongPayload payload;
    payload.client_timestamp = htonl(client_timestamp);
    payload.server_timestamp = htonl(static_cast<uint32_t>(
        std::chrono::system_clock::now().time_since_epoch().count() / 1000000
    ));

    uint8_t payload_buffer[sizeof(ServerPongPayload)];
    std::memcpy(payload_buffer, &payload, sizeof(ServerPongPayload));

    auto packet_data = ProtocolEncoder::encode_packet(
        PacketType::SERVER_PONG,
        payload_buffer,
        sizeof(ServerPongPayload),
        sequence_number_++
    );

    ::engine::NetworkPacket packet(packet_data);

    // Look up the network client ID for this address
    auto it = address_to_client_id_.find(client_address);
    if (it != address_to_client_id_.end()) {
        network_plugin_->send_to(packet, it->second);
    }
}

void ServerNetworkSystem::broadcastSnapshot(Registry& registry) {
    if (connected_clients_.empty()) {
        return;  // No clients to send to
    }

    // Get Position and Velocity components
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();

    // Count all entities with Position+Velocity (these are the entities we'll send)
    std::vector<Entity> entities_to_send;
    for (size_t i = 0; i < positions.size(); i++) {
        Entity entity = positions.get_entity_at(i);
        if (velocities.has_entity(entity)) {
            entities_to_send.push_back(entity);
        }
    }

    std::cout << "[ServerNetworkSystem] Broadcasting snapshot: " << entities_to_send.size() << " entities to " << connected_clients_.size() << " clients" << std::endl;

    // Prepare the snapshot payload
    ServerSnapshotPayload snapshot_header;
    snapshot_header.server_tick = htonl(server_tick_);
    snapshot_header.entity_count = htons(static_cast<uint16_t>(entities_to_send.size()));

    // Build the complete packet
    std::vector<uint8_t> payload_data;
    payload_data.resize(sizeof(ServerSnapshotPayload) + (sizeof(EntityState) * entities_to_send.size()));

    // Copy header
    std::memcpy(payload_data.data(), &snapshot_header, sizeof(ServerSnapshotPayload));

    // Add each entity's state
    size_t offset = sizeof(ServerSnapshotPayload);
    for (Entity entity : entities_to_send) {
        EntityState entry;

        // Find the player_id for this entity
        uint32_t player_id_for_entity = 0;
        for (const auto& [player_id, client] : connected_clients_) {
            if (client.entity_id == entity) {
                player_id_for_entity = player_id;
                break;
            }
        }

        // Send the player_id (not the internal entity ID) so clients can identify their own player
        entry.entity_id = htonl(player_id_for_entity);
        entry.entity_type = EntityType::PLAYER;  // TODO: Differentiate entity types

        try {
            const Position& pos = positions[entity];
            entry.position_x = pos.x;
            entry.position_y = pos.y;

            const Velocity& vel = velocities[entity];
            entry.velocity_x = static_cast<int16_t>(vel.x);
            entry.velocity_y = static_cast<int16_t>(vel.y);

            std::cout << "[ServerNetworkSystem]   Entity " << entity << " (player_id=" << player_id_for_entity << ") at (" << pos.x << ", " << pos.y << ")" << std::endl;
        } catch (...) {
            // Entity doesn't have these components, skip
            continue;
        }

        std::memcpy(payload_data.data() + offset, &entry, sizeof(EntityState));
        offset += sizeof(EntityState);
    }

    // Encode and broadcast to all clients
    auto packet_data = ProtocolEncoder::encode_packet(
        PacketType::SERVER_SNAPSHOT,
        payload_data.data(),
        payload_data.size(),
        sequence_number_++
    );

    ::engine::NetworkPacket packet(packet_data);
    for (const auto& [player_id, client] : connected_clients_) {
        // Look up the network client ID for this address
        auto it = address_to_client_id_.find(client.address);
        if (it != address_to_client_id_.end()) {
            network_plugin_->send_to(packet, it->second);
        }
    }
}

}
