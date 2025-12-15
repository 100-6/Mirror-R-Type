#pragma once

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "plugin_manager/INetworkPlugin.hpp"
#include <unordered_map>
#include <string>

namespace rtype::client {

using engine::ClientId;

/**
 * @brief Client-side network system for R-Type
 *
 * Handles:
 * - Connecting to server
 * - Sending CLIENT_CONNECT, CLIENT_INPUT
 * - Receiving SERVER_ACCEPT, SERVER_SNAPSHOT
 * - Creating/updating remote player entities
 */
class ClientNetworkSystem : public ISystem {
public:
    ClientNetworkSystem(engine::INetworkPlugin& plugin, const std::string& server_host, uint16_t server_port,
                        engine::TextureHandle remote_player_texture = 0, float sprite_width = 64.0f, float sprite_height = 64.0f)
        : network_plugin_(&plugin), server_host_(server_host), server_port_(server_port),
          remote_player_texture_(remote_player_texture), sprite_width_(sprite_width), sprite_height_(sprite_height) {}
    ~ClientNetworkSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

    // Get assigned player ID from server
    uint32_t get_player_id() const { return player_id_; }
    bool is_connected() const { return connected_; }

private:
    void handleIncomingPackets(Registry& registry);
    void handleServerAccept(Registry& registry, const uint8_t* payload);
    void handleServerSnapshot(Registry& registry, const uint8_t* payload);

    void sendClientConnect();
    void sendClientInput(Registry& registry);
    uint16_t captureCurrentInput(Registry& registry);

    // Network state
    engine::INetworkPlugin* network_plugin_ = nullptr;
    std::string server_host_;
    uint16_t server_port_;
    bool connected_ = false;
    uint32_t player_id_ = 0;
    uint32_t sequence_number_ = 0;
    uint32_t client_tick_ = 0;

    // Remote players tracking (player_id -> entity_id)
    std::unordered_map<uint32_t, Entity> remote_players_;

    // Input sending
    float input_send_timer_ = 0.0f;
    static constexpr float INPUT_SEND_RATE = 0.033f;  // 30 times per second

    // Current input state (captured from events)
    uint16_t current_input_flags_ = 0;

    // Rendering remote players
    engine::TextureHandle remote_player_texture_;
    float sprite_width_;
    float sprite_height_;
};

}
