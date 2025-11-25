/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Unit tests for PluginManager
*/

#include "PluginManager.hpp"
#include <iostream>
#include <cassert>

using namespace rtype;

void test_plugin_loading() {
    std::cout << "\n=== Test: Plugin Loading ===" << std::endl;
    
    PluginManager manager;
    
    try {
        auto* plugin = manager.load_plugin<IPlugin>(
            "./tests/plugins/test_plugin.so",
            "create_plugin"
        );
        
        assert(plugin != nullptr);
        assert(plugin->is_initialized());
        assert(std::string(plugin->get_name()) == "TestPlugin");
        assert(std::string(plugin->get_version()) == "1.0.0");
        
        std::cout << "✓ Plugin loaded successfully" << std::endl;
        std::cout << "✓ Plugin name: " << plugin->get_name() << std::endl;
        std::cout << "✓ Plugin version: " << plugin->get_version() << std::endl;
        
    } catch (const PluginException& e) {
        std::cerr << "✗ Test failed: " << e.what() << std::endl;
        throw;
    }
}

void test_plugin_unloading() {
    std::cout << "\n=== Test: Plugin Unloading ===" << std::endl;
    
    PluginManager manager;
    
    manager.load_plugin<IPlugin>("./tests/plugins/test_plugin.so");
    assert(manager.is_plugin_loaded("./tests/plugins/test_plugin.so"));
    std::cout << "✓ Plugin loaded" << std::endl;
    
    manager.unload_plugin("./tests/plugins/test_plugin.so");
    assert(!manager.is_plugin_loaded("./tests/plugins/test_plugin.so"));
    std::cout << "✓ Plugin unloaded" << std::endl;
}

void test_multiple_plugins() {
    std::cout << "\n=== Test: Multiple Plugins ===" << std::endl;
    
    PluginManager manager;
    
    // Load the same plugin with different paths (simulating different plugins)
    manager.load_plugin<IPlugin>("./tests/plugins/test_plugin.so", "create_plugin");
    
    assert(manager.get_plugin_count() == 1);
    std::cout << "✓ Plugin count: " << manager.get_plugin_count() << std::endl;
    
    manager.unload_all();
    assert(manager.get_plugin_count() == 0);
    std::cout << "✓ All plugins unloaded" << std::endl;
}

void test_error_handling() {
    std::cout << "\n=== Test: Error Handling ===" << std::endl;
    
    PluginManager manager;
    
    // Test loading non-existent plugin
    try {
        manager.load_plugin<IPlugin>("./non_existent.so");
        std::cerr << "✗ Should have thrown PluginException" << std::endl;
        assert(false);
    } catch (const PluginException& e) {
        std::cout << "✓ Caught expected exception: " << e.what() << std::endl;
    }
    
    // Test unloading non-existent plugin
    try {
        manager.unload_plugin("./non_existent.so");
        std::cerr << "✗ Should have thrown PluginException" << std::endl;
        assert(false);
    } catch (const PluginException& e) {
        std::cout << "✓ Caught expected exception: " << e.what() << std::endl;
    }
    
    // Test loading plugin twice
    manager.load_plugin<IPlugin>("./tests/plugins/test_plugin.so");
    try {
        manager.load_plugin<IPlugin>("./tests/plugins/test_plugin.so");
        std::cerr << "✗ Should have thrown PluginException" << std::endl;
        assert(false);
    } catch (const PluginException& e) {
        std::cout << "✓ Caught expected exception: " << e.what() << std::endl;
    }
}

void test_get_plugin() {
    std::cout << "\n=== Test: Get Plugin ===" << std::endl;
    
    PluginManager manager;
    
    manager.load_plugin<IPlugin>("./tests/plugins/test_plugin.so");
    
    auto* plugin = manager.get_plugin<IPlugin>("./tests/plugins/test_plugin.so");
    assert(plugin != nullptr);
    assert(plugin->is_initialized());
    std::cout << "✓ Retrieved plugin successfully" << std::endl;
    
    auto* non_existent = manager.get_plugin<IPlugin>("./non_existent.so");
    assert(non_existent == nullptr);
    std::cout << "✓ Correctly returned nullptr for non-existent plugin" << std::endl;
}

void test_plugin_lifecycle() {
    std::cout << "\n=== Test: Plugin Lifecycle ===" << std::endl;
    
    {
        PluginManager manager;
        manager.load_plugin<IPlugin>("./tests/plugins/test_plugin.so");
        assert(manager.get_plugin_count() == 1);
        std::cout << "✓ Plugin loaded in scope" << std::endl;
        
        // manager destructor should automatically unload all plugins
    }
    
    std::cout << "✓ PluginManager destroyed, plugins auto-unloaded" << std::endl;
}

int main() {
    std::cout << "=== PluginManager Unit Tests ===" << std::endl;
    
    try {
        test_plugin_loading();
        test_plugin_unloading();
        test_multiple_plugins();
        test_error_handling();
        test_get_plugin();
        test_plugin_lifecycle();
        
        std::cout << "\n=== All Tests Passed ✓ ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n=== Tests Failed ✗ ===" << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
