# tests/audio_plugins/CMakeLists.txt

option(BUILD_AUDIO_PLUGIN_TESTS "Build test executables for audio plugins" ON)

if(BUILD_AUDIO_PLUGIN_TESTS)
    # Miniaudio plugin test
    add_executable(test_miniaudio_plugin
        test_miniaudio_plugin.cpp
    )

    target_include_directories(test_miniaudio_plugin PRIVATE
        ${CMAKE_SOURCE_DIR}/engine/plugin_manager/include
    )

    target_link_libraries(test_miniaudio_plugin PRIVATE
        plugin_manager
        ${CMAKE_DL_LIBS}
    )

    set_target_properties(test_miniaudio_plugin PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    # Make sure the plugin is built before the test
    add_dependencies(test_miniaudio_plugin miniaudio_audio)
endif()
