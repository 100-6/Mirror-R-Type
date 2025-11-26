/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AsioNetworkPlugin - Implementation
*/

#include "plugins/network/asio/AsioNetworkPlugin.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/buffer.hpp>
#include <iostream>
#include <algorithm>

using namespace boost::asio;
using namespace boost::asio::ip;
using boost::system::error_code;

namespace engine {

AsioNetworkPlugin::AsioNetworkPlugin() : 
    initialized_(false),
    is_server_(false), 
    is_client_(false), 
    running_(false), 
    io_context_(nullptr), 
    socket_(nullptr), 
    server_endpoint_(nullptr), 
    recv_endpoint_(nullptr), 
    io_thread_(nullptr), 
    server_port_(0), 
    protocol_(NetworkProtocol::UDP), 
    next_client_id_(1), 
    server_ping_ms_(-1), 
    last_timeout_check_(std::chrono::steady_clock::now())
{
}

AsioNetworkPlugin::~AsioNetworkPlugin()
{
    if (initialized_)
        shutdown();
}

const char* AsioNetworkPlugin::get_name() const
{
    return "Asio Network Plugin";
}

const char* AsioNetworkPlugin::get_version() const
{
    return "1.0.0";
}

bool AsioNetworkPlugin::initialize()
{
    if (initialized_)
        return true;
    try {
        io_context_ = std::make_unique<io_context>();
        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] Initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void AsioNetworkPlugin::shutdown()
{
    if (!initialized_)
        return;
    stop_server();
    disconnect();
    if (io_context_)
        io_context_->stop();
    if (io_thread_ && io_thread_->joinable())
        io_thread_->join();
    io_context_.reset();
    socket_.reset();
    server_endpoint_.reset();
    recv_endpoint_.reset();
    io_thread_.reset();
    clients_by_endpoint_.clear();
    clients_by_id_.clear();
    std::lock_guard<std::mutex> lock(packet_mutex_);
    while (!received_packets_.empty())
        received_packets_.pop();
    initialized_ = false;
}

bool AsioNetworkPlugin::is_initialized() const
{
    return initialized_;
}

bool AsioNetworkPlugin::start_server(uint16_t port, NetworkProtocol protocol)
{
    if (!initialized_) {
        std::cerr << "[AsioNetworkPlugin] Cannot start server: not initialized" << std::endl;
        return false;
    }
    if (is_server_) {
        std::cerr << "[AsioNetworkPlugin] Server already running" << std::endl;
        return false;
    }
    if (protocol != NetworkProtocol::UDP) {
        std::cerr << "[AsioNetworkPlugin] Only UDP protocol is currently supported" << std::endl;
        return false;
    }
    try {
        server_port_ = port;
        protocol_ = protocol;
        udp::endpoint endpoint(udp::v4(), port);
        socket_ = std::make_unique<udp::socket>(*io_context_, endpoint);
        recv_endpoint_ = std::make_unique<udp::endpoint>();
        is_server_ = true;
        running_ = true;
        start_async_receive();
        io_thread_ = std::make_unique<std::thread>([this]() {run_io_context();});
        std::cout << "[AsioNetworkPlugin] Server started on port " << port << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] Failed to start server: " << e.what() << std::endl;
        is_server_ = false;
        running_ = false;
        socket_.reset();
        recv_endpoint_.reset();
        return false;
    }
}

void AsioNetworkPlugin::stop_server()
{
    if (!is_server_)
        return;
    running_ = false;
    is_server_ = false;
    if (socket_) {
        boost::system::error_code ec;
        socket_->close(ec);
        if (ec)
            std::cerr << "[AsioNetworkPlugin] Error closing socket: " << ec.message() << std::endl;
    }
    if (io_context_)
        io_context_->stop();
    if (io_thread_ && io_thread_->joinable())
        io_thread_->join();
    clients_by_endpoint_.clear();
    clients_by_id_.clear();
    std::cout << "[AsioNetworkPlugin] Server stopped" << std::endl;
}

bool AsioNetworkPlugin::is_server_running() const
{
    return is_server_ && running_;
}

bool AsioNetworkPlugin::connect(const std::string& host, uint16_t port, NetworkProtocol protocol)
{
    if (!initialized_) {
        std::cerr << "[AsioNetworkPlugin] Cannot connect: not initialized" << std::endl;
        return false;
    }
    if (is_client_) {
        std::cerr << "[AsioNetworkPlugin] Already connected" << std::endl;
        return false;
    }
    if (protocol != NetworkProtocol::UDP) {
        std::cerr << "[AsioNetworkPlugin] Only UDP protocol is currently supported" << std::endl;
        return false;
    }
    try {
        protocol_ = protocol;
        udp::resolver resolver(*io_context_);
        udp::resolver::results_type endpoints = resolver.resolve(udp::v4(), host, std::to_string(port));
        if (endpoints.empty()) {
            std::cerr << "[AsioNetworkPlugin] Could not resolve host: " << host << std::endl;
            return false;
        }
        socket_ = std::make_unique<udp::socket>(*io_context_);
        socket_->open(udp::v4());
        server_endpoint_ = std::make_unique<udp::endpoint>(*endpoints.begin());
        recv_endpoint_ = std::make_unique<udp::endpoint>();
        is_client_ = true;
        running_ = true;
        start_async_receive();
        io_thread_ = std::make_unique<std::thread>([this]() {run_io_context();});
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            if (on_connected_)
                on_connected_();
        }
        std::cout << "[AsioNetworkPlugin] Connected to " << host << ":" << port << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] Failed to connect: " << e.what() << std::endl;
        is_client_ = false;
        running_ = false;
        socket_.reset();
        server_endpoint_.reset();
        recv_endpoint_.reset();
        return false;
    }
}

