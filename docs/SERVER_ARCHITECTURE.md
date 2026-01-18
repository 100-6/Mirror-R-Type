# R-Type Server Architecture

## Overview

The R-Type server is a hybrid TCP/UDP network application that manages client connections, game lobbies, and multiplayer game sessions.

## Global Architecture

```
┌─────────────────────────────────────────────────────────┐
│                       Server                            │
│  (Orchestration and player management)                  │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────┐│
│  │  Network    │  │  Packet      │  │  GameSession   ││
│  │  Handler    │  │  Sender      │  │  Manager       ││
│  └─────────────┘  └──────────────┘  └────────────────┘│
│         │                 │                  │         │
└─────────┼─────────────────┼──────────────────┼─────────┘
          │                 │                  │
          ▼                 ▼                  ▼
  ┌───────────────┐ ┌──────────────┐ ┌─────────────────┐
  │ INetwork      │ │ INetwork     │ │ GameSession     │
  │ Plugin        │ │ Plugin       │ │ (instances)     │
  └───────────────┘ └──────────────┘ └─────────────────┘
```

## Main Components

### 1. Server (Main Class)

**File**: `src/r-type/server/include/Server.hpp`

**Responsibilities**:
- Orchestration of all components
- Server lifecycle management (start/stop/run)
- Connected player list management
- Coordination between lobbies and game sessions

**Key Members**:
```cpp
class Server {
private:
    // Components
    std::unique_ptr<NetworkHandler> network_handler_;
    std::unique_ptr<PacketSender> packet_sender_;
    std::unique_ptr<GameSessionManager> session_manager_;

    // State
    std::unordered_map<uint32_t, PlayerInfo> connected_clients_;
    LobbyManager lobby_manager_;

    // Network
    engine::INetworkPlugin* network_plugin_;
};
```

**Main Methods**:
- `bool start()` - Initialize and start the server
- `void run()` - Main server loop (60 TPS)
- `void stop()` - Graceful server shutdown

### 2. NetworkHandler

**File**: `src/r-type/server/include/NetworkHandler.hpp`

**Responsibilities**:
- Network packet reception and decoding
- Protocol version validation
- Packet routing to appropriate handlers

**Architecture**:
```cpp
NetworkHandler::process_packets()
    ↓
receive() → decode_header() → validate_version()
    ↓
route_packet()
    ├─→ handle_tcp_packet() → callback (CLIENT_CONNECT, etc.)
    └─→ handle_udp_packet() → callback (CLIENT_INPUT, etc.)
```

**Available Callbacks**:
- `on_client_connect` - New client connection
- `on_client_disconnect` - Client disconnection
- `on_client_ping` - Ping request
- `on_client_join_lobby` - Join lobby request
- `on_client_leave_lobby` - Leave lobby request
- `on_udp_handshake` - UDP handshake
- `on_client_input` - Player input
- `on_client_chat_message` - Player chat message

### 3. PacketSender

**File**: `src/r-type/server/include/PacketSender.hpp`

**Responsibilities**:
- TCP packet sending (reliable, ordered)
- UDP packet sending (fast, unreliable)
- Broadcasting to lobbies and sessions

**Main Methods**:

**TCP**:
- `send_tcp_packet(client_id, type, payload)` - Unicast send
- `broadcast_tcp_packet(type, payload)` - Send to all clients
- `broadcast_tcp_to_lobby(lobby_id, type, payload)` - Send to lobby

**UDP**:
- `send_udp_packet(client_id, type, payload)` - Unicast send
- `broadcast_udp_to_session(session_id, type, payload)` - Send to session

**Packet Format**:
```
┌────────────┬────────┬─────────────┬──────────────┬─────────┐
│  Version   │  Type  │  Payload    │   Sequence   │ Payload │
│  (1 byte)  │(1 byte)│  Length     │   Number     │  Data   │
│            │        │  (2 bytes)  │  (4 bytes)   │         │
└────────────┴────────┴─────────────┴──────────────┴─────────┘
```

