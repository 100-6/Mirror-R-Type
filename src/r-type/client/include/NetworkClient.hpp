/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** NetworkClient - Client network handler for hybrid TCP/UDP architecture
*/

#pragma once

#include <memory>
#include <string>
#include <functional>
#include <cstdint>
#include <atomic>

#include "plugin_manager/INetworkPlugin.hpp"
#include "protocol/PacketHeader.hpp"
#include "protocol/PacketTypes.hpp"
#include "protocol/Payloads.hpp"

namespace rtype::client {

/**
 * @brief Client-side network handler for hybrid TCP/UDP communication
 *
 * Handles:
 * - TCP connection for lobby and authentication
 * - UDP connection for gameplay
 * - Packet serialization and deserialization
 */
class NetworkClient {
public:
    /**
     * @brief Construct a new NetworkClient
     * @param plugin Reference to the network plugin
     */
    explicit NetworkClient(engine::INetworkPlugin& plugin);

    ~NetworkClient();

    // ============== Connection ==============

    /**
     * @brief Connect to server via TCP
     * @param host Server hostname or IP
     * @param port TCP port
     * @return true if connected successfully
     */
    bool connect(const std::string& host, uint16_t port);

    /**
     * @brief Disconnect from server
     */
    void disconnect();

    /**
     * @brief Check if TCP is connected
     */
    bool is_tcp_connected() const;

    /**
     * @brief Check if UDP is connected
     */
    bool is_udp_connected() const;

    // ============== Sending ==============

    /**
     * @brief Send connection request to server
     * @param player_name Player name (max 31 chars)
     */
    void send_connect(const std::string& player_name);

    /**
     * @brief Send disconnect notification
     */
    void send_disconnect();

    /**
     * @brief Send ping to server
     */
    void send_ping();

    /**
     * @brief Request to join a lobby
     * @param mode Game mode
     * @param difficulty Difficulty level
     */
    void send_join_lobby(protocol::GameMode mode, protocol::Difficulty difficulty);

    /**
     * @brief Request to leave current lobby
     */
    void send_leave_lobby();

    /**
     * @brief Request to create a custom room
     * @param room_name Room name (optional, empty for auto-name)
     * @param password Room password (optional, empty for no password)
     * @param mode Game mode
     * @param difficulty Difficulty level
     * @param map_id Map identifier (1=Nebula Outpost, 2=Asteroid Belt, 3=Bydo Mothership)
     * @param max_players Maximum players (2-4)
     */
    void send_create_room(const std::string& room_name, const std::string& password,
                          protocol::GameMode mode, protocol::Difficulty difficulty,
                          uint16_t map_id, uint8_t max_players);

    /**
     * @brief Request to join a custom room
     * @param room_id Room ID to join
     * @param password Room password (empty if no password)
     */
    void send_join_room(uint32_t room_id, const std::string& password);

    /**
     * @brief Request to leave current room
     */
    void send_leave_room();

    /**
     * @brief Request list of available rooms
     */
    void send_request_room_list();

    /**
     * @brief Request to start game (host only)
     */
    void send_start_game();

    /**
     * @brief Change player name in lobby
     * @param new_name New player name (max 31 chars)
     */
    void send_set_player_name(const std::string& new_name);

    /**
     * @brief Change player skin in lobby
     * @param skin_id Skin ID (0-14 for 3 colors x 5 ship types)
     */
    void send_set_player_skin(uint8_t skin_id);

    /**
     * @brief Send player input (via UDP if connected, TCP otherwise)
     * @param input_flags Input bitfield
     * @param client_tick Current client tick
     */
    void send_input(uint16_t input_flags, uint32_t client_tick);

    // ============== Update ==============

    /**
     * @brief Process incoming packets - call this every frame
     */
    void update();

    // ============== Callbacks ==============

    /**
     * @brief Set callback for when connection is accepted
     * @param callback Function receiving player_id
     */
    void set_on_accepted(std::function<void(uint32_t player_id)> callback);

    /**
     * @brief Set callback for connection rejection
     * @param callback Function receiving reason code and message
     */
    void set_on_rejected(std::function<void(uint8_t reason, const std::string& message)> callback);

    /**
     * @brief Set callback for lobby state updates
     * @param callback Function receiving lobby info
     */
    void set_on_lobby_state(std::function<void(const protocol::ServerLobbyStatePayload& state,
                                               const std::vector<protocol::PlayerLobbyEntry>& players)> callback);

    /**
     * @brief Set callback for countdown updates
     * @param callback Function receiving seconds remaining
     */
    void set_on_countdown(std::function<void(uint8_t seconds)> callback);

