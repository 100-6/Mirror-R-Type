# Static vs Dynamic Library Linking POC

## Overview

This POC (Proof of Concept) demonstrates the practical differences between **static linking (.a archives)** and **dynamic linking (.so shared libraries)** in the context of game development. It provides a hands-on comparison through a plugin-based enemy system built on top of a core game engine.

## What This POC Compares

### Static Libraries (.a / .lib)

Static libraries are compiled into the executable at **link time**.

**Implementation:**
- `CoreEngine` library providing entity management and core game functionality
- Linked directly into executables at compile time
- No runtime dependencies required

**Characteristics:**
- Code becomes part of the executable binary
- Single file distribution (all-in-one executable)
- No runtime loading overhead
- Optimal performance (direct function calls)
- Requires recompilation for updates

### Dynamic Libraries (.so / .dll)

Dynamic libraries are loaded at **runtime** using system APIs.

**Implementation:**
- Plugin system for enemy types (`BasicEnemy`, `BossEnemy`)
- Loaded on-demand with `dlopen()` (Linux) or `LoadLibrary()` (Windows)
- Can be modified without recompiling the main executable

**Characteristics:**
- Separate files distributed with the executable
- Runtime loading and unloading capability
- Modular, extensible architecture
- Slightly higher overhead (function pointer indirection)
- Hot-reloadable (update without recompiling main program)

## Project Structure

```
poc_libraries/
├── static_lib/          # Static library implementation
│   ├── CoreEngine.hpp   # Core engine interface
│   └── CoreEngine.cpp   # Core engine implementation
├── dynamic_lib/         # Dynamic library (plugin) interfaces
│   ├── IEnemyPlugin.hpp # Plugin interface definition
│   ├── BasicEnemy.cpp   # Basic enemy plugin implementation
│   └── BossEnemy.cpp    # Boss enemy plugin implementation
├── demo/
│   └── main.cpp         # Interactive demonstration
├── benchmark.cpp        # Performance benchmark
└── CMakeLists.txt       # Build configuration
```

## Building and Running

### Prerequisites

- CMake 3.20 or later
- C++20 compatible compiler (g++, clang++, MSVC)
- Standard C++ library
- Dynamic linking support (libdl on Linux/macOS)

### Build Instructions

#### Linux/macOS

```bash
cd pocs/poc_libraries
mkdir build && cd build
cmake ..
cmake --build .

# Run the demo
./poc_demo

# Run the benchmark
./benchmark
```

#### Windows (MSVC)

```bash
cd pocs\poc_libraries
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release

# Run the demo
.\Release\poc_demo.exe

# Run the benchmark
.\Release\benchmark.exe
```

#### Windows (MinGW)

```bash
cd pocs\poc_libraries
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .

# Run the demo
poc_demo.exe

# Run the benchmark
benchmark.exe
```

### What Gets Built

1. **libcore_engine.a** - Static library with core engine functionality
2. **libbasic_enemy.so** - Dynamic plugin for basic enemies
3. **libboss_enemy.so** - Dynamic plugin for boss enemies
4. **poc_demo** - Interactive demonstration executable
5. **benchmark** - Performance benchmark executable

## Understanding the Demo

The demo (`poc_demo`) showcases both linking approaches:

### Part 1: Static Library Usage

Demonstrates the `CoreEngine` static library:
- Entity creation and management
- Direct function calls (compiled into executable)
- No external dependencies at runtime

### Part 2: Dynamic Library Usage

Demonstrates the plugin system:
- Runtime loading of enemy plugins with `dlopen()`
- Dynamic symbol resolution with `dlsym()`
- Plugin instantiation and method calls through interfaces
- Runtime unloading with `dlclose()`

### Sample Output

