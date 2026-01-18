# Protocol library with compression support

find_package(lz4 CONFIG REQUIRED)

add_library(rtype_protocol
    compression/PacketCompressor.cpp
    compression/CompressionStats.cpp
)

target_include_directories(rtype_protocol
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..  # Access to protocol/*.hpp files
)

target_link_libraries(rtype_protocol
    PUBLIC
        lz4::lz4
)

# Protocol headers are header-only except for compression, but we create a library for compression implementation
