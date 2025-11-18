# server/CMakeLists.txt

add_executable(r-type_server
    src/main.cpp
)

target_link_libraries(r-type_server
    PRIVATE
        rtype_engine
)