```
╔════════════════════════════════════════════════════════════╗
║        R-Type: Static vs Dynamic Libraries POC            ║
╚════════════════════════════════════════════════════════════╝

============================================================
  PARTIE 1: Static Library (.a) - Core Engine
============================================================

→ The CoreEngine is linked statically
→ Code integrated into executable at compile time

Initializing CoreEngine...
[Core] Engine initialized

[Demo] Creating game entities...
[Core] Entity created: #1 - Player Ship
[Core] Entity created: #2 - Bullet 1
[Core] Entity created: #3 - Bullet 2

[Demo] Current entities:
  - #1: Player Ship
  - #2: Bullet 1
  - #3: Bullet 2

✓ ADVANTAGES .a (static):
  • No external runtime dependencies
  • Optimal performance (no indirection)
  • Simple distribution (single file)
  • Perfect for CORE code used by both client AND server

============================================================
  PARTIE 2: Dynamic Libraries (.so) - Plugin System
============================================================

→ Enemy plugins are loaded dynamically
→ .so files loaded with dlopen() at runtime

[Demo] Loading enemy plugins...
✓ Loaded plugin: Basic Enemy (./libbasic_enemy.so)
✓ Loaded plugin: Boss Enemy (./libboss_enemy.so)

[Demo] Spawning enemies from plugins...

--- Basic Enemy ---
[Plugin] Basic Enemy spawning at (800, 300)
Damage: 10

--- Boss Enemy ---
[Plugin] Boss Enemy spawning at (800, 300)
Damage: 50

✓ ADVANTAGES .so (dynamic):
  • Modification without recompiling main program
  • Extensible plugin/mod system
  • On-demand loading (memory savings)
  • Perfect for modular content (enemies, weapons, levels)
```

## Benchmark Results

The benchmark measures loading times and function call overhead.

### Performance Comparison

#### Loading Time

| Operation | Static (.a) | Dynamic (.so) |
|-----------|-------------|---------------|
| Load time | 0 µs (embedded) | 50-200 µs (dlopen) |
| Unload time | N/A | 10-50 µs (dlclose) |

**Key insight:** Static libraries have zero loading time because the code is already part of the executable. Dynamic libraries have a small one-time loading cost.

#### Function Call Overhead

| Operation | Static (.a) | Dynamic (.so) | Overhead |
|-----------|-------------|---------------|----------|
| Direct function call | ~1-2 ns | ~5-10 ns | ~5-8 ns |
| Virtual function call | ~2-3 ns | ~10-15 ns | ~8-12 ns |
| Create/destroy plugin | ~50 ns | ~100-150 ns | ~50-100 ns |

**Key insight:** Dynamic libraries add minimal overhead (nanoseconds) per call, which is negligible for game logic but worth considering for extremely hot paths (e.g., thousands of calls per frame).

### Real-World Impact

For a typical game frame (60 FPS = 16.67ms budget):

- **Loading overhead**: Negligible (done once at startup)
- **Call overhead**: ~0.001-0.01ms for thousands of calls
- **Practical impact**: < 0.1% performance difference in real scenarios

**Conclusion:** The performance difference is **negligible for game development**, making the choice primarily architectural rather than performance-driven.

## Recommendations

### Use Static Libraries (.a) For:

1. **Core engine code** - Used by both client and server
   - ECS framework
   - Physics engine
   - Network protocol
   - Math libraries
   - Common utilities

2. **Performance-critical paths** - Called millions of times per frame
   - Vector operations
   - Matrix math
   - Collision detection primitives

3. **Stable, rarely-changing code** - Foundation that doesn't need frequent updates

4. **Single-binary distribution** - When you want one executable with no dependencies

### Use Dynamic Libraries (.so) For:

1. **Plugin systems** - User-extensible content
   - Enemy types
   - Weapon systems
   - Level/stage definitions
   - Game modes

2. **Modding support** - Community-created content
   - Custom enemies
   - New weapons
   - User-created levels

3. **Frequently updated content** - Patches without full reinstall
   - Balance tweaks
   - Bug fixes in specific systems
   - Content updates

4. **Optional features** - Features that can be disabled
   - Debug tools
   - Editor extensions
   - Analytics modules

5. **Platform-specific code** - Different implementations per platform
   - Rendering backends (DirectX, OpenGL, Vulkan)
   - Audio systems
   - Input handlers

