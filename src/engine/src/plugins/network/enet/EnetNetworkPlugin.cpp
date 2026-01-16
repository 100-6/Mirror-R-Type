/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** EnetNetworkPlugin - ENet implementation of INetworkPlugin
*/

#include "plugins/network/enet/EnetNetworkPlugin.hpp"
#include <iostream>
#include <cstring>

namespace engine {

EnetNetworkPlugin::EnetNetworkPlugin() = default;

EnetNetworkPlugin::~EnetNetworkPlugin() {
    shutdown();
}

const char* EnetNetworkPlugin::get_name() const {
    return "EnetNetworkPlugin";
}

const char* EnetNetworkPlugin::get_version() const {
    return "1.0.0";
}

bool EnetNetworkPlugin::initialize() {
    if (initialized_) 
        return true;
    if (enet_initialize() != 0) {
        std::cerr << "[EnetNetworkPlugin] Failed to initialize ENet" << std::endl;
        return false;
    }
    initialized_ = true;
    std::cout << "[EnetNetworkPlugin] Initialized successfully" << std::endl;
    return true;
}

void EnetNetworkPlugin::shutdown() {
    if (!initialized_)
        return;
    if (is_server_)
        stop_server();
    else
        disconnect();
    enet_deinitialize();
    initialized_ = false;
    std::cout << "[EnetNetworkPlugin] Shutdown complete" << std::endl;
}

bool EnetNetworkPlugin::is_initialized() const {
    return initialized_;
}

bool EnetNetworkPlugin::start_server(uint16_t tcp_port, uint16_t udp_port) {
    return start_server(tcp_port, udp_port, true);
}

bool EnetNetworkPlugin::start_server(uint16_t tcp_port, uint16_t udp_port, bool listen_on_all_interfaces) {
    if (!initialized_) {
        std::cerr << "[EnetNetworkPlugin] Plugin not initialized" << std::endl;
        return false;
    }
    if (running_) {
        std::cerr << "[EnetNetworkPlugin] Already running" << std::endl;
        return false;
    }
    // In ENet unified mode, we use tcp_port as the primary port
    // udp_port is ignored since ENet handles everything over UDP internally
    (void)udp_port;
    primary_port_ = tcp_port;
    ENetAddress address;
    if (listen_on_all_interfaces)
        address.host = ENET_HOST_ANY;
    else
        enet_address_set_host(&address, "127.0.0.1");
    address.port = primary_port_;
    host_ = enet_host_create(&address, MAX_CLIENTS, CHANNEL_COUNT, 0, 0);
    if (host_ == nullptr) {
        std::cerr << "[EnetNetworkPlugin] Failed to create ENet server host on port " << primary_port_ << std::endl;
        return false;
    }
    is_server_ = true;
    running_ = true;
    network_thread_ = std::make_unique<std::thread>(&EnetNetworkPlugin::run_network_thread, this);
    std::cout << "[EnetNetworkPlugin] Server started on port " << primary_port_ << std::endl;
    return true;
}

void EnetNetworkPlugin::stop_server() {
    if (!is_server_ || !running_)
        return;
    running_ = false;

    if (network_thread_ && network_thread_->joinable())
        network_thread_->join();
    network_thread_.reset();
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (auto& [client_id, client_info] : clients_)
            if (client_info.peer)
                enet_peer_disconnect(client_info.peer, 0);
    }
    if (host_) {
        ENetEvent event;
        while (enet_host_service(host_, &event, 100) > 0)
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
                enet_packet_destroy(event.packet);
        enet_host_destroy(host_);
        host_ = nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        clients_.clear();
        peer_to_client_.clear();
    }
    is_server_ = false;
    std::cout << "[EnetNetworkPlugin] Server stopped" << std::endl;
}

bool EnetNetworkPlugin::is_server_running() const {
    return is_server_ && running_;
}

void EnetNetworkPlugin::disconnect_client(ClientId client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients_.find(client_id);

    if (it != clients_.end() && it->second.peer)
        enet_peer_disconnect(it->second.peer, 0);
}

