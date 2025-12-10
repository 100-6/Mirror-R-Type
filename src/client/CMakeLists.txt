# client/CMakeLists.txt

add_executable(r-type_client
    src/main.cpp
)

target_include_directories(r-type_client
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
)

target_link_libraries(r-type_client
    PRIVATE
        rtype_engine
        rtype_game
)

# Ensure the client can find shared libraries at runtime
set_target_properties(r-type_client PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)
