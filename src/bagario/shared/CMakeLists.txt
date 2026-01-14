add_library(bagario_protocol INTERFACE)

target_include_directories(bagario_protocol
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/protocol
)

add_library(bagario::protocol ALIAS bagario_protocol)