bool EnetNetworkPlugin::connect_tcp(const std::string& host, uint16_t port) {
    if (!initialized_) {
        std::cerr << "[EnetNetworkPlugin] Plugin not initialized" << std::endl;
        return false;
    }
    if (running_) {
        std::cerr << "[EnetNetworkPlugin] Already connected" << std::endl;
        return false;
    }
    host_ = enet_host_create(nullptr, 1, CHANNEL_COUNT, 0, 0);

    if (host_ == nullptr) {
        std::cerr << "[EnetNetworkPlugin] Failed to create ENet client host" << std::endl;
        return false;
    }
    ENetAddress address;
    enet_address_set_host(&address, host.c_str());
    address.port = port;
    server_peer_ = enet_host_connect(host_, &address, CHANNEL_COUNT, 0);
    if (server_peer_ == nullptr) {
        std::cerr << "[EnetNetworkPlugin] Failed to initiate connection to " << host << ":" << port << std::endl;
        enet_host_destroy(host_);
        host_ = nullptr;
        return false;
    }
    ENetEvent event;
    if (enet_host_service(host_, &event, CONNECTION_TIMEOUT_MS) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        connected_ = true;
        is_server_ = false;
        running_ = true;
        primary_port_ = port;
        network_thread_ = std::make_unique<std::thread>(&EnetNetworkPlugin::run_network_thread, this);
        std::cout << "[EnetNetworkPlugin] Connected to " << host << ":" << port << std::endl;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            if (on_connected_)
                on_connected_();
        }
        return true;
    }
    enet_peer_reset(server_peer_);
    enet_host_destroy(host_);
    server_peer_ = nullptr;
    host_ = nullptr;
    std::cerr << "[EnetNetworkPlugin] Connection to " << host << ":" << port << " timed out" << std::endl;
    return false;
}

bool EnetNetworkPlugin::connect_udp(const std::string& host, uint16_t port) {
    (void)host;
    (void)port;

    return connected_;
}

void EnetNetworkPlugin::disconnect() {
    if (!running_ || is_server_)
        return;
    running_ = false;
    connected_ = false;

    if (network_thread_ && network_thread_->joinable())
        network_thread_->join();
    network_thread_.reset();
    if (server_peer_) {
        enet_peer_disconnect(server_peer_, 0);
        if (host_) {
            ENetEvent event;
            while (enet_host_service(host_, &event, 100) > 0) {
                if (event.type == ENET_EVENT_TYPE_RECEIVE)
                    enet_packet_destroy(event.packet);
                else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                    break;
            }
        }
        server_peer_ = nullptr;
    }
    if (host_) {
        enet_host_destroy(host_);
        host_ = nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (on_disconnected_)
            on_disconnected_();
    }
    std::cout << "[EnetNetworkPlugin] Disconnected" << std::endl;
}

bool EnetNetworkPlugin::is_tcp_connected() const {
    return connected_;
}

bool EnetNetworkPlugin::is_udp_connected() const {
    return connected_;
}

bool EnetNetworkPlugin::send_tcp(const NetworkPacket& packet) {
    if (!connected_ || !server_peer_)
        return false;
    return send_packet_to_peer(server_peer_, packet, true);
}

bool EnetNetworkPlugin::send_udp(const NetworkPacket& packet) {
    if (!connected_ || !server_peer_)
        return false;
    return send_packet_to_peer(server_peer_, packet, false);
}

bool EnetNetworkPlugin::send_tcp_to(const NetworkPacket& packet, ClientId client_id) {
    ENetPeer* peer = get_peer_from_client_id(client_id);

    if (!peer)
        return false;
    return send_packet_to_peer(peer, packet, true);
}

bool EnetNetworkPlugin::send_udp_to(const NetworkPacket& packet, ClientId client_id) {
    ENetPeer* peer = get_peer_from_client_id(client_id);

    if (!peer)
        return false;
    return send_packet_to_peer(peer, packet, false);
}

size_t EnetNetworkPlugin::broadcast_tcp(const NetworkPacket& packet) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    size_t sent = 0;

    for (auto& [client_id, client_info] : clients_)
        if (client_info.peer && send_packet_to_peer(client_info.peer, packet, true))
            ++sent;
    return sent;
}

size_t EnetNetworkPlugin::broadcast_udp(const NetworkPacket& packet) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    size_t sent = 0;

    for (auto& [client_id, client_info] : clients_)
        if (client_info.peer && send_packet_to_peer(client_info.peer, packet, false))
            ++sent;
    return sent;
}

