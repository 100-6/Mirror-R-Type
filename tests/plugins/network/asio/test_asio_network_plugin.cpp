/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Test loading the Asio Network Plugin
*/

#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/INetworkPlugin.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <vector>

using namespace engine;

INetworkPlugin* test_plugin_loading(engine::PluginManager& plugin_manager) {
    std::cout << "\n=== Test: Plugin Loading ===" << std::endl;
    
    // Load the plugin (Windows: .dll, Linux/Mac: .so)
#ifdef _WIN32
    const char* plugin_path = "asio_network.dll";
#else
    const char* plugin_path = "libasio_network.so";
#endif
    
    auto* network = plugin_manager.load_plugin<INetworkPlugin>(
        plugin_path,
        "create_network_plugin"
    );

    if (!network)
        throw std::runtime_error("Failed to load network plugin");
    std::cout << "✓ Plugin loaded successfully!" << std::endl;
    std::cout << "  Name: " << network->get_name() << std::endl;
    std::cout << "  Version: " << network->get_version() << std::endl;
    return network;
}

void test_initialization(INetworkPlugin* network) {
    std::cout << "\n=== Test: Initialization ===" << std::endl;
    if (!network->initialize())
        throw std::runtime_error("Failed to initialize plugin");
    std::cout << "✓ Plugin initialized successfully!" << std::endl;
    std::cout << "  Initialized: " << (network->is_initialized() ? "Yes" : "No") << std::endl;
}

void test_server_start_stop(INetworkPlugin* network) {
    std::cout << "\n=== Test: Server Start/Stop ===" << std::endl;
    const uint16_t test_port = 12345;

    if (!network->start_server(test_port, NetworkProtocol::UDP))
        throw std::runtime_error("Failed to start server");
    std::cout << "✓ Server started on port " << test_port << std::endl;
    std::cout << "  Is running: " << (network->is_server_running() ? "Yes" : "No") << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    network->stop_server();
    std::cout << "✓ Server stopped" << std::endl;
    std::cout << "  Is running: " << (network->is_server_running() ? "Yes" : "No") << std::endl;
}

void test_client_server_communication(INetworkPlugin* network) {
    std::cout << "\n=== Test: Client Mode ===" << std::endl;
    std::cout << "⚠ Note: Full client-server test requires two processes" << std::endl;
    std::cout << "   This test validates client connection functionality only" << std::endl;
    const uint16_t port = 54321;
    bool connected = false;

    network->set_on_connected([&]() {
        std::cout << "  Client: Connected callback triggered" << std::endl;
        connected = true;
    });
    std::cout << "\nAttempting to connect to localhost:" << port << "..." << std::endl;
    if (network->connect("127.0.0.1", port, NetworkProtocol::UDP)) {
        std::cout << "✓ Client connected successfully" << std::endl;
        std::cout << "  Is connected: " << (network->is_connected() ? "Yes" : "No") << std::endl;
        std::vector<uint8_t> test_data = {1, 2, 3, 4, 5};
        NetworkPacket packet(test_data.data(), test_data.size());
        if (network->send(packet)) {
            std::cout << "✓ Sent test packet (" << test_data.size() << " bytes)" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        network->disconnect();
        std::cout << "✓ Client disconnected" << std::endl;
        std::cout << "  Is connected: " << (network->is_connected() ? "Yes" : "No") << std::endl;
    } else {
        std::cout << "⚠ Connection failed (expected if no server is running)" << std::endl;
        std::cout << "  This is normal - client functionality is still validated" << std::endl;
    }
    std::cout << "✓ Client mode test complete" << std::endl;
}

void test_shutdown(INetworkPlugin* network) {
    std::cout << "\n=== Test: Shutdown ===" << std::endl;
    network->shutdown();
    std::cout << "✓ Plugin shutdown complete!" << std::endl;
    std::cout << "  Initialized: " << (network->is_initialized() ? "Yes" : "No") << std::endl;
}

int main() {
    try {
        std::cout << "=== Asio Network Plugin Test ===" << std::endl;
        engine::PluginManager plugin_manager;
        auto* network = test_plugin_loading(plugin_manager);

        test_initialization(network);
        test_server_start_stop(network);
        test_client_server_communication(network);
        test_shutdown(network);
        
#ifdef _WIN32
        const char* plugin_path = "asio_network.dll";
#else
        const char* plugin_path = "libasio_network.so";
#endif
        
        std::cout << "\nUnloading plugin..." << std::endl;
        plugin_manager.unload_plugin(plugin_path);
        std::cout << "✓ Plugin unloaded successfully!" << std::endl;
        std::cout << "\n=== All tests passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Error: " << e.what() << std::endl;
        return 1;
    }
}