### 4. GameSessionManager

**File**: `src/r-type/server/include/GameSessionManager.hpp`

**Responsibilities**:
- Game session creation and destruction
- Update of all active sessions
- Automatic cleanup of inactive sessions
- Session callback configuration

**Session Lifecycle**:
```
create_session()
    ↓
setup_callbacks()
    ↓
update_all(delta_time)  [Called each tick]
    ↓
cleanup_inactive_sessions()
    ↓
remove_session() / on_game_over()
```

**Session Callbacks**:
- `on_state_snapshot` - Entity state snapshot
- `on_entity_spawn` - New entity spawn
- `on_entity_destroy` - Entity destruction
- `on_projectile_spawn` - Projectile spawn
- `on_wave_start` - Wave start
- `on_wave_complete` - Wave completion
- `on_game_over` - Game over

### 5. LobbyManager

**File**: `src/r-type/server/include/LobbyManager.hpp`

**Responsibilities**:
- Game lobby management (creation/deletion)
- Player matchmaking
- Pre-game countdown
- Lobby state notification

**Lobby States**:
```
WAITING (< min_players)
    ↓
READY (>= min_players)
    ↓
COUNTDOWN (10 seconds)
    ↓
STARTING
    ↓
[Game Session created]
```

## Network Flow

### Client Connection

```
Client                  Server
  │                       │
  ├──► CLIENT_CONNECT ───►│
  │                       ├─► Validate packet
  │                       ├─► Create PlayerInfo
  │                       ├─► Assign player_id
  │                       │
  │◄─── SERVER_ACCEPT ────┤
  │                       │
```

### Game Creation and Start

```
Client 1, 2, 3          Server                    GameSession
  │                       │                           │
  ├──► JOIN_LOBBY ───────►│                           │
  │                       ├─► lobby_manager.join()    │
  │◄─── LOBBY_STATE ──────┤                           │
  │                       │                           │
  │     [Min players reached]                         │
  │                       │                           │
  │◄─── COUNTDOWN (10) ───┤                           │
  │◄─── COUNTDOWN (9) ────┤                           │
  │        ...            │                           │
  │◄─── COUNTDOWN (1) ────┤                           │
  │                       │                           │
  │◄─── GAME_START ───────┤──► create_session() ────►│
  │                       │                           │
  ├──► UDP_HANDSHAKE ────►│──► associate_udp()       │
  │                       │                           │
  │◄═══ ENTITY_SPAWN ═════╬═══════════════════════════│
  │◄═══ WAVE_START ═══════╬═══════════════════════════│
  │                       │                           │
```

**Legend**: `───►` TCP, `═══►` UDP

### Chat Flow

```
Client 1                Server                    Client 2, 3, 4
  │                       │                           │
  ├──► CLIENT_CHAT_MSG ──►│                           │
  │     (message)         │                           │
  │                       ├─► lookup player_id        │
  │                       │   get player_name         │
  │                       │                           │
  │                       ├─► determine scope:        │
  │                       │   - in_lobby → broadcast  │
  │                       │     to lobby members      │
  │                       │   - in_game → broadcast   │
  │                       │     to session members    │
  │                       │                           │
  │◄── SERVER_CHAT_MSG ───┼───────────────────────────►
  │     (sender_name,     │                           │
  │      message)         │                           │
```

The server routes chat messages based on player state:
- **In lobby**: Broadcast to same lobby members (`in_lobby` && `lobby_id`)
- **In game**: Broadcast to same session members (`in_game` && `session_id`)

### Game Loop

```
Client                  Server                    GameSession
  │                       │                           │
  ├══► CLIENT_INPUT ═════►│──► handle_input() ───────►│
  │     (30 Hz)           │                           │
  │                       │                           │
  │                       │      update(dt)           │
  │                       │          ├─► MovementSystem
  │                       │          ├─► CollisionSystem
  │                       │          ├─► AISystem
  │                       │          └─► etc.
  │                       │                           │
  │◄═══ DELTA_SNAPSHOT ═══╬═══════════════════════════│
  │     (60 Hz)           │                           │
  │                       │                           │
```

