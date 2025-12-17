/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** IPlugin - Base interface for all plugins
*/

#pragma once

#include <string>

namespace engine {

/**
 * @brief Base interface that all plugins must implement
 * 
 * This interface defines the basic contract for any plugin in the system.
 * All specific plugin types (Graphics, Network, Audio, etc.) should inherit from this.
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;

    /**
     * @brief Get the name of the plugin
     * @return Plugin name as a C-string
     */
    virtual const char* get_name() const = 0;

    /**
     * @brief Get the version of the plugin
     * @return Plugin version as a C-string (e.g., "1.0.0")
     */
    virtual const char* get_version() const = 0;

    /**
     * @brief Initialize the plugin
     * @return true if initialization succeeded, false otherwise
     */
    virtual bool initialize() = 0;

    /**
     * @brief Shutdown and cleanup the plugin
     */
    virtual void shutdown() = 0;

    /**
     * @brief Check if the plugin is currently initialized
     * @return true if initialized, false otherwise
     */
    virtual bool is_initialized() const = 0;
};

}
