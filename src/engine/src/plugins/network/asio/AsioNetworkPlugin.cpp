/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AsioNetworkPlugin - Hybrid TCP/UDP Implementation
*/

#include "plugins/network/asio/AsioNetworkPlugin.hpp"
#include "plugin_manager/PluginExport.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/buffer.hpp>
#include <iostream>
#include <algorithm>

using namespace boost::asio;
using namespace boost::asio::ip;
using boost::system::error_code;

namespace engine {

AsioNetworkPlugin::AsioNetworkPlugin()
    : last_timeout_check_(std::chrono::steady_clock::now())
{
    client_tcp_read_buffer_.resize(TCP_READ_BUFFER_SIZE);
}

AsioNetworkPlugin::~AsioNetworkPlugin()
{
    if (initialized_)
        shutdown();
}

const char* AsioNetworkPlugin::get_name() const
{
    return "Asio Network Plugin (Hybrid TCP/UDP)";
}

const char* AsioNetworkPlugin::get_version() const
{
    return "2.0.0";
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

    // Mark as not running first to prevent new async operations
    running_ = false;
    tcp_connected_ = false;
    udp_connected_ = false;

    // Stop server if running (this doesn't touch io_context_)
    if (is_server_) {
        // Close TCP acceptor
        if (tcp_acceptor_) {
            boost::system::error_code ec;
            tcp_acceptor_->close(ec);
            tcp_acceptor_.reset();
        }

        // Close all TCP client sockets
        {
            std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
            for (auto& [id, client] : tcp_clients_) {
                if (client.socket) {
                    boost::system::error_code ec;
                    client.socket->close(ec);
                }
            }
            tcp_clients_.clear();
        }

        // Close server UDP socket
        if (udp_socket_) {
            boost::system::error_code ec;
            udp_socket_->close(ec);
            udp_socket_.reset();
        }

        // Clear UDP clients
        {
            std::lock_guard<std::mutex> lock(udp_clients_mutex_);
            udp_clients_by_endpoint_.clear();
            udp_clients_by_id_.clear();
        }

        // Clear associations
        {
            std::lock_guard<std::mutex> lock(association_mutex_);
            tcp_to_udp_.clear();
            udp_to_tcp_.clear();
        }

        is_server_ = false;
    }

    // Close client sockets
    if (client_tcp_socket_) {
        boost::system::error_code ec;
        client_tcp_socket_->close(ec);
        client_tcp_socket_.reset();
    }

    if (client_udp_socket_) {
        boost::system::error_code ec;
        client_udp_socket_->close(ec);
        client_udp_socket_.reset();
    }

    server_udp_endpoint_.reset();
    udp_recv_endpoint_.reset();

    // Now stop the IO context and join the thread
    if (io_work_)
        io_work_.reset();
    if (io_context_)
        io_context_->stop();
    if (io_thread_ && io_thread_->joinable())
        io_thread_->join();

    io_context_.reset();
    io_thread_.reset();

    // Clear packet queue
    {
        std::lock_guard<std::mutex> lock(packet_mutex_);
        while (!received_packets_.empty())
            received_packets_.pop();
    }

    // Clear callbacks to prevent dangling references
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        on_client_connected_ = nullptr;
        on_client_disconnected_ = nullptr;
        on_packet_received_ = nullptr;
        on_connected_ = nullptr;
        on_disconnected_ = nullptr;
    }

    initialized_ = false;
}

bool AsioNetworkPlugin::is_initialized() const
{
    return initialized_;
}

// ============== Server Operations ==============

bool AsioNetworkPlugin::start_server(uint16_t tcp_port, uint16_t udp_port)
{
    // Default behavior: listen on localhost only (127.0.0.1)
    return start_server(tcp_port, udp_port, false);
}

