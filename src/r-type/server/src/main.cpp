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
    std::cout << "                          By default, server listens on localhost only (127.0.0.1)\n";
    std::cout << "  --admin-password <pwd>  Enable admin interface with specified password\n\n";
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
    std::cout << "  " << program_name << " --admin-password secret123\n";
    std::cout << "      Start server with admin interface enabled (password: secret123)\n\n";
    std::cout << "  " << program_name << " 4242 4243\n";
    std::cout << "      Start server on localhost with TCP:4242 and UDP:4243\n\n";
    std::cout << "  " << program_name << " 4242 4243 -n\n";
    std::cout << "      Start server on all interfaces with TCP:4242 and UDP:4243\n\n";
    std::cout << "ARCHITECTURE:\n";
    std::cout << "  The server uses a hybrid TCP/UDP architecture:\n";
    std::cout << "  - TCP: Reliable connection, lobby, chat, game start/end\n";
    std::cout << "  - UDP: Real-time game state (position, velocity, actions)\n\n";
    std::cout << "ADMIN FEATURES:\n";
    std::cout << "  When admin is enabled (--admin-password), you can:\n";
    std::cout << "  - Use in-game console (~ key) after authentication\n";
    std::cout << "  - Execute commands: help, list, kick, info, shutdown\n\n";
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
 * @brief Parse admin password from command line arguments
 */
std::string parse_admin_password(int argc, char* argv[])
{
    for (int i = 1; i < argc - 1; ++i) {
        std::string arg = argv[i];
        if (arg == "--admin-password")
            return argv[i + 1];
    }
    return "";
}

/**
 * @brief Check if a string is a flag (starts with - or --)
 */
bool is_flag(const std::string& arg)
{
    return !arg.empty() && arg[0] == '-';
}

/**
 * @brief Parse TCP and UDP ports from command line arguments
 * Skips all flags and their values to find port numbers
 */
bool parse_ports(int argc, char* argv[], uint16_t& tcp_port, uint16_t& udp_port)
{
    int port_count = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--admin-password" || arg == "--config") {
            i++;
            continue;
        }
        if (is_flag(arg))
            continue;
        try {
            uint16_t port = static_cast<uint16_t>(std::stoi(arg));
            if (port_count == 0)
                tcp_port = port;
            else if (port_count == 1)
                udp_port = port;
            port_count++;
        } catch (const std::exception& e) {
            std::cerr << "Error: Invalid port number: " << arg << "\n";
            std::cerr << "Use --help for usage information\n";
            return false;
        }
    }
    return true;
}

/**
 * @brief Load ports from environment variables if set
 * Environment variables override default values but are overridden by command-line arguments
 */
void load_ports_from_env(uint16_t& tcp_port, uint16_t& udp_port)
{
    if (const char* env_tcp = std::getenv("RTYPE_SERVER_PORT_TCP")) {
        try {
            tcp_port = static_cast<uint16_t>(std::stoi(env_tcp));
            std::cout << "[Server] Using TCP port from environment: " << tcp_port << "\n";
        } catch (...) {
            std::cerr << "[Server] Warning: Invalid RTYPE_SERVER_PORT_TCP value, using default\n";
        }
    }
    if (const char* env_udp = std::getenv("RTYPE_SERVER_PORT_UDP")) {
        try {
            udp_port = static_cast<uint16_t>(std::stoi(env_udp));
            std::cout << "[Server] Using UDP port from environment: " << udp_port << "\n";
        } catch (...) {
            std::cerr << "[Server] Warning: Invalid RTYPE_SERVER_PORT_UDP value, using default\n";
        }
    }
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
int run_server(uint16_t tcp_port, uint16_t udp_port, bool listen_on_all_interfaces, const std::string& admin_password)
{
    g_server = std::make_unique<rtype::server::Server>(tcp_port, udp_port,
                                                        listen_on_all_interfaces,
                                                        admin_password);

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
    std::string admin_password;

    if (check_help_flag(argc, argv))
        return 0;
    load_ports_from_env(tcp_port, udp_port);
    listen_on_all_interfaces = parse_network_flag(argc, argv);
    admin_password = parse_admin_password(argc, argv);
    if (!parse_ports(argc, argv, tcp_port, udp_port))
        return 1;
    setup_signal_handlers();
    print_server_info(listen_on_all_interfaces);
    return run_server(tcp_port, udp_port, listen_on_all_interfaces, admin_password);
}
