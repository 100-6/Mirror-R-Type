/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AsioNetworkPlugin - Boost.Asio implementation of INetworkPlugin
** Supports hybrid TCP/UDP architecture
*/

#pragma once

#include "plugin_manager/INetworkPlugin.hpp"
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <memory>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

namespace engine {

/**
 * @brief Boost.Asio implementation of the network plugin interface
 *
 * This plugin uses Boost.Asio for hybrid TCP/UDP networking with async operations.
 * - TCP: Reliable communication for connections, lobbies, authentication
 * - UDP: Low-latency communication for gameplay
 *
 * Server mode: Listens on both TCP and UDP ports
 * Client mode: Connects via TCP first, then UDP when gameplay starts
 */
class AsioNetworkPlugin : public INetworkPlugin {
public:
    AsioNetworkPlugin();
    ~AsioNetworkPlugin() override;

    // IPlugin interface
    const char* get_name() const override;
    const char* get_version() const override;
    bool initialize() override;
    void shutdown() override;
    bool is_initialized() const override;

    // Server operations
    bool start_server(uint16_t tcp_port, uint16_t udp_port) override;
    bool start_server(uint16_t tcp_port, uint16_t udp_port, bool listen_on_all_interfaces);
    void stop_server() override;
    bool is_server_running() const override;

    // Client operations
    bool connect_tcp(const std::string& host, uint16_t port) override;
    bool connect_udp(const std::string& host, uint16_t port) override;
    void disconnect() override;
    bool is_tcp_connected() const override;
    bool is_udp_connected() const override;

    // Client communication
    bool send_tcp(const NetworkPacket& packet) override;
    bool send_udp(const NetworkPacket& packet) override;

    // Server communication
    bool send_tcp_to(const NetworkPacket& packet, ClientId client_id) override;
    bool send_udp_to(const NetworkPacket& packet, ClientId client_id) override;
    size_t broadcast_tcp(const NetworkPacket& packet) override;
    size_t broadcast_udp(const NetworkPacket& packet) override;
    size_t broadcast_tcp_except(const NetworkPacket& packet, ClientId exclude_client_id) override;
    size_t broadcast_udp_except(const NetworkPacket& packet, ClientId exclude_client_id) override;

    // UDP association
    void associate_udp_client(ClientId tcp_client_id, ClientId udp_client_id) override;
    ClientId get_tcp_client_from_udp(ClientId udp_client_id) const override;
    bool has_udp_association(ClientId tcp_client_id) const override;

    // Receiving
    std::vector<NetworkPacket> receive() override;
    void update(float delta_time) override;

    // Callbacks
    void set_on_client_connected(std::function<void(ClientId)> callback) override;
    void set_on_client_disconnected(std::function<void(ClientId)> callback) override;
    void set_on_packet_received(std::function<void(ClientId, const NetworkPacket&)> callback) override;
    void set_on_connected(std::function<void()> callback) override;
    void set_on_disconnected(std::function<void()> callback) override;

    // Statistics
    size_t get_client_count() const override;
    std::vector<ClientId> get_client_ids() const override;
    int get_client_ping(ClientId client_id) const override;
    int get_server_ping() const override;

private:
    // TCP client info (server side)
    struct TcpClientInfo {
        ClientId id;
        std::shared_ptr<boost::asio::ip::tcp::socket> socket;
        std::vector<uint8_t> read_buffer;
        std::chrono::steady_clock::time_point last_seen;
        int ping_ms = 0;
    };

    // UDP client info (server side)
    struct UdpClientInfo {
        ClientId id;
        std::unique_ptr<boost::asio::ip::udp::endpoint> endpoint;
        std::chrono::steady_clock::time_point last_seen;
    };

    // TCP server methods
    void start_tcp_accept();
    void handle_tcp_accept(std::shared_ptr<boost::asio::ip::tcp::socket> socket,
                          const boost::system::error_code& error);
    void start_tcp_receive(ClientId client_id);
    void handle_tcp_receive(ClientId client_id, const boost::system::error_code& error,
                           size_t bytes_transferred);
    void handle_tcp_disconnect(ClientId client_id);