## Network Protocol

### TCP/UDP Separation

**TCP** (Default port 4242):
- Connection/Disconnection
- Lobby management
- Control messages
- Important notifications

**UDP** (Default port 4243):
- Player inputs
- State snapshots
- Entity spawn/destroy
- Projectiles

### Packet Types

**TCP - Client → Server**:
- `CLIENT_CONNECT` - Connection request
- `CLIENT_DISCONNECT` - Voluntary disconnection
- `CLIENT_PING` - Latency measurement
- `CLIENT_JOIN_LOBBY` - Join lobby
- `CLIENT_LEAVE_LOBBY` - Leave lobby
- `CLIENT_CHAT_MESSAGE` - Chat message

**TCP - Server → Client**:
- `SERVER_ACCEPT` - Connection acceptance
- `SERVER_REJECT` - Connection rejection
- `SERVER_PONG` - Ping response
- `SERVER_LOBBY_STATE` - Lobby state
- `SERVER_GAME_START_COUNTDOWN` - Countdown
- `SERVER_GAME_START` - Game start
- `SERVER_CHAT_MESSAGE` - Chat message broadcast

**UDP - Client → Server**:
- `CLIENT_UDP_HANDSHAKE` - UDP/TCP association
- `CLIENT_INPUT` - Player input

**UDP - Server → Client**:
- `SERVER_DELTA_SNAPSHOT` - Entity state
- `SERVER_ENTITY_SPAWN` - Entity spawn
- `SERVER_ENTITY_DESTROY` - Entity destruction
- `SERVER_PROJECTILE_SPAWN` - Projectile spawn
- `SERVER_WAVE_START` - Wave start
- `SERVER_WAVE_COMPLETE` - Wave completion
- `SERVER_GAME_OVER` - Game over

## Configuration

**File**: `src/r-type/server/include/ServerConfig.hpp`

**Important Constants**:
```cpp
namespace rtype::server::config {
    constexpr uint16_t DEFAULT_TCP_PORT = 4242;
    constexpr uint16_t DEFAULT_UDP_PORT = 4243;
    constexpr uint32_t SERVER_TICK_RATE = 60;      // 60 TPS
    constexpr uint32_t TICK_INTERVAL_MS = 16;      // ~16ms
    constexpr uint32_t MAX_PLAYERS_PER_LOBBY = 4;
}
```

## Player Management

### PlayerInfo Structure

```cpp
struct PlayerInfo {
    uint32_t client_id;      // TCP connection ID
    uint32_t player_id;      // Unique player ID
    std::string player_name;
    uint32_t udp_client_id;  // UDP connection ID
    bool in_lobby;
    uint32_t lobby_id;
    bool in_game;
    uint32_t session_id;
};
```

### Player Lifecycle

```
1. TCP connection
   ↓
2. player_id assignment
   ↓
3. Acceptance (SERVER_ACCEPT)
   ↓
4. Join lobby
   ↓
5. UDP handshake
   ↓
6. Game start
   ↓
7. In game (receiving inputs, sending snapshots)
   ↓
8. Game over
   ↓
9. Return to lobby or disconnection
```

## Error Handling

### Unexpected Disconnection

The server detects disconnections via:
- TCP timeout (no keepalive)
- Network plugin `on_client_disconnected` callback

Action:
- Remove from lobby if applicable
- Notify other lobby players
- Resource cleanup

### Invalid Packets

- Protocol version validation
- Payload size validation
- Error logging without crash

### Orphaned Session

The `GameSessionManager` automatically cleans up inactive sessions each tick.

## Performance

### Tick Rate

