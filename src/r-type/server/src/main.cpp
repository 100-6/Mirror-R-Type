/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** R-Type Server Entry Point
*/

#include "Server.hpp"
#include "ServerConfig.hpp"
#include <iostream>
#include <csignal>
#include <memory>
#include <string>

static std::unique_ptr<rtype::server::Server> g_server;

/**
 * @brief Signal handler for graceful shutdown (SIGINT, SIGTERM)
 */
void signal_handler(int signal)
{
    std::cout << "\n[Server] Received signal " << signal << ", shutting down gracefully...\n";
    if (g_server)
        g_server->stop();
}

void print_help(const char* program_name)
{
    std::cout << "=== R-Type Server ===\n\n";
    std::cout << "USAGE:\n";
    std::cout << "  " << program_name << " [OPTIONS] [TCP_PORT] [UDP_PORT]\n\n";
    std::cout << "OPTIONS:\n";
    std::cout << "  -h, --help              Show this help message and exit\n";
    std::cout << "  -n, --network           Listen on all network interfaces (0.0.0.0)\n";
    std::cout << "                          By default, server listens on localhost only (127.0.0.1)\n\n";
    std::cout << "ARGUMENTS:\n";
    std::cout << "  TCP_PORT                TCP port for connections and lobby management\n";
    std::cout << "                          Default: " << rtype::server::config::DEFAULT_TCP_PORT << "\n\n";
    std::cout << "  UDP_PORT                UDP port for game state synchronization\n";
    std::cout << "                          Default: " << rtype::server::config::DEFAULT_UDP_PORT << "\n\n";
    std::cout << "EXAMPLES:\n";
    std::cout << "  " << program_name << "\n";
    std::cout << "      Start server on localhost with default ports\n";
    std::cout << "      (TCP:" << rtype::server::config::DEFAULT_TCP_PORT << ", UDP:" << rtype::server::config::DEFAULT_UDP_PORT << ")\n\n";
    std::cout << "  " << program_name << " -n\n";
    std::cout << "      Start server on all interfaces (0.0.0.0) with default ports\n\n";
    std::cout << "  " << program_name << " 4242 4243\n";
    std::cout << "      Start server on localhost with TCP:4242 and UDP:4243\n\n";
    std::cout << "  " << program_name << " 4242 4243 -n\n";
    std::cout << "      Start server on all interfaces with TCP:4242 and UDP:4243\n\n";
    std::cout << "ARCHITECTURE:\n";
    std::cout << "  The server uses a hybrid TCP/UDP architecture:\n";
    std::cout << "  - TCP: Reliable connection, lobby, chat, game start/end\n";
    std::cout << "  - UDP: Real-time game state (position, velocity, actions)\n\n";
    std::cout << "NOTES:\n";
    std::cout << "  - Use -n/--network flag to make server accessible from other machines\n";
    std::cout << "  - Press Ctrl+C to stop the server gracefully\n";
    std::cout << "  - Make sure firewall allows the specified ports\n\n";
}

/**
 * @brief Check if help flag is present and display help if needed
 * @return true if help was requested, false otherwise
 */
bool check_help_flag(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-h" || arg == "--help") {
            print_help(argv[0]);
            return true;
        }
    }
    return false;
}

/**
 * @brief Check if network flag is present in arguments
 */
bool parse_network_flag(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--network" || arg == "-n")
            return true;
    }
    return false;
}

/**
 * @brief Parse TCP port from command line arguments
 */
bool parse_tcp_port(int argc, char* argv[], int port_arg_idx, uint16_t& tcp_port)
{
    if (argc > port_arg_idx) {
        std::string port_str = argv[port_arg_idx];
        if (port_str != "--network" && port_str != "-n") {
            try {
                tcp_port = static_cast<uint16_t>(std::stoi(port_str));
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid TCP port number: " << port_str << "\n";
                std::cerr << "Use --help for usage information\n";
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief Parse UDP port from command line arguments
 */
bool parse_udp_port(int argc, char* argv[], int port_arg_idx, uint16_t& udp_port)
{
    if (argc > port_arg_idx + 1) {
        std::string port_str = argv[port_arg_idx + 1];
        if (port_str != "--network" && port_str != "-n") {
            try {
                udp_port = static_cast<uint16_t>(std::stoi(port_str));
            } catch (const std::exception& e) {
                std::cerr << "Error: Invalid UDP port number: " << port_str << "\n";
                std::cerr << "Use --help for usage information\n";
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief Setup signal handlers for graceful shutdown
 */
void setup_signal_handlers()
{
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

/**
 * @brief Print server startup information
 */
void print_server_info(bool listen_on_all_interfaces)
{
    std::cout << "=== R-Type Server ===\n";
    std::cout << "Protocol Version: 1.0\n";
    std::cout << "Transport: Hybrid TCP/UDP\n";
    std::cout << "Network: " << (listen_on_all_interfaces ? "All interfaces (0.0.0.0)" : "Localhost only (127.0.0.1)") << "\n\n";
}

/**
 * @brief Initialize and run the server
 */
int run_server(uint16_t tcp_port, uint16_t udp_port, bool listen_on_all_interfaces)
{
    g_server = std::make_unique<rtype::server::Server>(tcp_port, udp_port, listen_on_all_interfaces);

    if (!g_server->start()) {
        std::cerr << "[Server] Failed to start server\n";
        return 1;
    }
    g_server->run();
    g_server.reset();
    std::cout << "[Server] Shutdown complete\n";
    return 0;
}

int main(int argc, char* argv[])
{
    uint16_t tcp_port = rtype::server::config::DEFAULT_TCP_PORT;
    uint16_t udp_port = rtype::server::config::DEFAULT_UDP_PORT;
    bool listen_on_all_interfaces = false;
    int port_arg_idx = 1;

    if (check_help_flag(argc, argv))
        return 0;
    listen_on_all_interfaces = parse_network_flag(argc, argv);
    if (argc > 1) {
        std::string first_arg = argv[1];
        if (first_arg == "--network" || first_arg == "-n")
            port_arg_idx = 2;
    }
    if (!parse_tcp_port(argc, argv, port_arg_idx, tcp_port))
        return 1;
    if (!parse_udp_port(argc, argv, port_arg_idx, udp_port))
        return 1;
    setup_signal_handlers();
    print_server_info(listen_on_all_interfaces);
    return run_server(tcp_port, udp_port, listen_on_all_interfaces);
}