size_t EnetNetworkPlugin::broadcast_tcp_except(const NetworkPacket& packet, ClientId exclude_client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    size_t sent = 0;

    for (auto& [client_id, client_info] : clients_) {
        if (client_id != exclude_client_id && client_info.peer &&
            send_packet_to_peer(client_info.peer, packet, true)) {
            ++sent;
        }
    }
    return sent;
}

size_t EnetNetworkPlugin::broadcast_udp_except(const NetworkPacket& packet, ClientId exclude_client_id) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    size_t sent = 0;

    for (auto& [client_id, client_info] : clients_) {
        if (client_id != exclude_client_id && client_info.peer &&
            send_packet_to_peer(client_info.peer, packet, false)) {
            ++sent;
        }
    }
    return sent;
}

void EnetNetworkPlugin::associate_udp_client(ClientId tcp_client_id, ClientId udp_client_id) {
    std::lock_guard<std::mutex> lock(association_mutex_);
    tcp_to_udp_[tcp_client_id] = udp_client_id;
    udp_to_tcp_[udp_client_id] = tcp_client_id;
}

ClientId EnetNetworkPlugin::get_tcp_client_from_udp(ClientId udp_client_id) const {
    std::lock_guard<std::mutex> lock(association_mutex_);
    auto it = udp_to_tcp_.find(udp_client_id);

    if (it != udp_to_tcp_.end())
        return it->second;
    return udp_client_id;
}

bool EnetNetworkPlugin::has_udp_association(ClientId tcp_client_id) const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.find(tcp_client_id) != clients_.end();
}

std::vector<NetworkPacket> EnetNetworkPlugin::receive() {
    std::lock_guard<std::mutex> lock(packet_mutex_);
    std::vector<NetworkPacket> packets;

    while (!received_packets_.empty()) {
        packets.push_back(std::move(received_packets_.front()));
        received_packets_.pop();
    }
    return packets;
}

void EnetNetworkPlugin::update(float delta_time) {
    (void)delta_time;
}

void EnetNetworkPlugin::set_on_client_connected(std::function<void(ClientId)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_client_connected_ = std::move(callback);
}

void EnetNetworkPlugin::set_on_client_disconnected(std::function<void(ClientId)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_client_disconnected_ = std::move(callback);
}

void EnetNetworkPlugin::set_on_packet_received(std::function<void(ClientId, const NetworkPacket&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_packet_received_ = std::move(callback);
}

void EnetNetworkPlugin::set_on_connected(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_connected_ = std::move(callback);
}

void EnetNetworkPlugin::set_on_disconnected(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_disconnected_ = std::move(callback);
}

size_t EnetNetworkPlugin::get_client_count() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    return clients_.size();
}

std::vector<ClientId> EnetNetworkPlugin::get_client_ids() const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    std::vector<ClientId> ids;
    ids.reserve(clients_.size());

    for (const auto& [id, info] : clients_)
        ids.push_back(id);
    return ids;
}

int EnetNetworkPlugin::get_client_ping(ClientId client_id) const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients_.find(client_id);

    if (it != clients_.end() && it->second.peer)
        return static_cast<int>(it->second.peer->roundTripTime);
    return -1;
}

int EnetNetworkPlugin::get_server_ping() const {
    if (server_peer_)
        return static_cast<int>(server_peer_->roundTripTime);
    return -1;
}

void EnetNetworkPlugin::run_network_thread() {
    while (running_) {
        process_events();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void EnetNetworkPlugin::process_events() {
    if (!host_)
        return;
    ENetEvent event;
    while (enet_host_service(host_, &event, SERVICE_TIMEOUT_MS) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                handle_connect_event(event);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                handle_disconnect_event(event);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                handle_receive_event(event);
                break;
            case ENET_EVENT_TYPE_NONE:
                break;
        }
    }
}

void EnetNetworkPlugin::handle_connect_event(ENetEvent& event) {
    if (is_server_) {
        ClientId client_id = generate_client_id();
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            ClientInfo client_info;
            client_info.id = client_id;
            client_info.peer = event.peer;
            client_info.connected_at = std::chrono::steady_clock::now();
            clients_[client_id] = client_info;
            peer_to_client_[event.peer] = client_id;
            event.peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(client_id));
        }
        char host_ip[256];
        enet_address_get_host_ip(&event.peer->address, host_ip, sizeof(host_ip));
        std::cout << "[EnetNetworkPlugin] Client " << client_id << " connected from "
                  << host_ip << ":" << event.peer->address.port << std::endl;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            if (on_client_connected_)
                on_client_connected_(client_id);
        }
    }
}