bool AsioNetworkPlugin::start_server(uint16_t tcp_port, uint16_t udp_port, bool listen_on_all_interfaces)
{
    if (!initialized_) {
        std::cerr << "[AsioNetworkPlugin] Cannot start server: not initialized" << std::endl;
        return false;
    }
    if (is_server_) {
        std::cerr << "[AsioNetworkPlugin] Server already running" << std::endl;
        return false;
    }

    try {
        tcp_port_ = tcp_port;
        udp_port_ = udp_port;

        // Choose bind address based on configuration
        boost::asio::ip::address bind_address = listen_on_all_interfaces
            ? boost::asio::ip::address_v4::any()      // 0.0.0.0 - all interfaces
            : boost::asio::ip::address_v4::loopback(); // 127.0.0.1 - localhost only

        // Start TCP acceptor
        tcp::endpoint tcp_endpoint(bind_address, tcp_port);
        tcp_acceptor_ = std::make_unique<tcp::acceptor>(*io_context_, tcp_endpoint);
        tcp_acceptor_->set_option(tcp::acceptor::reuse_address(true));

        // Start UDP socket
        udp::endpoint udp_endpoint(bind_address, udp_port);
        udp_socket_ = std::make_unique<udp::socket>(*io_context_, udp_endpoint);
        udp_recv_endpoint_ = std::make_unique<udp::endpoint>();

        is_server_ = true;
        running_ = true;

        // Keep io_context running
        io_work_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
            boost::asio::make_work_guard(*io_context_));

        // Start async operations
        start_tcp_accept();
        start_udp_receive();

        // Start IO thread
        io_thread_ = std::make_unique<std::thread>([this]() { run_io_context(); });

        std::cout << "[AsioNetworkPlugin] Server started on " << bind_address.to_string()
                  << " - TCP:" << tcp_port << " UDP:" << udp_port << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] Failed to start server: " << e.what() << std::endl;
        is_server_ = false;
        running_ = false;
        tcp_acceptor_.reset();
        udp_socket_.reset();
        return false;
    }
}

void AsioNetworkPlugin::stop_server()
{
    if (!is_server_)
        return;

    running_ = false;
    is_server_ = false;

    // Close TCP acceptor
    if (tcp_acceptor_) {
        error_code ec;
        tcp_acceptor_->close(ec);
    }

    // Close all TCP client sockets
    {
        std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
        for (auto& [id, client] : tcp_clients_) {
            if (client.socket) {
                error_code ec;
                client.socket->close(ec);
            }
        }
        tcp_clients_.clear();
    }

    // Close UDP socket
    if (udp_socket_) {
        error_code ec;
        udp_socket_->close(ec);
    }

    // Clear UDP clients
    {
        std::lock_guard<std::mutex> lock(udp_clients_mutex_);
        udp_clients_by_endpoint_.clear();
        udp_clients_by_id_.clear();
    }

    // Clear associations
    {
        std::lock_guard<std::mutex> lock(association_mutex_);
        tcp_to_udp_.clear();
        udp_to_tcp_.clear();
    }

    // Stop IO
    if (io_work_)
        io_work_.reset();
    if (io_context_)
        io_context_->stop();
    if (io_thread_ && io_thread_->joinable())
        io_thread_->join();

    // Recreate io_context for potential reuse
    io_context_ = std::make_unique<io_context>();
    io_thread_.reset();

    std::cout << "[AsioNetworkPlugin] Server stopped" << std::endl;
}

bool AsioNetworkPlugin::is_server_running() const
{
    return is_server_ && running_;
}

// ============== TCP Server Methods ==============

void AsioNetworkPlugin::start_tcp_accept()
{
    if (!tcp_acceptor_ || !running_)
        return;

    auto socket = std::make_shared<tcp::socket>(*io_context_);
    tcp_acceptor_->async_accept(*socket,
        [this, socket](const error_code& ec) {
            handle_tcp_accept(socket, ec);
        });
}

void AsioNetworkPlugin::handle_tcp_accept(std::shared_ptr<tcp::socket> socket,
                                          const error_code& error)
{
    if (error) {
        if (error == boost::asio::error::operation_aborted)
            return;
        std::cerr << "[AsioNetworkPlugin] TCP accept error: " << error.message() << std::endl;
        start_tcp_accept();
        return;
    }

    ClientId client_id = generate_client_id();

    {
        std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
        TcpClientInfo& client = tcp_clients_[client_id];
        client.id = client_id;
        client.socket = socket;
        client.read_buffer.resize(TCP_READ_BUFFER_SIZE);
        client.last_seen = std::chrono::steady_clock::now();
    }

    std::cout << "[AsioNetworkPlugin] TCP client connected: " << client_id
              << " from " << socket->remote_endpoint() << std::endl;

    // Notify callback
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (on_client_connected_)
            on_client_connected_(client_id);
    }

    // Start receiving from this client
    start_tcp_receive(client_id);

    // Accept next connection
    start_tcp_accept();
}

