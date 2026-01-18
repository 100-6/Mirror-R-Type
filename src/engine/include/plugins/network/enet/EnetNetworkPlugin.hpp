/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** EnetNetworkPlugin - ENet implementation of INetworkPlugin
** Uses ENet channels for reliability: channel 0 = reliable (TCP-like), channel 1 = unreliable (UDP)
*/

#pragma once

#include "plugin_manager/INetworkPlugin.hpp"
#include <enet/enet.h>
#include <memory>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

namespace engine {

/**
 * @brief ENet implementation of the network plugin interface
 *
 * This plugin uses ENet for networking with channel-based reliability:
 * - Channel 0: Reliable ordered (simulates TCP behavior)
 * - Channel 1: Unreliable unordered (native UDP behavior)
 *
 * ENet provides built-in connection management, packet sequencing, and reliability
 * over UDP, making it ideal for game networking.
 *
 * Server mode: Creates an ENet host that accepts peer connections
 * Client mode: Creates an ENet host and connects to a server peer
 */
class EnetNetworkPlugin : public INetworkPlugin {
public:
    EnetNetworkPlugin();
    ~EnetNetworkPlugin() override;

    const char* get_name() const override;
    const char* get_version() const override;
    bool initialize() override;
    void shutdown() override;
    bool is_initialized() const override;

    bool start_server(uint16_t tcp_port, uint16_t udp_port) override;
    bool start_server(uint16_t tcp_port, uint16_t udp_port, bool listen_on_all_interfaces) override;
    void stop_server() override;
    bool is_server_running() const override;
    void disconnect_client(ClientId client_id) override;

    bool connect_tcp(const std::string& host, uint16_t port) override;
    bool connect_udp(const std::string& host, uint16_t port) override;
    void disconnect() override;
    bool is_tcp_connected() const override;
    bool is_udp_connected() const override;

    bool send_tcp(const NetworkPacket& packet) override;
    bool send_udp(const NetworkPacket& packet) override;

    bool send_tcp_to(const NetworkPacket& packet, ClientId client_id) override;
    bool send_udp_to(const NetworkPacket& packet, ClientId client_id) override;
    size_t broadcast_tcp(const NetworkPacket& packet) override;
    size_t broadcast_udp(const NetworkPacket& packet) override;
    size_t broadcast_tcp_except(const NetworkPacket& packet, ClientId exclude_client_id) override;
    size_t broadcast_udp_except(const NetworkPacket& packet, ClientId exclude_client_id) override;

    void associate_udp_client(ClientId tcp_client_id, ClientId udp_client_id) override;
    ClientId get_tcp_client_from_udp(ClientId udp_client_id) const override;
    bool has_udp_association(ClientId tcp_client_id) const override;

    std::vector<NetworkPacket> receive() override;
    void update(float delta_time) override;

    void set_on_client_connected(std::function<void(ClientId)> callback) override;
    void set_on_client_disconnected(std::function<void(ClientId)> callback) override;
    void set_on_packet_received(std::function<void(ClientId, const NetworkPacket&)> callback) override;
    void set_on_connected(std::function<void()> callback) override;
    void set_on_disconnected(std::function<void()> callback) override;

    size_t get_client_count() const override;
    std::vector<ClientId> get_client_ids() const override;
    int get_client_ping(ClientId client_id) const override;
    int get_server_ping() const override;

private:
    struct ClientInfo {
        ClientId id;
        ENetPeer* peer;
        std::chrono::steady_clock::time_point connected_at;
        int ping_ms = 0;
    };

    void process_events();
    void handle_connect_event(ENetEvent& event);
    void handle_disconnect_event(ENetEvent& event);
    void handle_receive_event(ENetEvent& event);
    ClientId generate_client_id();
    ClientId get_client_id_from_peer(ENetPeer* peer) const;
    ENetPeer* get_peer_from_client_id(ClientId client_id) const;
    bool send_packet_to_peer(ENetPeer* peer, const NetworkPacket& packet, bool reliable);
    void run_network_thread();

    bool initialized_ = false;
    bool is_server_ = false;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};

    ENetHost* host_ = nullptr;

    ENetPeer* server_peer_ = nullptr;

    std::unordered_map<ClientId, ClientInfo> clients_;
    std::unordered_map<ENetPeer*, ClientId> peer_to_client_;
    mutable std::mutex clients_mutex_;

    std::unordered_map<ClientId, ClientId> tcp_to_udp_;
    std::unordered_map<ClientId, ClientId> udp_to_tcp_;
    mutable std::mutex association_mutex_;

    std::unique_ptr<std::thread> network_thread_;

    uint16_t primary_port_ = 0;

    ClientId next_client_id_ = 1;

    mutable std::mutex packet_mutex_;
    std::queue<NetworkPacket> received_packets_;

    mutable std::mutex callback_mutex_;
    std::function<void(ClientId)> on_client_connected_;
    std::function<void(ClientId)> on_client_disconnected_;
    std::function<void(ClientId, const NetworkPacket&)> on_packet_received_;
    std::function<void()> on_connected_;
    std::function<void()> on_disconnected_;

    static constexpr size_t MAX_CLIENTS = 32;
    static constexpr size_t CHANNEL_COUNT = 2;
    static constexpr uint8_t CHANNEL_RELIABLE = 0;
    static constexpr uint8_t CHANNEL_UNRELIABLE = 1;
    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 5000;
    static constexpr uint32_t SERVICE_TIMEOUT_MS = 0;
};

}