    /**
     * @brief Set callback for game start
     * @param callback Function receiving session_id, udp_port, map_id and scroll speed
     */
    void set_on_game_start(std::function<void(uint32_t session_id, uint16_t udp_port, uint16_t map_id, float scroll_speed)> callback);

    /**
     * @brief Set callback for entity spawn
     * @param callback Function receiving spawn data
     */
    void set_on_entity_spawn(std::function<void(const protocol::ServerEntitySpawnPayload& spawn)> callback);

    /**
     * @brief Set callback for entity destroy
     * @param callback Function receiving destroy data
     */
    void set_on_entity_destroy(std::function<void(const protocol::ServerEntityDestroyPayload& destroy)> callback);

    /**
     * @brief Set callback for projectile spawn
     * @param callback Function receiving projectile spawn data
     */
    void set_on_projectile_spawn(std::function<void(const protocol::ServerProjectileSpawnPayload& spawn)> callback);

    /**
     * @brief Set callback for explosion events
     * @param callback Function receiving explosion payload
     */
    void set_on_explosion(std::function<void(const protocol::ServerExplosionPayload& payload)> callback);

    /**
     * @brief Set callback for game snapshot
     * @param callback Function receiving snapshot header and entity states
     */
    void set_on_snapshot(std::function<void(const protocol::ServerSnapshotPayload& snapshot, const std::vector<protocol::EntityState>& entities)> callback);

    /**
     * @brief Set callback for game over
     * @param callback Function receiving game over data
     */
    void set_on_game_over(std::function<void(const protocol::ServerGameOverPayload& result)> callback);

    /**
     * @brief Set callback for disconnection
     * @param callback Function to call on disconnect
     */
    void set_on_disconnected(std::function<void()> callback);

    /**
     * @brief Set callback for wave start events
     */
    void set_on_wave_start(std::function<void(const protocol::ServerWaveStartPayload&)> callback);

    /**
     * @brief Set callback for wave complete events
     */
    void set_on_wave_complete(std::function<void(const protocol::ServerWaveCompletePayload&)> callback);

    /**
     * @brief Set callback for score update events
     */
    void set_on_score_update(std::function<void(const protocol::ServerScoreUpdatePayload&)> callback);

    /**
     * @brief Set callback for room creation response
     * @param callback Function receiving room_created payload
     */
    void set_on_room_created(std::function<void(const protocol::ServerRoomCreatedPayload&)> callback);

    /**
     * @brief Set callback for room join response
     * @param callback Function receiving room_joined payload
     */
    void set_on_room_joined(std::function<void(const protocol::ServerRoomJoinedPayload&)> callback);

    /**
     * @brief Set callback for room left notification
     * @param callback Function receiving room_left payload
     */
    void set_on_room_left(std::function<void(const protocol::ServerRoomLeftPayload&)> callback);

    /**
     * @brief Set callback for room list response
     * @param callback Function receiving list of rooms
     */
    void set_on_room_list(std::function<void(const std::vector<protocol::RoomInfo>& rooms)> callback);

    /**
     * @brief Set callback for room errors
     * @param callback Function receiving error payload
     */
    void set_on_room_error(std::function<void(const protocol::ServerRoomErrorPayload&)> callback);

    /**
     * @brief Set callback for player name updates in room
     * @param callback Function receiving name update payload
     */
    void set_on_player_name_updated(std::function<void(const protocol::ServerPlayerNameUpdatedPayload&)> callback);

    /**
     * @brief Set callback for player skin updates in room
     * @param callback Function receiving skin update payload
     */
    void set_on_player_skin_updated(std::function<void(const protocol::ServerPlayerSkinUpdatedPayload&)> callback);

    // ============== Getters ==============

    uint32_t get_player_id() const { return player_id_; }
    uint32_t get_session_id() const { return session_id_; }
    uint32_t get_lobby_id() const { return lobby_id_; }
    bool is_in_lobby() const { return in_lobby_; }
    bool is_in_game() const { return in_game_; }
    uint32_t get_last_input_sequence() const { return input_sequence_number_ - 1; }  // Returns last sent sequence

private:
    void handle_packet(const engine::NetworkPacket& packet);