void AsioNetworkPlugin::start_tcp_receive(ClientId client_id)
{
    std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
    auto it = tcp_clients_.find(client_id);
    if (it == tcp_clients_.end() || !it->second.socket)
        return;

    auto& client = it->second;
    auto& buffer = client.read_buffer;

    // First read the header (8 bytes)
    boost::asio::async_read(*client.socket,
        boost::asio::buffer(buffer.data(), TCP_HEADER_SIZE),
        [this, client_id](const error_code& ec, size_t bytes) {
            if (ec) {
                handle_tcp_disconnect(client_id);
                return;
            }

            // Get payload length from header (bytes 2-3, big-endian)
            std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
            auto it = tcp_clients_.find(client_id);
            if (it == tcp_clients_.end())
                return;

            auto& buffer = it->second.read_buffer;
            // Payload length is at bytes 3-4 (after version, type, flags)
            uint16_t payload_len = (static_cast<uint16_t>(buffer[3]) << 8) |
                                   static_cast<uint16_t>(buffer[4]);

            if (payload_len == 0) {
                // No payload, packet is complete
                NetworkPacket packet(buffer.data(), TCP_HEADER_SIZE);
                packet.sender_id = client_id;
                packet.protocol = NetworkProtocol::TCP;
                packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();

                {
                    std::lock_guard<std::mutex> plock(packet_mutex_);
                    received_packets_.push(packet);
                }

                it->second.last_seen = std::chrono::steady_clock::now();

                // Continue receiving
                // Need to release lock before calling start_tcp_receive
                // So we post it to io_context
                boost::asio::post(*io_context_, [this, client_id]() {
                    start_tcp_receive(client_id);
                });
                return;
            }

            // Read payload
            boost::asio::async_read(*it->second.socket,
                boost::asio::buffer(buffer.data() + TCP_HEADER_SIZE, payload_len),
                [this, client_id, payload_len](const error_code& ec, size_t bytes) {
                    handle_tcp_receive(client_id, ec, TCP_HEADER_SIZE + payload_len);
                });
        });
}

void AsioNetworkPlugin::handle_tcp_receive(ClientId client_id, const error_code& error,
                                           size_t bytes_transferred)
{
    if (error) {
        handle_tcp_disconnect(client_id);
        return;
    }

    std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
    auto it = tcp_clients_.find(client_id);
    if (it == tcp_clients_.end())
        return;

    auto& client = it->second;
    client.last_seen = std::chrono::steady_clock::now();

    // Create packet
    NetworkPacket packet(client.read_buffer.data(), bytes_transferred);
    packet.sender_id = client_id;
    packet.protocol = NetworkProtocol::TCP;
    packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    {
        std::lock_guard<std::mutex> plock(packet_mutex_);
        received_packets_.push(packet);
    }

    {
        std::lock_guard<std::mutex> clock(callback_mutex_);
        if (on_packet_received_)
            on_packet_received_(client_id, packet);
    }

    // Continue receiving - post to avoid recursive lock
    boost::asio::post(*io_context_, [this, client_id]() {
        start_tcp_receive(client_id);
    });
}

void AsioNetworkPlugin::handle_tcp_disconnect(ClientId client_id)
{
    std::shared_ptr<tcp::socket> socket_to_close;

    {
        std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
        auto it = tcp_clients_.find(client_id);
        if (it == tcp_clients_.end())
            return;

        socket_to_close = it->second.socket;
        tcp_clients_.erase(it);
    }

    if (socket_to_close) {
        error_code ec;
        socket_to_close->close(ec);
    }

    // Remove UDP association
    {
        std::lock_guard<std::mutex> lock(association_mutex_);
        auto it = tcp_to_udp_.find(client_id);
        if (it != tcp_to_udp_.end()) {
            udp_to_tcp_.erase(it->second);
            tcp_to_udp_.erase(it);
        }
    }

    std::cout << "[AsioNetworkPlugin] TCP client disconnected: " << client_id << std::endl;

    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (on_client_disconnected_)
            on_client_disconnected_(client_id);
    }
}

// ============== UDP Server Methods ==============

void AsioNetworkPlugin::start_udp_receive()
{
    if (!udp_socket_ || !running_)
        return;

    udp_socket_->async_receive_from(
        boost::asio::buffer(udp_recv_buffer_),
        *udp_recv_endpoint_,
        [this](const error_code& ec, size_t bytes) {
            handle_udp_receive(ec, bytes);
        });
}

