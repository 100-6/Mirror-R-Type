option(BUILD_NETWORK_PLUGIN_TESTS "Build test executables for network plugins" ON)

if(BUILD_NETWORK_PLUGIN_TESTS)
    add_executable(test_asio_network_plugin
        test_asio_network_plugin.cpp
    )

    target_include_directories(test_asio_network_plugin PRIVATE
        ${CMAKE_SOURCE_DIR}/engine/plugin_manager/include
        ${CMAKE_SOURCE_DIR}/engine/include
    )

    target_link_libraries(test_asio_network_plugin PRIVATE
        plugin_manager
        ${CMAKE_DL_LIBS}
    )

    set_target_properties(test_asio_network_plugin PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )

    add_dependencies(test_asio_network_plugin asio_network)

    add_test(NAME AsioNetworkPlugin COMMAND test_asio_network_plugin)
endif()