    // UDP server methods
    void start_udp_receive();
    void handle_udp_receive(const boost::system::error_code& error, size_t bytes_transferred);
    ClientId get_or_create_udp_client(const std::string& endpoint_key);
    std::string endpoint_to_string(const boost::asio::ip::udp::endpoint& endpoint);

    // Client TCP methods
    void start_client_tcp_receive();
    void handle_client_tcp_receive(const boost::system::error_code& error, size_t bytes_transferred);

    // Client UDP methods
    void start_client_udp_receive();
    void handle_client_udp_receive(const boost::system::error_code& error, size_t bytes_transferred);

    // Common methods
    void run_io_context();
    ClientId generate_client_id();
    void check_client_timeouts();

    // State
    bool initialized_ = false;
    bool is_server_ = false;
    std::atomic<bool> running_{false};
    std::atomic<bool> tcp_connected_{false};
    std::atomic<bool> udp_connected_{false};

    // IO context and thread
    std::unique_ptr<boost::asio::io_context> io_context_;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> io_work_;
    std::unique_ptr<std::thread> io_thread_;

    // Server TCP
    std::unique_ptr<boost::asio::ip::tcp::acceptor> tcp_acceptor_;
    std::unordered_map<ClientId, TcpClientInfo> tcp_clients_;
    mutable std::mutex tcp_clients_mutex_;

    // Server UDP
    std::unique_ptr<boost::asio::ip::udp::socket> udp_socket_;
    std::unique_ptr<boost::asio::ip::udp::endpoint> udp_recv_endpoint_;
    std::array<uint8_t, 65536> udp_recv_buffer_;
    std::unordered_map<std::string, UdpClientInfo> udp_clients_by_endpoint_;
    std::unordered_map<ClientId, std::string> udp_clients_by_id_;
    mutable std::mutex udp_clients_mutex_;

    // TCP <-> UDP association
    std::unordered_map<ClientId, ClientId> tcp_to_udp_;  // tcp_client_id -> udp_client_id
    std::unordered_map<ClientId, ClientId> udp_to_tcp_;  // udp_client_id -> tcp_client_id
    mutable std::mutex association_mutex_;

    // Client TCP
    std::unique_ptr<boost::asio::ip::tcp::socket> client_tcp_socket_;
    std::vector<uint8_t> client_tcp_read_buffer_;
    std::string server_host_;

    // Client UDP
    std::unique_ptr<boost::asio::ip::udp::socket> client_udp_socket_;
    std::unique_ptr<boost::asio::ip::udp::endpoint> server_udp_endpoint_;
    std::array<uint8_t, 65536> client_udp_recv_buffer_;

    // Ports
    uint16_t tcp_port_ = 0;
    uint16_t udp_port_ = 0;

    // Client ID generation
    ClientId next_client_id_ = 1;

    // Received packets queue
    mutable std::mutex packet_mutex_;
    std::queue<NetworkPacket> received_packets_;

    // Callbacks
    mutable std::mutex callback_mutex_;
    std::function<void(ClientId)> on_client_connected_;
    std::function<void(ClientId)> on_client_disconnected_;
    std::function<void(ClientId, const NetworkPacket&)> on_packet_received_;
    std::function<void()> on_connected_;
    std::function<void()> on_disconnected_;

    // Statistics
    int server_ping_ms_ = -1;
    std::chrono::steady_clock::time_point last_timeout_check_;

    // Constants
    static constexpr float CLIENT_TIMEOUT_SECONDS = 30.0f;
    static constexpr float TIMEOUT_CHECK_INTERVAL = 5.0f;
    static constexpr size_t TCP_HEADER_SIZE = 8;  // Protocol header size
    static constexpr size_t TCP_READ_BUFFER_SIZE = 65536;
};

}