void AsioNetworkPlugin::handle_udp_receive(const error_code& error, size_t bytes_transferred)
{
    // DEBUG: Log every UDP receive
    std::cout << "[AsioNetworkPlugin] UDP raw receive: " << bytes_transferred << " bytes from " 
              << udp_recv_endpoint_->address().to_string() << ":" << udp_recv_endpoint_->port() << std::endl;

    if (error) {
        if (error == boost::asio::error::operation_aborted)
            return;
        std::cerr << "[AsioNetworkPlugin] UDP receive error: " << error.message() << std::endl;
        start_udp_receive();
        return;
    }

    if (bytes_transferred == 0) {
        start_udp_receive();
        return;
    }

    std::string endpoint_key = endpoint_to_string(*udp_recv_endpoint_);
    ClientId udp_client_id = get_or_create_udp_client(endpoint_key);

    // Create packet
    NetworkPacket packet(udp_recv_buffer_.data(), bytes_transferred);
    packet.sender_id = udp_client_id;
    packet.protocol = NetworkProtocol::UDP;
    packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    {
        std::lock_guard<std::mutex> lock(packet_mutex_);
        received_packets_.push(packet);
    }

    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (on_packet_received_)
            on_packet_received_(udp_client_id, packet);
    }

    start_udp_receive();
}

ClientId AsioNetworkPlugin::get_or_create_udp_client(const std::string& endpoint_key)
{
    std::lock_guard<std::mutex> lock(udp_clients_mutex_);

    auto it = udp_clients_by_endpoint_.find(endpoint_key);
    if (it != udp_clients_by_endpoint_.end()) {
        it->second.last_seen = std::chrono::steady_clock::now();
        return it->second.id;
    }

    // New UDP client
    ClientId new_id = generate_client_id();
    UdpClientInfo& info = udp_clients_by_endpoint_[endpoint_key];
    info.id = new_id;
    info.endpoint = std::make_unique<udp::endpoint>(*udp_recv_endpoint_);
    info.last_seen = std::chrono::steady_clock::now();
    udp_clients_by_id_[new_id] = endpoint_key;

    std::cout << "[AsioNetworkPlugin] New UDP client: " << new_id
              << " from " << endpoint_key << std::endl;

    return new_id;
}

std::string AsioNetworkPlugin::endpoint_to_string(const udp::endpoint& endpoint)
{
    return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
}

// ============== Client Operations ==============

bool AsioNetworkPlugin::connect_tcp(const std::string& host, uint16_t port)
{
    if (!initialized_) {
        std::cerr << "[AsioNetworkPlugin] Cannot connect: not initialized" << std::endl;
        return false;
    }
    if (tcp_connected_) {
        std::cerr << "[AsioNetworkPlugin] Already connected via TCP" << std::endl;
        return false;
    }

    try {
        server_host_ = host;
        tcp_port_ = port;

        // Resolve host
        tcp::resolver resolver(*io_context_);
        auto endpoints = resolver.resolve(tcp::v4(), host, std::to_string(port));
        if (endpoints.empty()) {
            std::cerr << "[AsioNetworkPlugin] Could not resolve host: " << host << std::endl;
            return false;
        }

        // Create socket and connect
        client_tcp_socket_ = std::make_unique<tcp::socket>(*io_context_);
        boost::asio::connect(*client_tcp_socket_, endpoints);

        tcp_connected_ = true;
        running_ = true;

        // Keep io_context running
        if (!io_work_)
            io_work_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
                boost::asio::make_work_guard(*io_context_));

        // Start receiving
        start_client_tcp_receive();

        // Start IO thread if not already running
        if (!io_thread_ || !io_thread_->joinable()) {
            io_thread_ = std::make_unique<std::thread>([this]() { run_io_context(); });
        }

        std::cout << "[AsioNetworkPlugin] Connected to " << host << ":" << port << " via TCP" << std::endl;

        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            if (on_connected_)
                on_connected_();
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] TCP connection failed: " << e.what() << std::endl;
        tcp_connected_ = false;
        client_tcp_socket_.reset();
        return false;
    }
}

