# R-Type Documentation

Welcome to the R-Type project documentation!

## 📚 Available Documents

### Architecture

- **[SERVER_ARCHITECTURE.md](SERVER_ARCHITECTURE.md)** - Complete server architecture documentation
  - Component overview
  - Detailed network flows
  - TCP/UDP protocol
  - Lobby and session management
  - Sequence diagrams
  - Configuration and deployment

- **[CLIENT_ARCHITECTURE.md](CLIENT_ARCHITECTURE.md)** - Complete client architecture documentation
  - Component architecture
  - Network entity management
  - Server synchronization
  - Client-side prediction
  - ECS (Entity Component System)
  - User interface
  - Configuration and deployment

### Network Protocol

- **[UDP_ACK_SYSTEM.md](UDP_ACK_SYSTEM.md)** - UDP Acknowledgment System
  - Why ACK over UDP?
  - Sequence number-based ACK architecture
  - Client-side and server-side implementation
  - Data flow scenarios (packet loss, out-of-order)
  - Metrics and monitoring
  - Comparison with TCP, QUIC, and ENet

- **[PROTOCOL.md](PROTOCOL.md)** - Complete Network Protocol Specification
  - Packet structures and formats
  - All packet types (client-to-server and server-to-client)
  - LZ4 compression system
  - Connection flows and gameplay loops

### Game Systems

- **[WAVE_SYSTEM.md](WAVE_SYSTEM.md)** - Enemy wave system
  - Wave JSON configuration
  - Enemy spawning
  - Deployment patterns
  - Difficulty management

- **[PROCEDURAL_GENERATION.md](PROCEDURAL_GENERATION.md)** - Procedural map generation
  - System architecture
  - Generation algorithms
  - Client-server synchronization
  - Configuration and parameters
  - Performance and optimization

### Refactoring

- **[../REFACTORING.md](../REFACTORING.md)** - Complete refactoring documentation
  - Refactoring methodology
  - Before/After for server and client
  - Code metrics
  - Patterns used
  - Benefits and impact

## 🎯 Where to Start?

### To understand the project

1. Read [REFACTORING.md](../REFACTORING.md) first to understand the global vision
2. Then [SERVER_ARCHITECTURE.md](SERVER_ARCHITECTURE.md) for the server
3. Finally [CLIENT_ARCHITECTURE.md](CLIENT_ARCHITECTURE.md) for the client

### For development

#### Server Side
- Consult [SERVER_ARCHITECTURE.md](SERVER_ARCHITECTURE.md) section "Main Components"
- Review network flow diagrams
- Explore the TCP/UDP protocol

#### Client Side
- Consult [CLIENT_ARCHITECTURE.md](CLIENT_ARCHITECTURE.md) section "Main Components"
- Understand the ECS architecture
- Study network synchronization

#### Game Systems
- For enemy waves: [WAVE_SYSTEM.md](WAVE_SYSTEM.md)
- For map generation: [PROCEDURAL_GENERATION.md](PROCEDURAL_GENERATION.md)

## 🏗️ General Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    R-Type System                        │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────┐         Network          ┌─────────┐ │
│  │              │   TCP: 4242 (lobby)      │         │ │
│  │    Client    │◄═══════════════════════►│ Server  │ │
│  │              │   UDP: 4243 (game)       │         │ │
│  └──────────────┘                          └─────────┘ │
│                                                         │
│  • Graphics (Raylib)                 • NetworkHandler  │
│  • Input (Raylib)                    • PacketSender    │
│  • Audio (Miniaudio)                 • GameSession     │
│  • Network (Boost.Asio)              • LobbyManager    │
│  • ECS (Custom)                      • ECS (Custom)    │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

## 📁 Code Structure

```
Mirror-R-Type/
├── src/
│   ├── engine/           # Game engine (ECS, plugins)
│   ├── r-type/
│   │   ├── client/       # Client code
│   │   │   ├── include/
│   │   │   └── src/
│   │   ├── server/       # Server code
│   │   │   ├── include/
│   │   │   └── src/
│   │   ├── shared/       # Shared code (protocol)
│   │   └── game-logic/   # Shared game logic
│   └── ...
├── docs/                 # 📚 You are here!
│   ├── README.md
│   ├── SERVER_ARCHITECTURE.md
│   └── CLIENT_ARCHITECTURE.md
├── REFACTORING.md
└── README.md
```

## 🔧 Compilation

```bash
# Configuration
cmake -B build

# Full compilation
cmake --build build

# Server only
cmake --build build --target r-type_server

# Client only
cmake --build build --target r-type_client
```

## 🚀 Execution

### Server
```bash
./build/r-type_server [tcp_port] [udp_port]

# Examples:
./build/r-type_server                # Default ports (4242, 4243)
./build/r-type_server 5000 5001      # Custom ports
```

### Client
```bash
./build/r-type_client [host] [tcp_port] [player_name]

# Examples:
./build/r-type_client                           # Default: localhost:4242, "Pilot"
./build/r-type_client 192.168.1.100             # Remote server
./build/r-type_client 192.168.1.100 5000 "Bob"  # Everything custom
```

## 🎮 Controls

| Key | Action |
|--------|--------|
| **W** / ↑ | Up |
| **S** / ↓ | Down |
| **A** / ← | Left |
| **D** / → | Right |
| **Space** | Shoot |
| **Shift** | Charge |
| **Ctrl** | Special |
| **E** | Switch weapon |
| **Escape** | Quit |

## 🌐 Network Protocol

### TCP (Default port 4242)
- Connection/Disconnection
- Lobby management
- Control messages
- Important notifications

### UDP (Default port 4243)
- Player inputs (30 Hz)
- State snapshots (60 Hz)
- Entity spawn/destroy
- Projectiles

## 📊 Performance

| Metric | Value |
|----------|--------|
| **Server Tick Rate** | 60 TPS |
| **Client FPS** | 60 (VSync) |
| **Input Rate** | 30 Hz |
| **Snapshot Rate** | 60 Hz |
| **Max Players/Lobby** | 4 |

## 🧪 Tests

```bash
# Run tests
cmake --build build --target test
ctest --test-dir build
```

## 🐛 Debugging

### Server
Server logs display:
- Startup state
- Connections/Disconnections
- Lobbies (creation/deletion)
- Sessions (start/end)
- Network errors

### Client
Client logs display:
- Server connection
- Lobby state
- Game start
- Entity spawns
- Errors

## 🤝 Contributing

To contribute to the project:

1. **Read the documentation** - Understand the architecture
2. **Follow the patterns** - Respect separation of concerns
3. **Test** - Verify everything compiles and works
4. **Document** - Update documentation if necessary

## 📝 Code Conventions

- **Naming**: `snake_case` for variables/functions, `PascalCase` for classes
- **Headers**: Guards `#pragma once`
- **Comments**: Doxygen style for public APIs
- **Format**: 4-space indentation, no tabs

## 🔗 Useful Links

- [CMake Documentation](https://cmake.org/documentation/)
- [Raylib](https://www.raylib.com/)
- [Boost.Asio](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [ECS Architecture](https://github.com/SanderMertens/ecs-faq)

## 📧 Support

For any questions:
- Consult the complete documentation
- Check flow diagrams
- Examine code examples

---

**Last update**: 2025-12-16

**Version**: 1.0 (Post-refactoring)

**Status**: Production-ready ✅
