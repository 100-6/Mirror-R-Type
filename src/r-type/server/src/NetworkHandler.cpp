/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** NetworkHandler implementation
*/

#include "NetworkHandler.hpp"
#include "protocol/ProtocolEncoder.hpp"
#include "NetworkUtils.hpp"
#include <iostream>

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
    using rtype::server::netutils::Memory;

    if (!listener_)
        return;

    switch (type) {
        case protocol::PacketType::CLIENT_CONNECT: {
            if (payload.size() != sizeof(protocol::ClientConnectPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_CONNECT payload size\n";
                return;
            }
            protocol::ClientConnectPayload data;
            Memory::copy_to_struct(data, payload.data());
            listener_->on_client_connect(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_DISCONNECT: {
            if (payload.size() != sizeof(protocol::ClientDisconnectPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_DISCONNECT payload size\n";
                return;
            }
            protocol::ClientDisconnectPayload data;
            Memory::copy_to_struct(data, payload.data());
            listener_->on_client_disconnect(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_PING: {
            if (payload.size() != sizeof(protocol::ClientPingPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_PING payload size\n";
                return;
            }
            protocol::ClientPingPayload data;
            Memory::copy_to_struct(data, payload.data());
            listener_->on_client_ping(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_JOIN_LOBBY: {
            if (payload.size() != sizeof(protocol::ClientJoinLobbyPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_JOIN_LOBBY payload size\n";
                return;
            }
            protocol::ClientJoinLobbyPayload data;
            Memory::copy_to_struct(data, payload.data());
            listener_->on_client_join_lobby(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_LEAVE_LOBBY: {
            if (payload.size() != sizeof(protocol::ClientLeaveLobbyPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_LEAVE_LOBBY payload size\n";
                return;
            }
            protocol::ClientLeaveLobbyPayload data;
            Memory::copy_to_struct(data, payload.data());
            listener_->on_client_leave_lobby(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_CREATE_ROOM: {
            if (payload.size() != sizeof(protocol::ClientCreateRoomPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_CREATE_ROOM payload size\n";
                return;
            }
            protocol::ClientCreateRoomPayload data;
            Memory::copy_to_struct(data, payload.data());
            listener_->on_client_create_room(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_JOIN_ROOM: {
            if (payload.size() != sizeof(protocol::ClientJoinRoomPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_JOIN_ROOM payload size\n";
                return;
            }
            protocol::ClientJoinRoomPayload data;
            Memory::copy_to_struct(data, payload.data());
            listener_->on_client_join_room(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_LEAVE_ROOM: {
            if (payload.size() != sizeof(protocol::ClientLeaveRoomPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_LEAVE_ROOM payload size\n";
                return;
            }
            protocol::ClientLeaveRoomPayload data;
            Memory::copy_to_struct(data, payload.data());
            listener_->on_client_leave_room(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_REQUEST_ROOM_LIST: {
            // No payload for room list request
            listener_->on_client_request_room_list(client_id);
            break;
        }
        case protocol::PacketType::CLIENT_START_GAME: {
            if (payload.size() != sizeof(protocol::ClientStartGamePayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_START_GAME payload size\n";
                return;
            }
            protocol::ClientStartGamePayload data;
            Memory::copy_to_struct(data, payload.data());
            listener_->on_client_start_game(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_SET_PLAYER_NAME: {
            if (payload.size() != sizeof(protocol::ClientSetPlayerNamePayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_SET_PLAYER_NAME payload size\n";
                return;
            }
            protocol::ClientSetPlayerNamePayload data;
            Memory::copy_to_struct(data, payload.data());
            listener_->on_client_set_player_name(client_id, data);
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
    using rtype::server::netutils::Memory;
    using rtype::server::netutils::ByteOrder;

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
            Memory::copy_to_struct(data, payload.data());
            listener_->on_udp_handshake(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_INPUT: {
            if (payload.size() != sizeof(protocol::ClientInputPayload)) {
                std::cerr << "[NetworkHandler] Invalid CLIENT_INPUT payload size\n";
                return;
            }
            protocol::ClientInputPayload data;
            Memory::copy_to_struct(data, payload.data());

            // Convert from network byte order
            data.player_id = ByteOrder::net_to_host32(data.player_id);
            data.input_flags = ByteOrder::net_to_host16(data.input_flags);
            data.client_tick = ByteOrder::net_to_host32(data.client_tick);

            listener_->on_client_input(client_id, data);
            break;
        }
        case protocol::PacketType::CLIENT_PING: {
            if (payload.size() != sizeof(protocol::ClientPingPayload))
                return;
            protocol::ClientPingPayload data;
            Memory::copy_to_struct(data, payload.data());
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