bool AsioNetworkPlugin::connect_udp(const std::string& host, uint16_t port)
{
    if (!initialized_) {
        std::cerr << "[AsioNetworkPlugin] Cannot connect UDP: not initialized" << std::endl;
        return false;
    }
    if (udp_connected_) {
        std::cerr << "[AsioNetworkPlugin] Already connected via UDP" << std::endl;
        return false;
    }

    try {
        udp_port_ = port;

        // Resolve host
        udp::resolver resolver(*io_context_);
        auto endpoints = resolver.resolve(udp::v4(), host, std::to_string(port));
        if (endpoints.empty()) {
            std::cerr << "[AsioNetworkPlugin] Could not resolve UDP host: " << host << std::endl;
            return false;
        }

        // Create UDP socket
        client_udp_socket_ = std::make_unique<udp::socket>(*io_context_);
        client_udp_socket_->open(udp::v4());
        server_udp_endpoint_ = std::make_unique<udp::endpoint>(*endpoints.begin());

        udp_connected_ = true;

        // Start receiving
        start_client_udp_receive();

        std::cout << "[AsioNetworkPlugin] Connected to " << host << ":" << port << " via UDP" << std::endl;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] UDP connection failed: " << e.what() << std::endl;
        udp_connected_ = false;
        client_udp_socket_.reset();
        server_udp_endpoint_.reset();
        return false;
    }
}

void AsioNetworkPlugin::disconnect()
{
    bool was_connected = tcp_connected_ || udp_connected_;

    tcp_connected_ = false;
    udp_connected_ = false;

    // Close client sockets - this will cancel pending async operations
    if (client_tcp_socket_) {
        boost::system::error_code ec;
        client_tcp_socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        client_tcp_socket_->close(ec);
        client_tcp_socket_.reset();
    }

    if (client_udp_socket_) {
        boost::system::error_code ec;
        client_udp_socket_->close(ec);
        client_udp_socket_.reset();
    }

    server_udp_endpoint_.reset();

    // If we're a client, stop the IO context and thread
    if (!is_server_) {
        running_ = false;
        
        // Release work guard first to allow io_context to finish
        if (io_work_)
            io_work_.reset();
        
        // Stop the context
        if (io_context_)
            io_context_->stop();
        
        // Wait for the thread to finish
        if (io_thread_ && io_thread_->joinable())
            io_thread_->join();
        
        io_thread_.reset();
        
        // Reset and recreate io_context for potential reconnection
        io_context_.reset();
        io_context_ = std::make_unique<boost::asio::io_context>();
    }

    if (was_connected) {
        std::cout << "[AsioNetworkPlugin] Disconnected" << std::endl;
        
        // Call callback without holding any locks that could cause deadlock
        std::function<void()> callback;
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            callback = on_disconnected_;
        }
        if (callback)
            callback();
    }
}

bool AsioNetworkPlugin::is_tcp_connected() const
{
    return tcp_connected_;
}

bool AsioNetworkPlugin::is_udp_connected() const
{
    return udp_connected_;
}

// ============== Client TCP Methods ==============

void AsioNetworkPlugin::start_client_tcp_receive()
{
    if (!client_tcp_socket_ || !tcp_connected_)
        return;

    // Read header first
    boost::asio::async_read(*client_tcp_socket_,
        boost::asio::buffer(client_tcp_read_buffer_.data(), TCP_HEADER_SIZE),
        [this](const error_code& ec, size_t bytes) {
            if (ec) {
                std::cerr << "[AsioNetworkPlugin] TCP receive error: " << ec.message() << std::endl;
                disconnect();
                return;
            }

            // Get payload length (at bytes 3-4 after version, type, flags)
            uint16_t payload_len = (static_cast<uint16_t>(client_tcp_read_buffer_[3]) << 8) |
                                   static_cast<uint16_t>(client_tcp_read_buffer_[4]);

            if (payload_len == 0) {
                // No payload
                handle_client_tcp_receive(error_code(), TCP_HEADER_SIZE);
                return;
            }

            // Read payload
            boost::asio::async_read(*client_tcp_socket_,
                boost::asio::buffer(client_tcp_read_buffer_.data() + TCP_HEADER_SIZE, payload_len),
                [this, payload_len](const error_code& ec, size_t bytes) {
                    handle_client_tcp_receive(ec, TCP_HEADER_SIZE + payload_len);
                });
        });
}

void AsioNetworkPlugin::handle_client_tcp_receive(const error_code& error, size_t bytes_transferred)
{
    if (error) {
        std::cerr << "[AsioNetworkPlugin] TCP receive error: " << error.message() << std::endl;
        disconnect();
        return;
    }

    NetworkPacket packet(client_tcp_read_buffer_.data(), bytes_transferred);
    packet.sender_id = 0;  // From server
    packet.protocol = NetworkProtocol::TCP;
    packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    {
        std::lock_guard<std::mutex> lock(packet_mutex_);
        received_packets_.push(packet);
    }

    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (on_packet_received_)
            on_packet_received_(0, packet);
    }

    start_client_tcp_receive();
}

