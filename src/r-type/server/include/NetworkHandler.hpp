#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include "protocol/PacketHeader.hpp"
#include "protocol/PacketTypes.hpp"
#include "protocol/Payloads.hpp"
#include "plugin_manager/INetworkPlugin.hpp"

namespace rtype::server {

/**
 * @brief Handles network packet routing and processing
 *
 * This class is responsible for:
 * - Receiving and decoding packets from the network plugin
 * - Routing packets based on protocol (TCP/UDP) and packet type
 * - Validating packet structure and protocol version
 */
class NetworkHandler {
public:
    // Callback types for different packet handlers
    using ClientConnectCallback = std::function<void(uint32_t, const protocol::ClientConnectPayload&)>;
    using ClientDisconnectCallback = std::function<void(uint32_t, const protocol::ClientDisconnectPayload&)>;
    using ClientPingCallback = std::function<void(uint32_t, const protocol::ClientPingPayload&)>;
    using ClientJoinLobbyCallback = std::function<void(uint32_t, const protocol::ClientJoinLobbyPayload&)>;
    using ClientLeaveLobbyCallback = std::function<void(uint32_t, const protocol::ClientLeaveLobbyPayload&)>;
    using UdpHandshakeCallback = std::function<void(uint32_t, const protocol::ClientUdpHandshakePayload&)>;
    using ClientInputCallback = std::function<void(uint32_t, const protocol::ClientInputPayload&)>;

    /**
     * @brief Construct a new NetworkHandler
     * @param network_plugin Pointer to the network plugin for packet reception
     */
    explicit NetworkHandler(engine::INetworkPlugin* network_plugin);

    /**
     * @brief Process all pending packets from the network
     */
    void process_packets();

    // Setters for packet handlers
    void set_client_connect_handler(ClientConnectCallback callback) { on_client_connect_ = callback; }
    void set_client_disconnect_handler(ClientDisconnectCallback callback) { on_client_disconnect_ = callback; }
    void set_client_ping_handler(ClientPingCallback callback) { on_client_ping_ = callback; }
    void set_client_join_lobby_handler(ClientJoinLobbyCallback callback) { on_client_join_lobby_ = callback; }
    void set_client_leave_lobby_handler(ClientLeaveLobbyCallback callback) { on_client_leave_lobby_ = callback; }
    void set_udp_handshake_handler(UdpHandshakeCallback callback) { on_udp_handshake_ = callback; }
    void set_client_input_handler(ClientInputCallback callback) { on_client_input_ = callback; }

private:
    engine::INetworkPlugin* network_plugin_;

    // Packet handler callbacks
    ClientConnectCallback on_client_connect_;
    ClientDisconnectCallback on_client_disconnect_;
    ClientPingCallback on_client_ping_;
    ClientJoinLobbyCallback on_client_join_lobby_;
    ClientLeaveLobbyCallback on_client_leave_lobby_;
    UdpHandshakeCallback on_udp_handshake_;
    ClientInputCallback on_client_input_;

    /**
     * @brief Route a packet to the appropriate handler based on protocol and type
     */
    void route_packet(uint32_t client_id, const protocol::PacketHeader& header,
                     const std::vector<uint8_t>& payload, engine::NetworkProtocol protocol);

    /**
     * @brief Handle TCP protocol packets (connection, lobby, authentication)
     */
    void handle_tcp_packet(uint32_t client_id, protocol::PacketType type,
                          const std::vector<uint8_t>& payload);

    /**
     * @brief Handle UDP protocol packets (gameplay, inputs)
     */
    void handle_udp_packet(uint32_t client_id, protocol::PacketType type,
                          const std::vector<uint8_t>& payload);
};

}