void AsioNetworkPlugin::disconnect()
{
    if (!is_client_)
        return;
    running_ = false;
    is_client_ = false;
    if (socket_) {
        boost::system::error_code ec;
        socket_->close(ec);
        if (ec)
            std::cerr << "[AsioNetworkPlugin] Error closing socket: " << ec.message() << std::endl;
    }
    if (io_context_)
        io_context_->stop();
    if (io_thread_ && io_thread_->joinable())
        io_thread_->join();
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (on_disconnected_)
            on_disconnected_();
    }
    std::cout << "[AsioNetworkPlugin] Disconnected" << std::endl;
}

bool AsioNetworkPlugin::is_connected() const
{
    return is_client_ && running_;
}

bool AsioNetworkPlugin::send(const NetworkPacket& packet)
{
    if (!is_client_ || !socket_ || !server_endpoint_) {
        std::cerr << "[AsioNetworkPlugin] Cannot send: not connected" << std::endl;
        return false;
    }
    try {
        socket_->send_to(buffer(packet.data), *server_endpoint_);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] Send failed: " << e.what() << std::endl;
        return false;
    }
}

bool AsioNetworkPlugin::send_to(const NetworkPacket& packet, ClientId client_id)
{
    if (!is_server_ || !socket_) {
        std::cerr << "[AsioNetworkPlugin] Cannot send_to: not in server mode" << std::endl;
        return false;
    }
    auto it = clients_by_id_.find(client_id);
    if (it == clients_by_id_.end()) {
        std::cerr << "[AsioNetworkPlugin] Client " << client_id << " not found" << std::endl;
        return false;
    }
    auto client_it = clients_by_endpoint_.find(it->second);
    if (client_it == clients_by_endpoint_.end()) {
        return false;
    }
    try {
        socket_->send_to(buffer(packet.data), *client_it->second.endpoint);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] Send_to failed: " << e.what() << std::endl;
        return false;
    }
}

size_t AsioNetworkPlugin::broadcast(const NetworkPacket& packet)
{
    if (!is_server_ || !socket_) {
        std::cerr << "[AsioNetworkPlugin] Cannot broadcast: not in server mode" << std::endl;
        return 0;
    }
    size_t count = 0;
    for (const auto& [endpoint_key, client_info] : clients_by_endpoint_) {
        try {
            socket_->send_to(buffer(packet.data), *client_info.endpoint);
            count++;
        } catch (const std::exception& e) {
            std::cerr << "[AsioNetworkPlugin] Broadcast to client " << client_info.id
                     << " failed: " << e.what() << std::endl;
        }
    }
    return count;
}

size_t AsioNetworkPlugin::broadcast_except(const NetworkPacket& packet, ClientId exclude_client_id)
{
    if (!is_server_ || !socket_) {
        std::cerr << "[AsioNetworkPlugin] Cannot broadcast: not in server mode" << std::endl;
        return 0;
    }
    size_t count = 0;
    for (const auto& [endpoint_key, client_info] : clients_by_endpoint_) {
        if (client_info.id == exclude_client_id)
            continue;
        try {
            socket_->send_to(buffer(packet.data), *client_info.endpoint);
            count++;
        } catch (const std::exception& e) {
            std::cerr << "[AsioNetworkPlugin] Broadcast to client " << client_info.id
                     << " failed: " << e.what() << std::endl;
        }
    }
    return count;
}

std::vector<NetworkPacket> AsioNetworkPlugin::receive()
{
    std::lock_guard<std::mutex> lock(packet_mutex_);
    std::vector<NetworkPacket> packets;

    while (!received_packets_.empty()) {
        packets.push_back(std::move(received_packets_.front()));
        received_packets_.pop();
    }
    return packets;
}

void AsioNetworkPlugin::update(float delta_time)
{
    if (is_server_)
        check_client_timeouts();
}

void AsioNetworkPlugin::set_on_client_connected(std::function<void(ClientId)> callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_client_connected_ = callback;
}

void AsioNetworkPlugin::set_on_client_disconnected(std::function<void(ClientId)> callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_client_disconnected_ = callback;
}

void AsioNetworkPlugin::set_on_packet_received(std::function<void(ClientId, const NetworkPacket&)> callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_packet_received_ = callback;
}

void AsioNetworkPlugin::set_on_connected(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_connected_ = callback;
}

void AsioNetworkPlugin::set_on_disconnected(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(callback_mutex_);
    on_disconnected_ = callback;
}