// ============== Client UDP Methods ==============

void AsioNetworkPlugin::start_client_udp_receive()
{
    if (!client_udp_socket_ || !udp_connected_)
        return;

    client_udp_socket_->async_receive(
        boost::asio::buffer(client_udp_recv_buffer_),
        [this](const error_code& ec, size_t bytes) {
            handle_client_udp_receive(ec, bytes);
        });
}

void AsioNetworkPlugin::handle_client_udp_receive(const error_code& error, size_t bytes_transferred)
{
    if (error) {
        if (error == boost::asio::error::operation_aborted)
            return;
        std::cerr << "[AsioNetworkPlugin] UDP receive error: " << error.message() << std::endl;
        start_client_udp_receive();
        return;
    }

    if (bytes_transferred == 0) {
        start_client_udp_receive();
        return;
    }

    NetworkPacket packet(client_udp_recv_buffer_.data(), bytes_transferred);
    packet.sender_id = 0;  // From server
    packet.protocol = NetworkProtocol::UDP;
    packet.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    {
        std::lock_guard<std::mutex> lock(packet_mutex_);
        received_packets_.push(packet);
    }

    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (on_packet_received_)
            on_packet_received_(0, packet);
    }

    start_client_udp_receive();
}

// ============== Client Communication ==============

bool AsioNetworkPlugin::send_tcp(const NetworkPacket& packet)
{
    if (!tcp_connected_ || !client_tcp_socket_) {
        std::cerr << "[AsioNetworkPlugin] Cannot send TCP: not connected" << std::endl;
        return false;
    }

    try {
        boost::asio::write(*client_tcp_socket_, boost::asio::buffer(packet.data));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] TCP send failed: " << e.what() << std::endl;
        return false;
    }
}

bool AsioNetworkPlugin::send_udp(const NetworkPacket& packet)
{
    if (!udp_connected_ || !client_udp_socket_ || !server_udp_endpoint_) {
        std::cerr << "[AsioNetworkPlugin] Cannot send UDP: not connected" << std::endl;
        return false;
    }

    try {
        client_udp_socket_->send_to(boost::asio::buffer(packet.data), *server_udp_endpoint_);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] UDP send failed: " << e.what() << std::endl;
        return false;
    }
}

// ============== Server Communication ==============

bool AsioNetworkPlugin::send_tcp_to(const NetworkPacket& packet, ClientId client_id)
{
    if (!is_server_) {
        std::cerr << "[AsioNetworkPlugin] Cannot send_tcp_to: not in server mode" << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
    auto it = tcp_clients_.find(client_id);
    if (it == tcp_clients_.end() || !it->second.socket) {
        std::cerr << "[AsioNetworkPlugin] TCP client " << client_id << " not found" << std::endl;
        return false;
    }

    try {
        boost::asio::write(*it->second.socket, boost::asio::buffer(packet.data));
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] TCP send_to failed: " << e.what() << std::endl;
        return false;
    }
}

bool AsioNetworkPlugin::send_udp_to(const NetworkPacket& packet, ClientId client_id)
{
    if (!is_server_ || !udp_socket_) {
        std::cerr << "[AsioNetworkPlugin] Cannot send_udp_to: not in server mode" << std::endl;
        return false;
    }

    // First check if this is a TCP client with UDP association
    ClientId udp_client_id = client_id;
    {
        std::lock_guard<std::mutex> lock(association_mutex_);
        auto it = tcp_to_udp_.find(client_id);
        if (it != tcp_to_udp_.end()) {
            udp_client_id = it->second;
        }
    }

    std::lock_guard<std::mutex> lock(udp_clients_mutex_);
    auto it = udp_clients_by_id_.find(udp_client_id);
    if (it == udp_clients_by_id_.end()) {
        std::cerr << "[AsioNetworkPlugin] UDP client " << udp_client_id << " not found" << std::endl;
        return false;
    }

    auto client_it = udp_clients_by_endpoint_.find(it->second);
    if (client_it == udp_clients_by_endpoint_.end() || !client_it->second.endpoint) {
        return false;
    }

    try {
        udp_socket_->send_to(boost::asio::buffer(packet.data), *client_it->second.endpoint);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] UDP send_to failed: " << e.what() << std::endl;
        return false;
    }
}

