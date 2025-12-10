# server/CMakeLists.txt

add_executable(r-type_server
    src/main.cpp
)

target_include_directories(r-type_server
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/engine/include
)

target_link_libraries(r-type_server
    PRIVATE
        rtype_engine
        rtype_game
)

# Ensure the server can find shared libraries at runtime
set_target_properties(r-type_server PROPERTIES
    INSTALL_RPATH "$ORIGIN"
    BUILD_WITH_INSTALL_RPATH TRUE
)
