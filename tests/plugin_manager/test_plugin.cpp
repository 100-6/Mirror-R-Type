/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Simple test plugin for testing the PluginManager
*/

#include "IPlugin.hpp"
#include <iostream>

namespace rtype {

/**
 * @brief A simple test plugin implementation
 */
class TestPlugin : public IPlugin {
private:
    bool initialized_ = false;

public:
    const char* get_name() const override {
        return "TestPlugin";
    }

    const char* get_version() const override {
        return "1.0.0";
    }

    bool initialize() override {
        std::cout << "TestPlugin::initialize() called" << std::endl;
        initialized_ = true;
        return true;
    }

    void shutdown() override {
        std::cout << "TestPlugin::shutdown() called" << std::endl;
        initialized_ = false;
    }

    bool is_initialized() const override {
        return initialized_;
    }

    // Custom test method
    void test_method() {
        std::cout << "TestPlugin::test_method() called" << std::endl;
    }
};

} // namespace rtype

// Export the plugin creation functions
extern "C" {
    rtype::IPlugin* create_plugin() {
        return new rtype::TestPlugin();
    }

    void destroy_plugin(rtype::IPlugin* plugin) {
        delete plugin;
    }
}
