cmake_minimum_required(VERSION 3.20)
project(POC_Libraries LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

message(STATUS "=== Building POC: Static vs Dynamic Libraries ===")

# ============================================================================
# STATIC LIBRARY (.a) - Core Engine
# ============================================================================
message(STATUS "Building STATIC library: libcore_engine.a")

add_library(core_engine STATIC
    static_lib/CoreEngine.cpp
)

target_include_directories(core_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/static_lib
)

# Info sur la bibliothèque statique
get_target_property(CORE_ENGINE_TYPE core_engine TYPE)
message(STATUS "  → Type: ${CORE_ENGINE_TYPE}")
message(STATUS "  → Will be embedded in executable at link time")
message(STATUS "  → No runtime dependency")

# ============================================================================
# DYNAMIC LIBRARIES (.so) - Enemy Plugins
# ============================================================================
message(STATUS "Building DYNAMIC libraries: enemy plugins")

# Plugin 1: Basic Enemy
add_library(basic_enemy SHARED
    dynamic_lib/BasicEnemy.cpp
)

target_include_directories(basic_enemy
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/dynamic_lib
)

# Important: éviter le préfixe "lib" en double et gérer les symboles
set_target_properties(basic_enemy PROPERTIES
    PREFIX ""
    OUTPUT_NAME "libbasic_enemy"
)

# Plugin 2: Boss Enemy
add_library(boss_enemy SHARED
    dynamic_lib/BossEnemy.cpp
)

target_include_directories(boss_enemy
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/dynamic_lib
)

set_target_properties(boss_enemy PROPERTIES
    PREFIX ""
    OUTPUT_NAME "libboss_enemy"
)

# Info sur les bibliothèques dynamiques
get_target_property(BASIC_ENEMY_TYPE basic_enemy TYPE)
message(STATUS "  → Type: ${BASIC_ENEMY_TYPE}")
message(STATUS "  → Will be loaded at runtime with dlopen()")
message(STATUS "  → Can be updated without recompiling main program")

# ============================================================================
# DEMO EXECUTABLE
# ============================================================================
message(STATUS "Building demo executable")

add_executable(poc_demo
    demo/main.cpp
)

# Link static library (happens at compile time)
target_link_libraries(poc_demo
    PRIVATE
        core_engine  # Static library linked here
        ${CMAKE_DL_LIBS}  # Pour dlopen/dlsym
)

target_include_directories(poc_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================================
# BENCHMARK EXECUTABLE
# ============================================================================
message(STATUS "Building benchmark executable")

add_executable(benchmark
    benchmark.cpp
)

target_link_libraries(benchmark
    PRIVATE
        core_engine
        ${CMAKE_DL_LIBS}
)

target_include_directories(benchmark
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============================================================================
# COMPARAISON DES TAILLES
# ============================================================================
add_custom_command(TARGET poc_demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Build Complete ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "File sizes:"
    COMMAND ls -lh $<TARGET_FILE:poc_demo> 2>/dev/null || true
    COMMAND ls -lh $<TARGET_FILE:basic_enemy> 2>/dev/null || true
    COMMAND ls -lh $<TARGET_FILE:boss_enemy> 2>/dev/null || true
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "To run the demo:"
    COMMAND ${CMAKE_COMMAND} -E echo "  cd ${CMAKE_BINARY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "  ./poc_demo"
    COMMAND ${CMAKE_COMMAND} -E echo ""
)

# ============================================================================
# INSTALLATION (optionnel)
# ============================================================================
install(TARGETS poc_demo core_engine basic_enemy boss_enemy
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

message(STATUS "=== Configuration complete ===")
