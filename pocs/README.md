# Proof of Concepts (POCs)

## Overview

This directory contains technical Proof of Concepts (POCs) conducted during the R-Type project development. These POCs were created to evaluate different technologies, architectures, and design patterns before making critical architectural decisions for the main project.

Each POC includes comprehensive documentation, benchmarks, and recommendations based on empirical testing.

## Purpose of POCs

POCs serve several critical purposes in the development process:

1. **Risk Mitigation**: Validate technical approaches before investing significant development time
2. **Performance Analysis**: Measure and compare performance characteristics of different solutions
3. **Architectural Decisions**: Provide data-driven insights for architecture choices
4. **Team Learning**: Share knowledge and best practices across the development team
5. **Documentation**: Create reference implementations for future development

## Available POCs

### 1. ECS vs OOP Performance Benchmark

**Location:** [`ecs-vs-oop/`](ecs-vs-oop/)

**Description:** Comprehensive performance comparison between Entity-Component-System (ECS) and traditional Object-Oriented Programming (OOP) paradigms for game entity management.

**Key Findings:**
- ECS provides 2-3x performance improvement over OOP
- Superior cache locality and memory layout in ECS
- Performance gap increases with entity count
- Recommended for R-Type's entity management system

**Technologies:** C++11, chrono for benchmarking

[Read Full Documentation →](ecs-vs-oop/README.md)

---

### 2. Static vs Dynamic Library Linking

**Location:** [`poc_libraries/`](poc_libraries/)

**Description:** Practical comparison of static linking (.a/.lib) versus dynamic linking (.so/.dll) in the context of game development, demonstrating both core engine libraries and plugin architectures.

**Key Findings:**
- Static libraries: Zero runtime overhead, simpler distribution
- Dynamic libraries: Extensibility, hot-reload capability
- Performance difference is negligible (< 1%) for typical game workloads
- Recommended hybrid approach: static for core, dynamic for plugins (optional)

**Technologies:** CMake, C++20, dlopen/dlsym, plugin architecture

[Read Full Documentation →](poc_libraries/README.md)

---

### 3. Graphics Library Comparison: SFML vs Raylib

**Location:** [`graphic/`](graphic/)

**Description:** Side-by-side comparison of two popular C++ graphics libraries for 2D game development, evaluating performance, features, and developer experience.

**Key Findings:**
- SFML: Mature, feature-rich, includes networking support
- Raylib: Lightweight, simple API, faster startup
- SFML recommended for R-Type due to built-in networking
- Both excellent choices depending on project requirements

**Technologies:** SFML 2.5+, Raylib 4.0+, C++17, Makefile

[Read Full Documentation →](graphic/README.md)

---

### 4. Event Bus vs Direct Communication

**Location:** [`EventBus/`](EventBus/)

**Description:** Comprehensive architectural comparison between direct method calls and publish-subscribe event bus patterns for inter-component communication in game engines.

**Key Findings:**
- Event Bus: Superior decoupling, better testability, excellent extensibility
- Direct Communication: Simpler, faster (but negligible difference)
- 422ns overhead per event is negligible (< 0.4% of frame budget)
- Event Bus strongly recommended for R-Type's message subsystem

**Technologies:** C++20, Google Test, Google Benchmark, CMake

[Read Full Documentation →](EventBus/README.md)

---

## Quick Start

Each POC directory contains:
- **README.md**: Comprehensive documentation with analysis and recommendations
- **Source code**: Working implementations for hands-on testing
- **Build instructions**: How to compile and run the POC
- **Benchmarks**: Performance measurements and comparisons

### General Build Pattern

Most POCs follow a similar build pattern:

```bash
# Navigate to POC directory
cd pocs/<poc-name>

# Create build directory (if using CMake)
mkdir build && cd build
cmake ..
cmake --build .

# Or use Make (if Makefile provided)
make

# Run the executable
./<executable-name>
```

Refer to each POC's individual README for specific instructions.

## POC Results Summary

| POC | Technology Evaluated | Recommended Choice | Impact Level |
|-----|---------------------|-------------------|--------------|
| **ECS vs OOP** | Entity management paradigms | ECS | High - Core architecture |
| **Static vs Dynamic** | Library linking strategies | Static (core) + Dynamic (plugins) | Medium - Build system |
| **SFML vs Raylib** | Graphics libraries | SFML | High - Graphics framework |
| **Event Bus vs Direct** | Communication patterns | Event Bus | High - Message subsystem |

## Recommendations for R-Type

Based on the POC findings, the recommended technical architecture for R-Type includes:

