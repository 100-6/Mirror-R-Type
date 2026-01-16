# R-Type

> A modern, modular game engine for building R-Type and other multiplayer games

[![License](https://img.shields.io/badge/license-EPITECH-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C++-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com)

## About

**R-Type** is a game engine built from scratch in C++20, designed to recreate the classic R-Type shoot 'em up game with modern architecture and extensibility. The engine features a plugin-based system, allowing developers to swap graphics, audio, and networking implementations seamlessly.

### Key Features

- **Plugin Architecture** - Modular design with hot-swappable plugins for graphics, audio, and networking
- **Multiplayer Ready** - Built-in client-server architecture with UDP networking via Boost.Asio
- **Multiple Renderers** - Support for Raylib graphics backend (extensible to SFML, SDL, etc.)
- **Audio System** - Integrated audio plugin based on MiniAudio
- **Event Bus** - Decoupled communication system for game events
- **ECS Architecture** - Entity Component System for efficient game object management
- **Cross-Platform** - Windows, Linux, and macOS support via vcpkg

## Documentation

### Getting Started
- **[Setup Guide](docs/SETUP.md)** - Get started with building and running the project
- **[Developer Guide](docs/DEVELOPER.md)** - Contributing guidelines and development workflow

### Architecture
- **[📚 Documentation Hub](docs/README.md)** - Point d'entrée de toute la documentation
- **[Server Architecture](docs/SERVER_ARCHITECTURE.md)** - Architecture détaillée du serveur
- **[Client Architecture](docs/CLIENT_ARCHITECTURE.md)** - Architecture détaillée du client

### Game Systems
- **[Wave System Guide](docs/WAVE_SYSTEM.md)** - JSON-based wave spawning system documentation
- **[Procedural Generation](docs/PROCEDURAL_GENERATION.md)** - Runtime procedural map generation system

## Quick Start

### Prerequisites

- CMake 3.20+
- C++20 compatible compiler (GCC 12+, Clang 15+, or MSVC 2022+)
- vcpkg (automatically setup by the project)

### Build

```bash
# Clone vcpkg (first time only)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg && ./bootstrap-vcpkg.sh && cd ..

# Build the project
cmake -B build
cmake --build build

# Run the game
./build/r_type_client
./build/r_type_server
```

For detailed build instructions, see **[Setup Guide](docs/SETUP.md)**.

## Project Structure

```
Mirror-R-Type/
├── engine/               # Core engine code
│   ├── include/         # Public engine headers
│   ├── src/             # Engine implementation
│   ├── plugin_manager/  # Plugin loading system
│   └── plugins/         # Official plugins
│       ├── graphics/    # Graphics backends (Raylib)
│       ├── audio/       # Audio backends (MiniAudio)
│       └── network/     # Network backends (Asio)
├── client/              # Game client application
├── server/              # Game server application
├── tests/               # Unit and integration tests
├── assets/              # Game assets (sprites, sounds, etc.)
└── docs/                # Documentation
```

## Plugin System

The engine uses a dynamic plugin architecture:

- **Graphics Plugins** - Rendering backends (Raylib, SFML, etc.)
- **Audio Plugins** - Sound systems (MiniAudio, OpenAL, etc.)
- **Network Plugins** - Networking implementations (Asio UDP/TCP, ENet, etc.)
- **Input Plugins** - Input handling systems

Create your own plugins by implementing the provided interfaces. See the [Plugin Creation Guide](engine/plugin_manager/HOW_TO_CREATE_PLUGIN.md).

## Gameplay

R-Type is a faithful recreation of the classic shoot 'em up:

- **Side-scrolling shooter** with procedurally generated levels
- **Multiplayer co-op** support up to 4 players
- **Power-ups and weapons** system
- **Enemy AI** with various patterns
- **Boss battles** with multiple phases

## Development

### Building from Source

See the detailed [Setup Guide](docs/SETUP.md) for platform-specific instructions.

### Contributing

We welcome contributions! Please read our [Developer Guide](docs/DEVELOPER.md) and [Contributing Guidelines](CONTRIBUTING.md) before submitting a PR.

### Code Quality

- **C++20** modern C++ practices
- **clang-tidy** linting (optional, see [Developer Guide](docs/DEVELOPER.md))
- **Unit tests** with Google Test
- **Documentation** with inline comments and separate guides

## Networking

The engine implements a robust client-server architecture:

- **UDP Protocol** for real-time gameplay
- **Client Prediction** for responsive controls
- **Server Reconciliation** for authoritative gameplay
- **Entity Interpolation** for smooth visuals

## Dependencies

All dependencies are managed via vcpkg:

- **Boost.Asio** - Async networking
- **Raylib** - Graphics rendering
- **MiniAudio** - Audio playback

For system dependencies and development tools, see [Setup Guide](docs/SETUP.md).

## License

This project is part of the EPITECH curriculum.

## Authors

EPITECH Project 2025 - R-Type

## Links

- [Original R-Type](https://en.wikipedia.org/wiki/R-Type)
- [Project Documentation](docs/)

---
