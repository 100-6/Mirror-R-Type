/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** NetworkHandler implementation
*/

#include "NetworkHandler.hpp"
#include "protocol/ProtocolEncoder.hpp"
#include <iostream>
#include <cstring>

namespace rtype::server {

NetworkHandler::NetworkHandler(engine::INetworkPlugin* network_plugin)
    : network_plugin_(network_plugin)
{
}

void NetworkHandler::process_packets()
{
    auto packets = network_plugin_->receive();

    for (const auto& packet : packets) {
        if (packet.data.size() < protocol::HEADER_SIZE) {
            std::cerr << "[NetworkHandler] Packet too small from client " << packet.sender_id << "\n";
            continue;
        }

        protocol::PacketHeader header = protocol::ProtocolEncoder::decode_header(
            packet.data.data(), packet.data.size());

        if (header.version != protocol::PROTOCOL_VERSION) {
            std::cerr << "[NetworkHandler] Invalid protocol version from client " << packet.sender_id
                      << ": " << static_cast<int>(header.version) << "\n";
            continue;
        }

        std::vector<uint8_t> payload(
            packet.data.begin() + protocol::HEADER_SIZE,
            packet.data.begin() + protocol::HEADER_SIZE + header.payload_length
        );

        route_packet(packet.sender_id, header, payload, packet.protocol);
    }
}

void NetworkHandler::route_packet(uint32_t client_id, const protocol::PacketHeader& header,
                                  const std::vector<uint8_t>& payload, engine::NetworkProtocol protocol)
{
    auto packet_type = static_cast<protocol::PacketType>(header.type);

    if (protocol == engine::NetworkProtocol::TCP)
        handle_tcp_packet(client_id, packet_type, payload);
    else if (protocol == engine::NetworkProtocol::UDP)
        handle_udp_packet(client_id, packet_type, payload);
}

void NetworkHandler::handle_tcp_packet(uint32_t client_id, protocol::PacketType type,
                                       const std::vector<uint8_t>& payload)
{
    if (!listener_)
        return;

    switch (type) {
        case protocol::PacketType::CLIENT_CONNECT: {
            if (payload.size() != sizeof(protocol::ClientConnectPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_CONNECT payload size\n";
                return;
            }
            protocol::ClientConnectPayload data;
            std::memcpy(&data, payload.data(), sizeof(data));
            listener_->on_client_connect(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_DISCONNECT: {
            if (payload.size() != sizeof(protocol::ClientDisconnectPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_DISCONNECT payload size\n";
                return;
            }
            protocol::ClientDisconnectPayload data;
            std::memcpy(&data, payload.data(), sizeof(data));
            listener_->on_client_disconnect(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_PING: {
            if (payload.size() != sizeof(protocol::ClientPingPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_PING payload size\n";
                return;
            }
            protocol::ClientPingPayload data;
            std::memcpy(&data, payload.data(), sizeof(data));
            listener_->on_client_ping(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_JOIN_LOBBY: {
            if (payload.size() != sizeof(protocol::ClientJoinLobbyPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_JOIN_LOBBY payload size\n";
                return;
            }
            protocol::ClientJoinLobbyPayload data;
            std::memcpy(&data, payload.data(), sizeof(data));
            listener_->on_client_join_lobby(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_LEAVE_LOBBY: {
            if (payload.size() != sizeof(protocol::ClientLeaveLobbyPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_LEAVE_LOBBY payload size\n";
                return;
            }
            protocol::ClientLeaveLobbyPayload data;
            std::memcpy(&data, payload.data(), sizeof(data));
            listener_->on_client_leave_lobby(client_id, data);
            break;
        }
        default:
            std::cerr << "[NetworkHandler] Unexpected TCP packet type: 0x" << std::hex
                      << static_cast<int>(type) << std::dec << "\n";
            break;
    }
}

void NetworkHandler::handle_udp_packet(uint32_t client_id, protocol::PacketType type,
                                       const std::vector<uint8_t>& payload)
{
    if (!listener_)
        return;

    switch (type) {
        case protocol::PacketType::CLIENT_UDP_HANDSHAKE: {
            std::cout << "[NetworkHandler] Processing UDP handshake from client " << client_id << "\n";
            if (payload.size() != sizeof(protocol::ClientUdpHandshakePayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_UDP_HANDSHAKE payload size: "
                          << payload.size() << " expected " << sizeof(protocol::ClientUdpHandshakePayload) << "\n";
                return;
            }
            protocol::ClientUdpHandshakePayload data;
            std::memcpy(&data, payload.data(), sizeof(data));
            listener_->on_udp_handshake(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_INPUT: {
            if (payload.size() != sizeof(protocol::ClientInputPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_INPUT payload size\n";
                return;
            }
            protocol::ClientInputPayload data;
            std::memcpy(&data, payload.data(), sizeof(data));

            // Convert from network byte order
            data.player_id = ntohl(data.player_id);
            data.input_flags = ntohs(data.input_flags);
            data.client_tick = ntohl(data.client_tick);

            listener_->on_client_input(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_PING: {
            if (payload.size() != sizeof(protocol::ClientPingPayload))
                return;
            protocol::ClientPingPayload data;
            std::memcpy(&data, payload.data(), sizeof(data));
            listener_->on_client_ping(client_id, data);
            break;
        }
        default:
            std::cerr << "[NetworkHandler] Unexpected UDP packet type: 0x" << std::hex
                      << static_cast<int>(type) << std::dec << "\n";
            break;
    }
}

}