- **Server**: 60 TPS (Ticks Per Second)
- **Client Input**: ~30 Hz
- **State Snapshot**: 60 Hz

### Optimizations

1. **Delta Snapshots**: Only changed entities are sent
2. **Entity Prioritization**: Snapshots prioritize critical entities:
   - **PLAYER** (maximum priority) - always included
   - **PROJECTILE** (high priority) - included for precise gameplay
   - **ENEMY** (medium priority) - included for synchronization
   - **WALL** excluded from snapshots (predictable scrolling client-side)
3. **Snapshot Size Limit**: Maximum 55 entities per snapshot (payload 1387 bytes / 25 bytes per EntityState)
4. **UDP for gameplay**: Latency reduction
5. **Entity Pooling**: Reuse of destroyed entities
6. **Spatial Partitioning**: For collisions (in GameSession)

## Deployment

### Compilation

```bash
cmake -B build
cmake --build build --target r-type_server
```

### Execution

```bash
# Default ports (4242 TCP, 4243 UDP)
./r-type_server

# Custom ports
./r-type_server <tcp_port> <udp_port>
```

### Logs

The server displays:
- Startup state
- Connections/Disconnections
- Lobby creation/destruction
- Game start/end
- Network errors

## Dependencies

- **game_engine**: ECS, systems, plugins
- **rtype_logic**: Game logic (collision, AI, etc.)
- **Boost.Asio**: Network plugin
- **Protocol**: Shared client/server headers

## Complete Sequence Diagram

```
┌──────┐  ┌────────┐  ┌──────────────┐  ┌───────────────┐  ┌─────────────┐
│Client│  │ Server │  │NetworkHandler│  │ LobbyManager  │  │GameSession  │
└──┬───┘  └───┬────┘  └──────┬───────┘  └───────┬───────┘  └──────┬──────┘
   │          │               │                  │                 │
   ├─CONNECT─►│               │                  │                 │
   │          ├─receive()────►│                  │                 │
   │          │               ├─decode()         │                 │
   │          │               ├─route()          │                 │
   │          │◄──callback────┤                  │                 │
   │          ├─create_player()                  │                 │
   │◄─ACCEPT──┤               │                  │                 │
   │          │               │                  │                 │
   ├JOIN_LOBBY│               │                  │                 │
   │          ├──────────────►│──callback───────►│                 │
   │          │               │                  ├─join()          │
   │◄LOBBY────┤◄──broadcast───┼──────────────────┤                 │
   │  STATE   │               │                  │                 │
   │          │               │           [countdown loop]         │
   │◄COUNTDOWN┤◄──broadcast───┼──────────────────┤                 │
   │          │               │                  │                 │
   │          │               │                  ├─on_game_start() │
   │          │               │                  │                 │
   │◄GAME_────┤               │                  ├──create────────►│
   │  START   │               │                  │                 │
   │          │               │                  │                 ├─setup()
   ├UDP_HAND─►│               │                  │                 │
   │  SHAKE   ├──────────────►│──callback────────┼─────────────────►
   │          │               │                  │                 ├associate
   │          │               │                  │                 │
   │          │               │                [game loop 60Hz]    │
   │          │               │                  │                 │
   ├═INPUT═══►│═══════════════╬══════════════════╬═════════════════►
   │          │               │                  │                 ├─update()
   │◄═SNAPSHOT│◄══════════════╬══════════════════╬═════════════════┤
   │          │               │                  │                 │
   │          │               │                [game over]         │
   │          │               │                  │                 │
   │◄GAME_────┤◄══════════════╬══════════════════╬═════════════════┤
   │  OVER    │               │                  │                 │
   │          │               │                  │                 ▼
```

## Conclusion

The server's modular architecture enables:
- **Maintainability**: Easy-to-understand and modify code
- **Extensibility**: Simplified addition of new features
- **Testability**: Each component can be tested independently
- **Performance**: High tick rate with network optimizations
- **Robustness**: Error handling and automatic cleanup
