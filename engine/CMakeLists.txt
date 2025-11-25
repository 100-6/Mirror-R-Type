# engine/CMakeLists.txt

add_library(rtype_engine
    src/Test.cpp
    src/core/event/EventBus.cpp
)

target_include_directories(rtype_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
