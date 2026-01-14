/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** PluginPaths - Cross-platform plugin path helpers
*/

#pragma once

#include <string>

namespace engine {

/**
 * @brief Helper class for cross-platform plugin paths
 * 
 * This class provides utilities to construct plugin paths in a unified way
 * across Windows and Linux/Mac platforms.
 */
class PluginPaths {
public:
    /**
     * @brief Get the file extension for plugins on the current platform
     * @return ".dll" on Windows, ".so" on Linux/Mac
     */
    static constexpr const char* get_plugin_extension() {
#ifdef _WIN32
        return ".dll";
#else
        return ".so";
#endif
    }

    /**
     * @brief Construct a plugin path from a base name
     * @param plugin_name Base name of the plugin (e.g., "raylib_graphics")
     * @param plugin_dir Directory containing plugins (default: "plugins/")
     * @return Full path to the plugin with correct extension
     * 
     * @example
     * // Returns "plugins/raylib_graphics.dll" on Windows
     * // Returns "plugins/raylib_graphics.so" on Linux/Mac
     * auto path = PluginPaths::get_plugin_path("raylib_graphics");
     */
    static std::string get_plugin_path(const std::string& plugin_name, 
                                       const std::string& plugin_dir = "plugins/") {
        return plugin_dir + plugin_name + get_plugin_extension();
    }

    /**
     * @brief Common plugin names
     */
    static constexpr const char* RAYLIB_GRAPHICS = "raylib_graphics";
    static constexpr const char* RAYLIB_INPUT = "raylib_input";
    static constexpr const char* SFML_GRAPHICS = "sfml_graphics";
    static constexpr const char* SFML_INPUT = "sfml_input";
    static constexpr const char* MINIAUDIO_AUDIO = "miniaudio_audio";
    static constexpr const char* ASIO_NETWORK = "asio_network";
};

} // namespace engine