### Core Engine Architecture
- **Entity Management**: ECS (Entity-Component-System)
- **Graphics Library**: SFML (2D rendering + networking)
- **Communication**: Event Bus (Pub/Sub pattern)
- **Library Strategy**: Static linking for core, dynamic for plugins (optional)

### Technical Stack
```
R-Type Engine
├── ECS Framework (custom implementation)
│   ├── Entity Manager
│   ├── Component Systems
│   └── System Scheduler
├── Graphics (SFML)
│   ├── 2D Rendering
│   ├── Sprite Management
│   └── Window Management
├── Networking (SFML)
│   ├── UDP for game state
│   ├── TCP for reliable messages
│   └── Client-Server protocol
├── Event Bus (custom implementation)
│   ├── Event definitions
│   ├── Pub/Sub system
│   └── Type-safe callbacks
└── Core Libraries (static linking)
    ├── Physics engine
    ├── Collision detection
    ├── Math utilities
    └── Common types
```

### Performance Characteristics
- **Entity throughput**: 100,000+ entities at 60 FPS (ECS)
- **Event overhead**: < 1µs per event (Event Bus)
- **Frame budget**: 16.67ms at 60 FPS
- **Network latency**: Target < 50ms round-trip

## Directory Structure

```
pocs/
├── README.md                   # This file
├── ecs-vs-oop/                 # ECS vs OOP comparison
│   ├── README.md
│   └── benchmark.cpp
├── poc_libraries/              # Static vs Dynamic linking
│   ├── README.md
│   ├── CMakeLists.txt
│   ├── benchmark.cpp
│   ├── static_lib/
│   ├── dynamic_lib/
│   └── demo/
├── graphic/                    # SFML vs Raylib comparison
│   ├── README.md
│   ├── Makefile
│   ├── poc_sfml.cpp
│   └── poc_raylib.cpp
└── EventBus/                   # Event Bus vs Direct Communication
    ├── README.md
    ├── CMakeLists.txt
    ├── benchmarks/
    ├── include/
    ├── src/
    └── tests/
```

## Contributing to POCs

When creating new POCs, follow these guidelines:

### POC Structure
1. **Clear objective**: Define what is being tested and why
2. **Multiple implementations**: Compare at least two approaches
3. **Benchmarks**: Include quantitative performance measurements
4. **Documentation**: Comprehensive README with findings and recommendations
5. **Working code**: Runnable examples that demonstrate the concepts
6. **Build instructions**: Clear steps to compile and run

### Documentation Template
```markdown
# POC Title

## Overview
Brief description of what is being compared and why.

## What This POC Compares
Detailed explanation of each approach.

## Building and Running
Step-by-step build instructions.

## Understanding the Results
Explanation of benchmarks and measurements.

## Recommendations
Clear recommendation with rationale.

## Conclusion
Summary and next steps.
```

### Naming Conventions
- Directory names: lowercase with underscores (`ecs_vs_oop`, `poc_libraries`)
- README files: Always `README.md` (uppercase)
- Source files: Descriptive names (`benchmark.cpp`, `poc_sfml.cpp`)

## Best Practices from POCs

Key lessons learned from POC evaluations:

1. **Measure, Don't Assume**: Always benchmark before making performance-critical decisions
2. **Context Matters**: The "best" solution depends on project requirements
3. **Trade-offs Are Real**: No solution is perfect; understand the compromises
4. **Maintainability Counts**: Slight performance costs may be worth better architecture
5. **Start Simple**: Begin with simpler solutions; optimize only when necessary

## Additional Resources

### External References
- [Data-Oriented Design Book](https://www.dataorienteddesign.com/dodbook/)
- [SFML Official Documentation](https://www.sfml-dev.org/documentation/)
- [Raylib Official Website](https://www.raylib.com/)
- [Modern C++ Best Practices](https://isocpp.github.io/CppCoreGuidelines/)

### Related Project Documentation
- Main project README: [`../README.md`](../README.md)
- Architecture documentation: (link to architecture docs when available)
- Coding standards: (link to coding standards when available)

## Conclusion

These POCs provide a solid foundation for the technical decisions in the R-Type project. Each POC was carefully designed to answer specific questions about performance, architecture, and developer experience.

The recommendations are based on empirical data, not opinions or assumptions. By following these recommendations, the R-Type project can avoid common pitfalls and build on proven architectural patterns.

**Key Takeaway:** Invest time in POCs early in the project to make informed decisions that will save significant time and effort during development.

---

**R-Type Development Team**

**Project:** R-Type Multiplayer Game Engine
**Institution:** EPITECH
**Last Updated:** 2026-01-18