void EnetNetworkPlugin::handle_disconnect_event(ENetEvent& event) {
    if (is_server_) {
        ClientId client_id = get_client_id_from_peer(event.peer);
        if (client_id != 0) {
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                clients_.erase(client_id);
                peer_to_client_.erase(event.peer);
            }
            {
                std::lock_guard<std::mutex> lock(association_mutex_);
                auto tcp_it = tcp_to_udp_.find(client_id);
                if (tcp_it != tcp_to_udp_.end()) {
                    udp_to_tcp_.erase(tcp_it->second);
                    tcp_to_udp_.erase(tcp_it);
                }
            }
            std::cout << "[EnetNetworkPlugin] Client " << client_id << " disconnected" << std::endl;
            {
                std::lock_guard<std::mutex> lock(callback_mutex_);
                if (on_client_disconnected_)
                    on_client_disconnected_(client_id);
            }
        }
    } else {
        connected_ = false;
        std::cout << "[EnetNetworkPlugin] Disconnected from server" << std::endl;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            if (on_disconnected_)
                on_disconnected_();
        }
    }
}

void EnetNetworkPlugin::handle_receive_event(ENetEvent& event) {
    NetworkPacket packet;
    packet.data.assign(event.packet->data, event.packet->data + event.packet->dataLength);
    packet.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
    packet.protocol = (event.channelID == CHANNEL_RELIABLE)
        ? NetworkProtocol::TCP
        : NetworkProtocol::UDP;
    if (is_server_)
        packet.sender_id = get_client_id_from_peer(event.peer);
    else
        packet.sender_id = 0;
    {
        std::lock_guard<std::mutex> lock(packet_mutex_);
        received_packets_.push(std::move(packet));
    }
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (on_packet_received_) {
            std::lock_guard<std::mutex> pkt_lock(packet_mutex_);
            NetworkPacket callback_packet;
            callback_packet.data.assign(event.packet->data, event.packet->data + event.packet->dataLength);
            callback_packet.sender_id = is_server_ ? get_client_id_from_peer(event.peer) : 0;
            callback_packet.protocol = (event.channelID == CHANNEL_RELIABLE)
                ? NetworkProtocol::TCP
                : NetworkProtocol::UDP;
            on_packet_received_(callback_packet.sender_id, callback_packet);
        }
    }
    enet_packet_destroy(event.packet);
}

ClientId EnetNetworkPlugin::generate_client_id() {
    return next_client_id_++;
}

ClientId EnetNetworkPlugin::get_client_id_from_peer(ENetPeer* peer) const {
    if (!peer || !peer->data) {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        auto it = peer_to_client_.find(peer);
        if (it != peer_to_client_.end())
            return it->second;
        return 0;
    }
    return static_cast<ClientId>(reinterpret_cast<uintptr_t>(peer->data));
}

ENetPeer* EnetNetworkPlugin::get_peer_from_client_id(ClientId client_id) const {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients_.find(client_id);

    if (it != clients_.end())
        return it->second.peer;
    return nullptr;
}

bool EnetNetworkPlugin::send_packet_to_peer(ENetPeer* peer, const NetworkPacket& packet, bool reliable) {
    if (!peer || packet.data.empty())
        return false;
    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    uint8_t channel = reliable ? CHANNEL_RELIABLE : CHANNEL_UNRELIABLE;
    ENetPacket* enet_packet = enet_packet_create(
        packet.data.data(),
        packet.data.size(),
        flags
    );

    if (!enet_packet)
        return false;
    if (enet_peer_send(peer, channel, enet_packet) < 0) {
        enet_packet_destroy(enet_packet);
        return false;
    }
    return true;
}

}

extern "C" {

PLUGIN_API engine::INetworkPlugin* create_network_plugin() {
    return new engine::EnetNetworkPlugin();
}

PLUGIN_API void destroy_network_plugin(engine::INetworkPlugin* plugin) {
    delete plugin;
}

}