    // Packet handlers
    void handle_server_accept(const std::vector<uint8_t>& payload);
    void handle_server_reject(const std::vector<uint8_t>& payload);
    void handle_server_pong(const std::vector<uint8_t>& payload);
    void handle_lobby_state(const std::vector<uint8_t>& payload);
    void handle_countdown(const std::vector<uint8_t>& payload);
    void handle_game_start(const std::vector<uint8_t>& payload);
    void handle_entity_spawn(const std::vector<uint8_t>& payload);
    void handle_entity_destroy(const std::vector<uint8_t>& payload);
    void handle_projectile_spawn(const std::vector<uint8_t>& payload);
    void handle_explosion_event(const std::vector<uint8_t>& payload);
    void handle_snapshot(const std::vector<uint8_t>& payload);
    void handle_game_over(const std::vector<uint8_t>& payload);
    void handle_wave_start(const std::vector<uint8_t>& payload);
    void handle_wave_complete(const std::vector<uint8_t>& payload);
    void handle_score_update(const std::vector<uint8_t>& payload);
    void handle_room_created(const std::vector<uint8_t>& payload);
    void handle_room_joined(const std::vector<uint8_t>& payload);
    void handle_room_left(const std::vector<uint8_t>& payload);
    void handle_room_list(const std::vector<uint8_t>& payload);
    void handle_room_error(const std::vector<uint8_t>& payload);
    void handle_player_name_updated(const std::vector<uint8_t>& payload);
    void handle_player_skin_updated(const std::vector<uint8_t>& payload);

    // UDP connection after game start
    void connect_udp(uint16_t udp_port);
    void send_udp_handshake();

    // Packet building
    void send_tcp_packet(protocol::PacketType type, const std::vector<uint8_t>& payload);
    void send_udp_packet(protocol::PacketType type, const std::vector<uint8_t>& payload);
    std::vector<uint8_t> serialize_payload(const void* payload, size_t size);

    engine::INetworkPlugin& network_plugin_;
    std::string server_host_;
    uint16_t tcp_port_ = 0;
    uint16_t udp_port_ = 0;

    // State
    uint32_t player_id_ = 0;
    uint32_t session_id_ = 0;
    uint32_t lobby_id_ = 0;
    uint32_t room_id_ = 0;
    std::atomic<bool> in_lobby_{false};
    std::atomic<bool> in_game_{false};
    std::atomic<bool> in_room_{false};

    // Ping tracking
    uint32_t last_ping_timestamp_ = 0;
    int server_ping_ms_ = -1;

    // Input sequence tracking (for lag compensation)
    uint32_t input_sequence_number_ = 0;

    // Packet sequence tracking (for compression/ordering)
    uint32_t tcp_sequence_number_ = 0;
    uint32_t udp_sequence_number_ = 0;

    // Callbacks
    std::function<void(uint32_t)> on_accepted_;
    std::function<void(uint8_t, const std::string&)> on_rejected_;
    std::function<void(const protocol::ServerLobbyStatePayload&, const std::vector<protocol::PlayerLobbyEntry>&)> on_lobby_state_;
    std::function<void(uint8_t)> on_countdown_;
    std::function<void(uint32_t, uint16_t, uint16_t, float)> on_game_start_;
    std::function<void(const protocol::ServerEntitySpawnPayload&)> on_entity_spawn_;
    std::function<void(const protocol::ServerEntityDestroyPayload&)> on_entity_destroy_;
    std::function<void(const protocol::ServerProjectileSpawnPayload&)> on_projectile_spawn_;
    std::function<void(const protocol::ServerExplosionPayload&)> on_explosion_;
    std::function<void(const protocol::ServerSnapshotPayload&, const std::vector<protocol::EntityState>&)> on_snapshot_;
    std::function<void(const protocol::ServerGameOverPayload&)> on_game_over_;
    std::function<void()> on_disconnected_;
    std::function<void(const protocol::ServerWaveStartPayload&)> on_wave_start_;
    std::function<void(const protocol::ServerWaveCompletePayload&)> on_wave_complete_;
    std::function<void(const protocol::ServerScoreUpdatePayload&)> on_score_update_;
    std::function<void(const protocol::ServerRoomCreatedPayload&)> on_room_created_;
    std::function<void(const protocol::ServerRoomJoinedPayload&)> on_room_joined_;
    std::function<void(const protocol::ServerRoomLeftPayload&)> on_room_left_;
    std::function<void(const std::vector<protocol::RoomInfo>&)> on_room_list_;
    std::function<void(const protocol::ServerRoomErrorPayload&)> on_room_error_;
    std::function<void(const protocol::ServerPlayerNameUpdatedPayload&)> on_player_name_updated_;
    std::function<void(const protocol::ServerPlayerSkinUpdatedPayload&)> on_player_skin_updated_;
};

}