size_t AsioNetworkPlugin::get_client_count() const
{
    return clients_by_id_.size();
}

std::vector<ClientId> AsioNetworkPlugin::get_client_ids() const
{
    std::vector<ClientId> ids;
    ids.reserve(clients_by_id_.size());

    for (const auto& [id, endpoint] : clients_by_id_)
        ids.push_back(id);
    return ids;
}

int AsioNetworkPlugin::get_client_ping(ClientId client_id) const
{
    auto it = clients_by_id_.find(client_id);

    if (it == clients_by_id_.end())
        return -1;
    auto client_it = clients_by_endpoint_.find(it->second);
    if (client_it == clients_by_endpoint_.end())
        return -1;
    return client_it->second.ping_ms;
}

int AsioNetworkPlugin::get_server_ping() const
{
    return server_ping_ms_;
}

void AsioNetworkPlugin::start_async_receive()
{
    if (!socket_ || !recv_endpoint_)
        return;
    socket_->async_receive_from(
        buffer(recv_buffer_),
        *recv_endpoint_,
        [this](const boost::system::error_code& error, size_t bytes_transferred) {
            handle_receive(error, bytes_transferred);
        }
    );
}

void AsioNetworkPlugin::handle_receive(const boost::system::error_code& error, size_t bytes_transferred)
{
    if (error) {
        if (error == boost::asio::error::operation_aborted)
            return;
        std::cerr << "[AsioNetworkPlugin] Receive error: " << error.message() << std::endl;
        start_async_receive();
        return;
    }
    if (bytes_transferred == 0) {
        start_async_receive();
        return;
    }
    NetworkPacket packet(recv_buffer_.data(), bytes_transferred);
    packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    ClientId sender_id = 0;
    if (is_server_) {
        std::string endpoint_key = endpoint_to_string(*recv_endpoint_);
        sender_id = get_or_create_client_id(endpoint_key);
        packet.sender_id = sender_id;
        update_client_activity(sender_id);
    } else {
        packet.sender_id = 0;
    }
    {
        std::lock_guard<std::mutex> lock(packet_mutex_);
        received_packets_.push(packet);
    }
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (on_packet_received_)
            on_packet_received_(packet.sender_id, packet);
    }
    start_async_receive();
}

void AsioNetworkPlugin::run_io_context()
{
    try {
        io_context_->restart();
        io_context_->run();
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] IO context error: " << e.what() << std::endl;
    }
}

ClientId AsioNetworkPlugin::generate_client_id()
{
    return next_client_id_++;
}

ClientId AsioNetworkPlugin::get_or_create_client_id(const std::string& endpoint_key)
{
    auto it = clients_by_endpoint_.find(endpoint_key);
    if (it != clients_by_endpoint_.end())
        return it->second.id;
    ClientId new_id = generate_client_id();
    ClientInfo& info = clients_by_endpoint_[endpoint_key];
    info.id = new_id;
    info.endpoint = std::make_unique<udp::endpoint>(*recv_endpoint_);
    info.last_seen = std::chrono::steady_clock::now();
    info.ping_ms = 0;
    clients_by_id_[new_id] = endpoint_key;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (on_client_connected_)
            on_client_connected_(new_id);
    }
    std::cout << "[AsioNetworkPlugin] New client connected: " << new_id
             << " from " << endpoint_key << std::endl;
    return new_id;
}

std::string AsioNetworkPlugin::endpoint_to_string(const udp::endpoint& endpoint)
{
    return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
}

void AsioNetworkPlugin::update_client_activity(ClientId client_id)
{
    auto it = clients_by_id_.find(client_id);

    if (it == clients_by_id_.end())
        return;
    auto client_it = clients_by_endpoint_.find(it->second);
    if (client_it == clients_by_endpoint_.end())
        return;
    client_it->second.last_seen = std::chrono::steady_clock::now();
}

void AsioNetworkPlugin::check_client_timeouts()
{
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - last_timeout_check_).count();

    if (elapsed < TIMEOUT_CHECK_INTERVAL)
        return;
    last_timeout_check_ = now;
    std::vector<ClientId> timed_out_clients;
    for (const auto& [endpoint_key, client_info] : clients_by_endpoint_) {
        float inactive_time = std::chrono::duration<float>(now - client_info.last_seen).count();
        if (inactive_time > CLIENT_TIMEOUT_SECONDS)
            timed_out_clients.push_back(client_info.id);
    }
    for (ClientId client_id : timed_out_clients) {
        auto it = clients_by_id_.find(client_id);
        if (it == clients_by_id_.end())
            continue;
        std::string endpoint_key = it->second;
        clients_by_id_.erase(it);
        clients_by_endpoint_.erase(endpoint_key);
        std::cout << "[AsioNetworkPlugin] Client " << client_id << " timed out" << std::endl;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            if (on_client_disconnected_)
                on_client_disconnected_(client_id);
        }
    }
}

}

extern "C" {
    engine::INetworkPlugin* create_network_plugin() {
        return new engine::AsioNetworkPlugin();
    }

    void destroy_network_plugin(engine::INetworkPlugin* plugin) {
        delete plugin;
    }
}