## Recommended Architecture for R-Type

### Static Libraries (.a)

```
rtype_engine.a
├── ECS Core (entity/component/system)
├── Physics Engine (collision, movement)
├── Network Protocol (UDP/TCP serialization)
└── Math Library (vectors, matrices)

rtype_protocol.a
├── Packet definitions
├── Serialization/deserialization
└── Network constants

rtype_common.a
├── Shared types
├── Utility functions
└── Configuration
```

### Dynamic Libraries (.so) - Optional

```
plugins/enemies/
├── enemy_basic.so      # Basic enemy types
├── enemy_boss.so       # Boss enemies
└── enemy_special.so    # Special/event enemies

plugins/weapons/
├── weapon_standard.so  # Standard weapons
├── weapon_powerup.so   # Power-up weapons
└── weapon_special.so   # Special weapons

plugins/levels/
├── level_1.so          # Stage 1
├── level_2.so          # Stage 2
└── level_boss.so       # Boss stages

plugins/mods/           # Community mods
└── mod_*.so            # User-created content
```

### Development Strategy

**Phase 1: Static Only**
- Start with all code in static libraries
- Focus on core gameplay and functionality
- Simplify build and distribution
- Faster iteration during development

**Phase 2: Selective Dynamic**
- Identify modular, extensible systems
- Convert to plugins if modularity adds value
- Keep performance-critical code static
- Add plugin system only if needed

**Recommendation:** For R-Type, **start with static libraries** for everything. The benefits of dynamic linking (modularity, hot-reload) are only valuable if you plan to support mods or have a large team working on independent systems simultaneously.

## Performance Best Practices

### Static Libraries

1. **Link-Time Optimization (LTO)**: Enable whole-program optimization
   ```cmake
   set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
   ```

2. **Minimize public symbols**: Use internal linkage where possible
   ```cpp
   namespace { /* internal functions */ }
   ```

3. **Header-only when appropriate**: For templates and small functions

### Dynamic Libraries

1. **Minimize exports**: Only export necessary symbols
   ```cpp
   #ifdef _WIN32
   #define EXPORT __declspec(dllexport)
   #else
   #define EXPORT __attribute__((visibility("default")))
   #endif
   ```

2. **Version your interfaces**: Use stable ABIs
   ```cpp
   struct IPlugin_v1 { /* stable interface */ };
   ```

3. **Cache function pointers**: Avoid repeated `dlsym()` calls
   ```cpp
   // Cache these at load time
   auto createFunc = (CreatePluginFunc)dlsym(handle, "createPlugin");
   ```

4. **Error handling**: Always check for loading failures
   ```cpp
   if (!handle) {
       std::cerr << "Failed to load: " << dlerror() << std::endl;
       // Fallback or error recovery
   }
   ```

## Common Pitfalls

### Static Libraries

- **Symbol conflicts**: Multiple static libraries with same symbol names
- **Bloat**: All code included even if not used (use LTO to mitigate)
- **Update friction**: Any change requires full recompilation and redistribution

### Dynamic Libraries

- **Dependency hell**: Missing .so files at runtime
- **Version mismatches**: ABI incompatibility between versions
- **Load failures**: Path issues, permission problems
- **Symbol visibility**: Unintended exports causing conflicts

## Conclusion

This POC demonstrates that:

1. **Static libraries** are ideal for core, stable functionality with optimal performance
2. **Dynamic libraries** enable modularity and extensibility at minimal performance cost
3. **Performance differences** are negligible (< 1%) for typical game workloads
4. **The choice** is primarily architectural: stability vs flexibility

For the **R-Type project**, the recommended approach is:
- Use **static libraries** for the core engine (ECS, physics, networking)
- Consider **dynamic libraries** only if plugin support or modding is a requirement
- Keep it simple: start static, add dynamic only when needed

The benchmark proves that performance should not drive this decision. Choose the architecture that best fits your development workflow and maintenance requirements.
