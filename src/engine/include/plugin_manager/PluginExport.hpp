/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** PluginExport - Platform-specific export macros for plugins
*/

#pragma once

// Define PLUGIN_API for proper DLL export/import on Windows
#if defined(_WIN32) || defined(_WIN64)
    #ifdef PLUGIN_EXPORTS
        #define PLUGIN_API __declspec(dllexport)
    #else
        #define PLUGIN_API __declspec(dllimport)
    #endif
#else
    #define PLUGIN_API
#endif
