#pragma once

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "plugin_manager/INetworkPlugin.hpp"
#include <unordered_map>
#include <string>
#include <chrono>

namespace rtype::server {

using engine::ClientId;

/**
 * @brief Information about a connected client
 */
struct ConnectedClient {
    uint32_t player_id;
    std::string address;
    ClientId network_client_id;  // Network plugin's client ID
    std::chrono::steady_clock::time_point last_seen;
    bool authenticated;
    std::string player_name;
    Entity entity_id;  // ECS entity associated with this client

    ConnectedClient() : player_id(0), address(""), network_client_id(0),
                        last_seen(std::chrono::steady_clock::now()),
                        authenticated(false), player_name(""), entity_id(UINT32_MAX) {}

    ConnectedClient(uint32_t id, const std::string& addr, ClientId net_client_id = 0)
        : player_id(id), address(addr), network_client_id(net_client_id),
          last_seen(std::chrono::steady_clock::now()),
          authenticated(false), player_name(""), entity_id(UINT32_MAX) {}
};

/**
 * @brief Server-side network system for R-Type
 *
 * This system handles R-Type specific protocol logic:
 * - Validates incoming packets
 * - Decodes R-Type protocol messages
 * - Publishes game events based on network packets
 * - Sends game state updates to clients
 * - Tracks connected clients
 * - Handles client timeouts
 *
 * This is R-Type specific and uses rtype::protocol namespace.
 * The generic NetworkSystem in engine/ just handles raw bytes.
 */
class ServerNetworkSystem : public ISystem {
public:
    ServerNetworkSystem(engine::INetworkPlugin& plugin, uint8_t max_players = 4)
        : network_plugin_(&plugin), max_players_(max_players) {}
    ~ServerNetworkSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

private:
    void handleIncomingPackets(Registry& registry);
    void handleClientConnect(Registry& registry, const uint8_t* payload, const std::string& client_address);
    void handleClientInput(Registry& registry, const uint8_t* payload, const std::string& client_address);
    void handleClientDisconnect(Registry& registry, const uint8_t* payload, const std::string& client_address);
    void handleClientPing(Registry& registry, const uint8_t* payload, const std::string& client_address);
    void handleClientJoinLobby(Registry& registry, const uint8_t* payload, const std::string& client_address);
    void handleClientLeaveLobby(Registry& registry, const uint8_t* payload, const std::string& client_address);

    void checkClientTimeouts(Registry& registry);
    uint32_t generatePlayerId();
    ConnectedClient* findClientByAddress(const std::string& address);
    ConnectedClient* findClientById(uint32_t player_id);

    // Packet sending methods
    void sendServerAccept(const std::string& client_address, uint32_t player_id);
    void sendServerReject(const std::string& client_address, const std::string& reason);
    void sendServerPong(const std::string& client_address, uint32_t client_timestamp);
    void broadcastSnapshot(Registry& registry);

    // Client tracking
    std::unordered_map<uint32_t, ConnectedClient> connected_clients_;
    std::unordered_map<std::string, ClientId> address_to_client_id_;  // Map address to network client ID
    uint32_t next_player_id_ = 1;
    uint32_t sequence_number_ = 0;
    uint32_t server_tick_ = 0;
    uint8_t max_players_ = 4;

    // Network plugin reference for sending packets
    engine::INetworkPlugin* network_plugin_ = nullptr;

    // Timeout configuration
    static constexpr float CLIENT_TIMEOUT_SECONDS = 30.0f;
    static constexpr float SNAPSHOT_SEND_RATE = 0.05f;  // 20 snapshots per second
    float snapshot_timer_ = 0.0f;
};

}
