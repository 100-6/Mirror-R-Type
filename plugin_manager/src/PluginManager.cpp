/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** PluginManager implementation
*/

#include "PluginManager.hpp"
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace rtype {

PluginManager::PluginManager() = default;

PluginManager::~PluginManager() {
    unload_all();
}

void* PluginManager::load_library(const std::string& path) {
#ifdef _WIN32
    return reinterpret_cast<void*>(LoadLibraryA(path.c_str()));
#else
    return dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
#endif
}

void PluginManager::unload_library(void* handle) {
    if (!handle) {
        return;
    }

#ifdef _WIN32
    FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

void* PluginManager::get_function(void* handle, const std::string& function_name) {
    if (!handle) {
        return nullptr;
    }

#ifdef _WIN32
    return reinterpret_cast<void*>(
        GetProcAddress(reinterpret_cast<HMODULE>(handle), function_name.c_str())
    );
#else
    return dlsym(handle, function_name.c_str());
#endif
}

std::string PluginManager::get_last_error() {
#ifdef _WIN32
    DWORD error = GetLastError();
    if (error == 0) {
        return "No error";
    }

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&messageBuffer),
        0,
        nullptr
    );

    std::string message(messageBuffer, size);
    LocalFree(messageBuffer);
    return message;
#else
    const char* error = dlerror();
    return error ? error : "No error";
#endif
}

void PluginManager::unload_plugin(const std::string& plugin_path) {
    auto it = loaded_plugins_.find(plugin_path);
    if (it == loaded_plugins_.end()) {
        throw PluginException("Plugin not loaded: " + plugin_path);
    }

    auto& handle = it->second;

    // Shutdown the plugin
    try {
        if (handle.plugin_instance && handle.plugin_instance->is_initialized()) {
            handle.plugin_instance->shutdown();
        }
    } catch (const std::exception& e) {
        std::cerr << "Warning: Exception during plugin shutdown: " << e.what() << std::endl;
    }

    // Delete the plugin instance using the deleter
    try {
        if (handle.deleter && handle.plugin_instance) {
            handle.deleter(handle.plugin_instance);
        }
    } catch (const std::exception& e) {
        std::cerr << "Warning: Exception during plugin destruction: " << e.what() << std::endl;
    }

    // Unload the library
    unload_library(handle.library_handle);

    // Remove from map
    loaded_plugins_.erase(it);
}

void PluginManager::unload_all() {
    // Create a copy of the keys to avoid iterator invalidation
    std::vector<std::string> plugin_paths;
    plugin_paths.reserve(loaded_plugins_.size());
    
    for (const auto& [path, _] : loaded_plugins_) {
        plugin_paths.push_back(path);
    }

    // Unload each plugin
    for (const auto& path : plugin_paths) {
        try {
            unload_plugin(path);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to unload plugin " << path 
                      << ": " << e.what() << std::endl;
        }
    }

    loaded_plugins_.clear();
}

bool PluginManager::is_plugin_loaded(const std::string& plugin_path) const {
    return loaded_plugins_.find(plugin_path) != loaded_plugins_.end();
}

size_t PluginManager::get_plugin_count() const {
    return loaded_plugins_.size();
}

} // namespace rtype
