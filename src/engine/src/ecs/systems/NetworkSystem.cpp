/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** NetworkSystem
*/

#include "ecs/systems/NetworkSystem.hpp"
#include "ecs/Components.hpp"
#include <iostream>

NetworkSystem::NetworkSystem(engine::INetworkPlugin& plugin, bool server_mode, uint16_t port)
    : network_plugin(plugin), is_server_mode(server_mode), server_port(port)
{
    // Pas besoin de vérifier null - les références ne peuvent pas être nulles
}

void NetworkSystem::init(Registry& registry)
{
    (void)registry;
    std::cout << "NetworkSystem: Initialisation in "
              << (is_server_mode ? "SERVER" : "CLIENT")
              << " mode with " << network_plugin.get_name() << std::endl;

    // Start server automatically in server mode
    if (is_server_mode) {
        if (network_plugin.start_server(server_port)) {
            std::cout << "NetworkSystem: Server started on port " << server_port << std::endl;
        } else {
            throw std::runtime_error("NetworkSystem: Failed to start server on port " + std::to_string(server_port));
        }
    }
}

void NetworkSystem::shutdown()
{
    std::cout << "NetworkSystem: Shutdown" << std::endl;

    if (is_server_mode && network_plugin.is_server_running()) {
        network_plugin.stop_server();
    } else if (!is_server_mode && network_plugin.is_connected()) {
        network_plugin.disconnect();
    }
}

void NetworkSystem::update(Registry& registry, float dt)
{
    (void)registry;

    // Update the network plugin (poll for events)
    network_plugin.update(dt);

    // Receive and process packets
    auto packets = network_plugin.receive();
    
    for (const auto& packet : packets) {
        // Process received packets
        // This is where you would deserialize packet data and update entities
        // For now, just log that we received a packet
        if (!packet.data.empty()) {
            if (is_server_mode) {
                std::cout << "NetworkSystem [SERVER]: Received packet from client " 
                          << packet.sender_id << " (" << packet.data.size() << " bytes)" << std::endl;
            } else {
                std::cout << "NetworkSystem [CLIENT]: Received packet from server (" 
                          << packet.data.size() << " bytes)" << std::endl;
            }
        }
    }
    
    // TODO: Add entity synchronization logic here
    // - Serialize entity state (Position, Velocity, etc.)
    // - Send updates to clients (server mode) or server (client mode)
    // - Handle interpolation/prediction for smooth gameplay
}
