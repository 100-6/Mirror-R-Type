/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** NetworkSystem
*/

#ifndef NETWORKSYSTEM_HPP_
#define NETWORKSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "plugin_manager/INetworkPlugin.hpp"

/**
 * @brief System that handles network communication via INetworkPlugin
 *
 * This system manages network operations for both server and client modes.
 * It processes network packets and synchronizes entity state across the network.
 *
 * Uses hybrid TCP/UDP architecture:
 * - TCP: Connection, lobby, authentication
 * - UDP: Gameplay, inputs, state snapshots
 */
class NetworkSystem : public ISystem {
    private:
        engine::INetworkPlugin& network_plugin;  // Reference to the plugin (non-owned)
        bool is_server_mode;                     // True if running as server
        uint16_t tcp_port_;                      // TCP port (connection, lobby)
        uint16_t udp_port_;                      // UDP port (gameplay)

    public:
        /**
         * @brief Constructor with a network plugin
         * @param plugin Reference to the network plugin to use
         * @param server_mode True if system should operate in server mode
         * @param tcp_port TCP port for connections/lobby
         * @param udp_port UDP port for gameplay
         */
        explicit NetworkSystem(engine::INetworkPlugin& plugin, bool server_mode = false,
                               uint16_t tcp_port = 0, uint16_t udp_port = 0);
        virtual ~NetworkSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;

        /**
         * @brief Check if system is in server mode
         * @return True if server mode, false if client mode
         */
        bool is_server() const { return is_server_mode; }

        /**
         * @brief Get the network plugin
         * @return Reference to the network plugin
         */
        engine::INetworkPlugin& get_plugin() const { return network_plugin; }
};

#endif /* !NETWORKSYSTEM_HPP_ */
