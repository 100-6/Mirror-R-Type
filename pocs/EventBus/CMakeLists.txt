cmake_minimum_required(VERSION 3.20)
project(EventBusPOC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler options for better diagnostics
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Google Test setup
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Google Benchmark setup
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Approach A - Direct Communication
add_executable(approach_a
    src/approach_a/main.cpp
    src/approach_a/PhysicsEngine.cpp
    src/approach_a/AudioEngine.cpp
    src/approach_a/ScoreManager.cpp
    src/approach_a/Renderer.cpp
)

# Approach B - Event Bus
add_executable(approach_b
    src/approach_b/main.cpp
    src/approach_b/EventBus.cpp
    src/approach_b/PhysicsEngine.cpp
    src/approach_b/AudioEngine.cpp
    src/approach_b/ScoreManager.cpp
    src/approach_b/Renderer.cpp
)

# Unit Tests
enable_testing()

add_executable(tests
    tests/test_approach_a.cpp
    tests/test_approach_b.cpp
    src/approach_a/PhysicsEngine.cpp
    src/approach_a/AudioEngine.cpp
    src/approach_a/ScoreManager.cpp
    src/approach_a/Renderer.cpp
    src/approach_b/EventBus.cpp
    src/approach_b/PhysicsEngine.cpp
    src/approach_b/AudioEngine.cpp
    src/approach_b/ScoreManager.cpp
    src/approach_b/Renderer.cpp
)

target_link_libraries(tests
    GTest::gtest_main
    GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(tests)

# Benchmarks
add_executable(benchmarks
    benchmarks/benchmark_comparison.cpp
    src/approach_a/PhysicsEngine.cpp
    src/approach_a/AudioEngine.cpp
    src/approach_a/ScoreManager.cpp
    src/approach_a/Renderer.cpp
    src/approach_b/EventBus.cpp
    src/approach_b/PhysicsEngine.cpp
    src/approach_b/AudioEngine.cpp
    src/approach_b/ScoreManager.cpp
    src/approach_b/Renderer.cpp
)

target_link_libraries(benchmarks
    benchmark::benchmark
)
