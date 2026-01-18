#include "BagarioServer.hpp"
#include "BagarioConfig.hpp"

#include <iostream>
#include <csignal>
#include <atomic>

std::atomic<bool> g_running{true};

void signal_handler(int signal) {
    std::cout << "\n[Main] Received signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [tcp_port] [udp_port] [--network]" << std::endl;
    std::cout << "  tcp_port: TCP port (default: " << bagario::config::DEFAULT_TCP_PORT << ")" << std::endl;
    std::cout << "  udp_port: UDP port (default: " << bagario::config::DEFAULT_UDP_PORT << ")" << std::endl;
    std::cout << "  --network: Listen on all network interfaces (0.0.0.0) instead of localhost only" << std::endl;
}

struct ServerConfig {
    uint16_t tcp_port;
    uint16_t udp_port;
    bool listen_on_all_interfaces;
};

bool parse_arguments(int argc, char* argv[], ServerConfig& config) {
    int port_count = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return false;
        } else if (arg == "--network") {
            config.listen_on_all_interfaces = true;
        } else {
            try {
                uint16_t port = static_cast<uint16_t>(std::stoi(arg));
                if (port_count == 0) {
                    config.tcp_port = port;
                } else if (port_count == 1) {
                    config.udp_port = port;
                } else {
                    std::cerr << "[Main] Too many port arguments" << std::endl;
                    return false;
                }
                port_count++;
            } catch (const std::exception& e) {
                std::cerr << "[Main] Invalid argument: " << arg << std::endl;
                return false;
            }
        }
    }
    return true;
}

void load_ports_from_env(ServerConfig& config) {
    if (const char* env_tcp = std::getenv("BAGARIO_SERVER_PORT_TCP")) {
        try {
            config.tcp_port = static_cast<uint16_t>(std::stoi(env_tcp));
            std::cout << "[Main] Using TCP port from environment: " << config.tcp_port << std::endl;
        } catch (...) {
            std::cerr << "[Main] Warning: Invalid BAGARIO_SERVER_PORT_TCP value, using default" << std::endl;
        }
    }
    if (const char* env_udp = std::getenv("BAGARIO_SERVER_PORT_UDP")) {
        try {
            config.udp_port = static_cast<uint16_t>(std::stoi(env_udp));
            std::cout << "[Main] Using UDP port from environment: " << config.udp_port << std::endl;
        } catch (...) {
            std::cerr << "[Main] Warning: Invalid BAGARIO_SERVER_PORT_UDP value, using default" << std::endl;
        }
    }
}

void setup_signal_handlers() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
}

void print_server_info(const ServerConfig& config) {
    std::cout << "========================================" << std::endl;
    std::cout << "         Bagario Game Server            " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "TCP Port: " << config.tcp_port << std::endl;
    std::cout << "UDP Port: " << config.udp_port << std::endl;
    std::cout << "Map Size: " << bagario::config::MAP_WIDTH << "x"
              << bagario::config::MAP_HEIGHT << std::endl;
    std::cout << "Max Players: " << static_cast<int>(bagario::config::MAX_PLAYERS) << std::endl;
    std::cout << "Network Mode: " << (config.listen_on_all_interfaces ? "All interfaces (0.0.0.0)" : "Localhost only (127.0.0.1)") << std::endl;
    std::cout << "========================================" << std::endl;
}

bool initialize_server(bagario::server::BagarioServer& server) {
    if (!server.start()) {
        std::cerr << "[Main] Failed to start server" << std::endl;
        return false;
    }
    std::cout << "[Main] Server is running. Press Ctrl+C to stop." << std::endl;
    return true;
}

void run_server_loop(bagario::server::BagarioServer& server) {
    while (g_running && server.is_running())
        server.run();
}

void shutdown_server(bagario::server::BagarioServer& server) {
    server.stop();
    std::cout << "[Main] Server shutdown complete" << std::endl;
}

int main(int argc, char* argv[]) {
    ServerConfig config;
    config.tcp_port = bagario::config::DEFAULT_TCP_PORT;
    config.udp_port = bagario::config::DEFAULT_UDP_PORT;
    config.listen_on_all_interfaces = false;

    load_ports_from_env(config);
    if (!parse_arguments(argc, argv, config))
        return argc >= 2 ? 1 : 0;
    setup_signal_handlers();
    print_server_info(config);
    bagario::server::BagarioServer server(config.tcp_port, config.udp_port, config.listen_on_all_interfaces);
    if (!initialize_server(server))
        return 1;
    run_server_loop(server);
    shutdown_server(server);
    return 0;
}