size_t AsioNetworkPlugin::broadcast_tcp(const NetworkPacket& packet)
{
    if (!is_server_) {
        std::cerr << "[AsioNetworkPlugin] Cannot broadcast_tcp: not in server mode" << std::endl;
        return 0;
    }

    size_t count = 0;
    std::lock_guard<std::mutex> lock(tcp_clients_mutex_);

    for (auto& [id, client] : tcp_clients_) {
        if (!client.socket)
            continue;
        try {
            boost::asio::write(*client.socket, boost::asio::buffer(packet.data));
            count++;
        } catch (const std::exception& e) {
            std::cerr << "[AsioNetworkPlugin] TCP broadcast to " << id << " failed: " << e.what() << std::endl;
        }
    }
    return count;
}

size_t AsioNetworkPlugin::broadcast_udp(const NetworkPacket& packet)
{
    if (!is_server_ || !udp_socket_) {
        std::cerr << "[AsioNetworkPlugin] Cannot broadcast_udp: not in server mode" << std::endl;
        return 0;
    }

    size_t count = 0;

    // Only broadcast to clients that have UDP association
    std::vector<udp::endpoint> endpoints;
    {
        std::lock_guard<std::mutex> alock(association_mutex_);
        std::lock_guard<std::mutex> ulock(udp_clients_mutex_);

        for (const auto& [tcp_id, udp_id] : tcp_to_udp_) {
            auto it = udp_clients_by_id_.find(udp_id);
            if (it != udp_clients_by_id_.end()) {
                auto client_it = udp_clients_by_endpoint_.find(it->second);
                if (client_it != udp_clients_by_endpoint_.end() && client_it->second.endpoint) {
                    endpoints.push_back(*client_it->second.endpoint);
                }
            }
        }
    }

    for (const auto& endpoint : endpoints) {
        try {
            udp_socket_->send_to(boost::asio::buffer(packet.data), endpoint);
            count++;
        } catch (const std::exception& e) {
            std::cerr << "[AsioNetworkPlugin] UDP broadcast failed: " << e.what() << std::endl;
        }
    }
    return count;
}

size_t AsioNetworkPlugin::broadcast_tcp_except(const NetworkPacket& packet, ClientId exclude_client_id)
{
    if (!is_server_) {
        return 0;
    }

    size_t count = 0;
    std::lock_guard<std::mutex> lock(tcp_clients_mutex_);

    for (auto& [id, client] : tcp_clients_) {
        if (id == exclude_client_id || !client.socket)
            continue;
        try {
            boost::asio::write(*client.socket, boost::asio::buffer(packet.data));
            count++;
        } catch (const std::exception& e) {
            std::cerr << "[AsioNetworkPlugin] TCP broadcast to " << id << " failed: " << e.what() << std::endl;
        }
    }
    return count;
}

size_t AsioNetworkPlugin::broadcast_udp_except(const NetworkPacket& packet, ClientId exclude_client_id)
{
    if (!is_server_ || !udp_socket_) {
        return 0;
    }

    size_t count = 0;

    std::vector<std::pair<ClientId, udp::endpoint>> endpoints;
    {
        std::lock_guard<std::mutex> alock(association_mutex_);
        std::lock_guard<std::mutex> ulock(udp_clients_mutex_);

        for (const auto& [tcp_id, udp_id] : tcp_to_udp_) {
            if (tcp_id == exclude_client_id)
                continue;
            auto it = udp_clients_by_id_.find(udp_id);
            if (it != udp_clients_by_id_.end()) {
                auto client_it = udp_clients_by_endpoint_.find(it->second);
                if (client_it != udp_clients_by_endpoint_.end() && client_it->second.endpoint) {
                    endpoints.emplace_back(tcp_id, *client_it->second.endpoint);
                }
            }
        }
    }

    for (const auto& [id, endpoint] : endpoints) {
        try {
            udp_socket_->send_to(boost::asio::buffer(packet.data), endpoint);
            count++;
        } catch (const std::exception& e) {
            std::cerr << "[AsioNetworkPlugin] UDP broadcast failed: " << e.what() << std::endl;
        }
    }
    return count;
}

// ============== UDP Association ==============

void AsioNetworkPlugin::associate_udp_client(ClientId tcp_client_id, ClientId udp_client_id)
{
    std::lock_guard<std::mutex> lock(association_mutex_);
    tcp_to_udp_[tcp_client_id] = udp_client_id;
    udp_to_tcp_[udp_client_id] = tcp_client_id;

    std::cout << "[AsioNetworkPlugin] Associated TCP client " << tcp_client_id
              << " with UDP client " << udp_client_id << std::endl;
}

