/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AsioNetworkPlugin - Boost.Asio implementation of INetworkPlugin
*/

#pragma once

#include "plugin_manager/INetworkPlugin.hpp"
#include <boost/asio.hpp>
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
 * This plugin uses Boost.Asio for UDP networking with async operations.
 * It supports both client and server modes, with callbacks for all network events.
 * All Boost.Asio types are hidden behind forward declarations to avoid leaking
 * headers into the engine.
 */
class AsioNetworkPlugin : public INetworkPlugin {
public:
    AsioNetworkPlugin();
    ~AsioNetworkPlugin() override;

    const char* get_name() const override;
    const char* get_version() const override;
    bool initialize() override;
    void shutdown() override;
    bool is_initialized() const override;

    bool start_server(uint16_t port, NetworkProtocol protocol = NetworkProtocol::UDP) override;
    void stop_server() override;
    bool is_server_running() const override;

    bool connect(const std::string& host, uint16_t port,
                NetworkProtocol protocol = NetworkProtocol::UDP) override;
    void disconnect() override;
    bool is_connected() const override;

    bool send(const NetworkPacket& packet) override;
    bool send_to(const NetworkPacket& packet, ClientId client_id) override;
    size_t broadcast(const NetworkPacket& packet) override;
    size_t broadcast_except(const NetworkPacket& packet, ClientId exclude_client_id) override;
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
    struct ClientInfo;
    void start_async_receive();
    void handle_receive(const boost::system::error_code& error, size_t bytes_transferred);
    void run_io_context();
    ClientId generate_client_id();
    ClientId get_or_create_client_id(const std::string& endpoint_key);
    std::string endpoint_to_string(const boost::asio::ip::udp::endpoint& endpoint);
    void update_client_activity(ClientId client_id);
    void check_client_timeouts();

    bool initialized_;
    bool is_server_;
    bool is_client_;
    std::atomic<bool> running_;

    std::unique_ptr<boost::asio::io_context> io_context_;
    std::unique_ptr<boost::asio::ip::udp::socket> socket_;
    std::unique_ptr<boost::asio::ip::udp::endpoint> server_endpoint_;
    std::unique_ptr<boost::asio::ip::udp::endpoint> recv_endpoint_;
    std::unique_ptr<std::thread> io_thread_;

    uint16_t server_port_;
    NetworkProtocol protocol_;

    struct ClientInfo {
        ClientId id;
        std::unique_ptr<boost::asio::ip::udp::endpoint> endpoint;
        std::chrono::steady_clock::time_point last_seen;
        int ping_ms;
    };

    std::unordered_map<std::string, ClientInfo> clients_by_endpoint_;
    std::unordered_map<ClientId, std::string> clients_by_id_;
    ClientId next_client_id_;

    std::array<uint8_t, 65536> recv_buffer_;

    mutable std::mutex packet_mutex_;
    std::queue<NetworkPacket> received_packets_;

    mutable std::mutex callback_mutex_;
    std::function<void(ClientId)> on_client_connected_;
    std::function<void(ClientId)> on_client_disconnected_;
    std::function<void(ClientId, const NetworkPacket&)> on_packet_received_;
    std::function<void()> on_connected_;
    std::function<void()> on_disconnected_;

    int server_ping_ms_;
    std::chrono::steady_clock::time_point last_timeout_check_;

    static constexpr float CLIENT_TIMEOUT_SECONDS = 30.0f;
    static constexpr float TIMEOUT_CHECK_INTERVAL = 5.0f;
};

}
