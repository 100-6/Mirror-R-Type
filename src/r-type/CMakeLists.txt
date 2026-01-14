# R-Type Game Project

# Add shared protocol library
add_subdirectory(shared)

# Add game-specific logic library
add_subdirectory(game-logic)

# Add client and server executables
if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

# Copy assets to build directory
add_custom_target(copy_rtype_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
    COMMENT "Copying R-Type assets to build directory"
)

# Ensure assets are copied before building executables
if(BUILD_CLIENT)
    add_dependencies(r-type_client copy_rtype_assets)
endif()

if(BUILD_SERVER)
    add_dependencies(r-type_server copy_rtype_assets)
endif()