ClientId AsioNetworkPlugin::get_tcp_client_from_udp(ClientId udp_client_id) const
{
    std::lock_guard<std::mutex> lock(association_mutex_);
    auto it = udp_to_tcp_.find(udp_client_id);
    return (it != udp_to_tcp_.end()) ? it->second : 0;
}

bool AsioNetworkPlugin::has_udp_association(ClientId tcp_client_id) const
{
    std::lock_guard<std::mutex> lock(association_mutex_);
    return tcp_to_udp_.find(tcp_client_id) != tcp_to_udp_.end();
}

// ============== Receiving ==============

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

// ============== Callbacks ==============

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

// ============== Statistics ==============

size_t AsioNetworkPlugin::get_client_count() const
{
    std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
    return tcp_clients_.size();
}

std::vector<ClientId> AsioNetworkPlugin::get_client_ids() const
{
    std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
    std::vector<ClientId> ids;
    ids.reserve(tcp_clients_.size());

    for (const auto& [id, client] : tcp_clients_)
        ids.push_back(id);
    return ids;
}

int AsioNetworkPlugin::get_client_ping(ClientId client_id) const
{
    std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
    auto it = tcp_clients_.find(client_id);
    return (it != tcp_clients_.end()) ? it->second.ping_ms : -1;
}

int AsioNetworkPlugin::get_server_ping() const
{
    return server_ping_ms_;
}

// ============== Private Methods ==============

void AsioNetworkPlugin::run_io_context()
{
    try {
        io_context_->run();
    } catch (const std::exception& e) {
        std::cerr << "[AsioNetworkPlugin] IO context error: " << e.what() << std::endl;
    }
}

ClientId AsioNetworkPlugin::generate_client_id()
{
    return next_client_id_++;
}

void AsioNetworkPlugin::check_client_timeouts()
{
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - last_timeout_check_).count();

    if (elapsed < TIMEOUT_CHECK_INTERVAL)
        return;

    last_timeout_check_ = now;

    // Check TCP timeouts
    std::vector<ClientId> timed_out;
    {
        std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
        for (const auto& [id, client] : tcp_clients_) {
            float inactive = std::chrono::duration<float>(now - client.last_seen).count();
            if (inactive > CLIENT_TIMEOUT_SECONDS)
                timed_out.push_back(id);
        }
    }

    for (ClientId id : timed_out) {
        std::cout << "[AsioNetworkPlugin] TCP client " << id << " timed out" << std::endl;
        handle_tcp_disconnect(id);
    }
}

void AsioNetworkPlugin::disconnect_client(ClientId client_id)
{
    if (!is_server_)
        return;

    std::cout << "[AsioNetworkPlugin] Disconnecting client " << client_id << std::endl;
    {
        std::lock_guard<std::mutex> lock(tcp_clients_mutex_);
        auto it = tcp_clients_.find(client_id);
        if (it != tcp_clients_.end()) {
            if (it->second.socket) {
                error_code ec;
                it->second.socket->shutdown(tcp::socket::shutdown_both, ec);
                it->second.socket->close(ec);
            }
            tcp_clients_.erase(it);
        }
    }
    {
        std::lock_guard<std::mutex> lock(association_mutex_);
        auto udp_id_it = tcp_to_udp_.find(client_id);
        if (udp_id_it != tcp_to_udp_.end()) {
            ClientId udp_client_id = udp_id_it->second;
            udp_to_tcp_.erase(udp_client_id);
            tcp_to_udp_.erase(udp_id_it);
        }
    }
    {
        std::lock_guard<std::mutex> lock(udp_clients_mutex_);
        auto udp_it = udp_clients_by_id_.find(client_id);
        if (udp_it != udp_clients_by_id_.end()) {
            std::string endpoint = udp_it->second;
            udp_clients_by_endpoint_.erase(endpoint);
            udp_clients_by_id_.erase(udp_it);
        }
    }
    if (on_client_disconnected_)
        on_client_disconnected_(client_id);
}

}

extern "C" {
    PLUGIN_API engine::INetworkPlugin* create_network_plugin() {
        return new engine::AsioNetworkPlugin();
    }

    PLUGIN_API void destroy_network_plugin(engine::INetworkPlugin* plugin) {
        delete plugin;
    }
}
