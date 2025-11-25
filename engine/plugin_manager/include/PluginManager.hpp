/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** PluginManager - Dynamic plugin loader
*/

#pragma once

#include "IPlugin.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <functional>

namespace rtype {

/**
 * @brief Exception thrown when plugin operations fail
 */
class PluginException : public std::runtime_error {
public:
    explicit PluginException(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Manages dynamic loading and unloading of plugins (.so/.dll files)
 * 
 * This class provides functionality to:
 * - Load plugins dynamically at runtime
 * - Manage plugin lifecycle (initialize, shutdown)
 * - Unload plugins when no longer needed
 * - Query loaded plugins
 * 
 * Usage example:
 * @code
 * PluginManager manager;
 * auto* graphics = manager.load_plugin<IGraphicsPlugin>(
 *     "./plugins/libsfml_graphics.so",
 *     "create_graphics_plugin"
 * );
 * // Use the plugin...
 * manager.unload_plugin("./plugins/libsfml_graphics.so");
 * @endcode
 */
class PluginManager {
public:
    /**
     * @brief Constructor
     */
    PluginManager();

    /**
     * @brief Destructor - automatically unloads all plugins
     */
    ~PluginManager();

    // Disable copy and move
    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;
    PluginManager(PluginManager&&) = delete;
    PluginManager& operator=(PluginManager&&) = delete;

    /**
     * @brief Load a plugin from a shared library file
     * 
     * @tparam PluginType The type of plugin interface to load (must inherit from IPlugin)
     * @param plugin_path Path to the .so/.dll file
     * @param create_function_name Name of the factory function (default: "create_plugin")
     * @return Pointer to the loaded plugin instance
     * @throws PluginException if loading fails
     */
    template<typename PluginType>
    PluginType* load_plugin(const std::string& plugin_path, 
                           const std::string& create_function_name = "create_plugin");

    /**
     * @brief Unload a previously loaded plugin
     * 
     * @param plugin_path Path to the plugin that was loaded
     * @throws PluginException if the plugin is not loaded
     */
    void unload_plugin(const std::string& plugin_path);

    /**
     * @brief Unload all loaded plugins
     */
    void unload_all();

    /**
     * @brief Check if a plugin is currently loaded
     * 
     * @param plugin_path Path to the plugin
     * @return true if the plugin is loaded, false otherwise
     */
    bool is_plugin_loaded(const std::string& plugin_path) const;

    /**
     * @brief Get a pointer to a loaded plugin
     * 
     * @tparam PluginType The type of plugin interface
     * @param plugin_path Path to the plugin
     * @return Pointer to the plugin, or nullptr if not loaded
     */
    template<typename PluginType>
    PluginType* get_plugin(const std::string& plugin_path);

    /**
     * @brief Get the number of currently loaded plugins
     * 
     * @return Number of loaded plugins
     */
    size_t get_plugin_count() const;

private:
    /**
     * @brief Internal structure to hold plugin information
     */
    struct PluginHandle {
        void* library_handle;           // dlopen/LoadLibrary handle
        IPlugin* plugin_instance;        // Pointer to the plugin object
        std::function<void(IPlugin*)> deleter; // Function to destroy the plugin
    };

    /**
     * @brief Map of loaded plugins (path -> handle)
     */
    std::unordered_map<std::string, PluginHandle> loaded_plugins_;

    /**
     * @brief Load a dynamic library
     * 
     * @param path Path to the library file
     * @return Library handle, or nullptr on failure
     */
    void* load_library(const std::string& path);

    /**
     * @brief Unload a dynamic library
     * 
     * @param handle Library handle
     */
    void unload_library(void* handle);

    /**
     * @brief Get a function pointer from a loaded library
     * 
     * @param handle Library handle
     * @param function_name Name of the function to retrieve
     * @return Function pointer, or nullptr on failure
     */
    void* get_function(void* handle, const std::string& function_name);

    /**
     * @brief Get the last error message from the system
     * 
     * @return Error message as a string
     */
    std::string get_last_error();
};

// Template implementation
template<typename PluginType>
PluginType* PluginManager::load_plugin(const std::string& plugin_path,
                                       const std::string& create_function_name) {
    // Check if already loaded
    if (is_plugin_loaded(plugin_path)) {
        throw PluginException("Plugin already loaded: " + plugin_path);
    }

    // Load the library
    void* library_handle = load_library(plugin_path);
    if (!library_handle) {
        throw PluginException("Failed to load library: " + plugin_path + 
                            " - " + get_last_error());
    }

    // Get the create function
    using CreateFunction = PluginType*(*)();
    auto create_fn = reinterpret_cast<CreateFunction>(
        get_function(library_handle, create_function_name)
    );

    if (!create_fn) {
        unload_library(library_handle);
        throw PluginException("Failed to find function '" + create_function_name + 
                            "' in " + plugin_path + " - " + get_last_error());
    }

    // Create the plugin instance
    PluginType* plugin = nullptr;
    try {
        plugin = create_fn();
        if (!plugin) {
            unload_library(library_handle);
            throw PluginException("Plugin creation function returned nullptr for " + 
                                plugin_path);
        }
    } catch (const std::exception& e) {
        unload_library(library_handle);
        throw PluginException("Exception during plugin creation: " + 
                            std::string(e.what()));
    }

    // Initialize the plugin
    try {
        if (!plugin->initialize()) {
            delete plugin;
            unload_library(library_handle);
            throw PluginException("Plugin initialization failed for " + plugin_path);
        }
    } catch (const std::exception& e) {
        delete plugin;
        unload_library(library_handle);
        throw PluginException("Exception during plugin initialization: " + 
                            std::string(e.what()));
    }

    // Get the destroy function (optional, will use delete if not found)
    std::function<void(IPlugin*)> deleter;
    using DestroyFunction = void(*)(PluginType*);
    auto destroy_fn = reinterpret_cast<DestroyFunction>(
        get_function(library_handle, "destroy_plugin")
    );

    if (destroy_fn) {
        deleter = [destroy_fn](IPlugin* p) {
            destroy_fn(static_cast<PluginType*>(p));
        };
    } else {
        deleter = [](IPlugin* p) {
            delete p;
        };
    }

    // Store the plugin handle
    loaded_plugins_[plugin_path] = {
        library_handle,
        plugin,
        deleter
    };

    return plugin;
}

template<typename PluginType>
PluginType* PluginManager::get_plugin(const std::string& plugin_path) {
    auto it = loaded_plugins_.find(plugin_path);
    if (it == loaded_plugins_.end()) {
        return nullptr;
    }
    return static_cast<PluginType*>(it->second.plugin_instance);
}

} // namespace rtype
