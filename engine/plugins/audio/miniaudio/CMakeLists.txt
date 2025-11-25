cmake_minimum_required(VERSION 3.20)

# Miniaudio Audio Plugin
project(miniaudio_audio_plugin)

# Download miniaudio header if not present
set(MINIAUDIO_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/miniaudio.h")
if(NOT EXISTS ${MINIAUDIO_HEADER})
    message(STATUS "Downloading miniaudio.h...")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h"
        ${MINIAUDIO_HEADER}
        SHOW_PROGRESS
    )
endif()

# Create the plugin as a shared library
add_library(miniaudio_audio SHARED
    src/MiniaudioPlugin.cpp
)

# Include directories
target_include_directories(miniaudio_audio PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/engine/plugin_manager/include
)

# Set properties
set_target_properties(miniaudio_audio PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    PREFIX ""  # Remove 'lib' prefix on Linux
    OUTPUT_NAME "miniaudio_audio"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    # Linux-specific settings
    target_link_libraries(miniaudio_audio PRIVATE
        ${CMAKE_DL_LIBS}
        pthread
        m
    )
elseif(APPLE)
    # macOS-specific settings
    set_target_properties(miniaudio_audio PROPERTIES
        SUFFIX ".dylib"
    )
    target_link_libraries(miniaudio_audio PRIVATE
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework CoreFoundation"
    )
elseif(WIN32)
    # Windows-specific settings
    set_target_properties(miniaudio_audio PROPERTIES
        SUFFIX ".dll"
    )
    target_link_libraries(miniaudio_audio PRIVATE
        winmm
    )
endif()

# Installation rules
install(TARGETS miniaudio_audio
    LIBRARY DESTINATION plugins
    RUNTIME DESTINATION plugins
)
