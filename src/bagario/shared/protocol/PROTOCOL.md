# Bagario Network Protocol Specification

**Version:** 1.1
**Status:** Draft
**Last Updated:** 2026-01-14

## Table of Contents

1. [Introduction](#1-introduction)
2. [Terminology](#2-terminology)
3. [Transport Layer](#3-transport-layer)
4. [Packet Structure](#4-packet-structure)
5. [Client Packets](#5-client-packets)
6. [Server Packets](#6-server-packets)
7. [Connection Flow](#7-connection-flow)
8. [Game Loop](#8-game-loop)
9. [Error Handling](#9-error-handling)
10. [Security Considerations](#10-security-considerations)

---

## 1. Introduction

This document specifies the network protocol for Bagario, an agar.io-style multiplayer game. The protocol is designed to be efficient, low-latency, and suitable for real-time gameplay with multiple concurrent players.

### 1.1 Protocol Goals

- **Low Latency**: Minimize packet size and overhead for real-time gameplay
- **Reliability**: Critical packets use reliable channels
- **Simplicity**: Binary protocol with fixed-size structures
- **Extensibility**: Reserved packet type ranges for future features

### 1.2 Key Words

The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL NOT", "SHOULD", "SHOULD NOT", "RECOMMENDED", "MAY", and "OPTIONAL" in this document are to be interpreted as described in [RFC 2119](https://www.ietf.org/rfc/rfc2119.txt).

---

## 2. Terminology

- **Client**: The game client application running on a player's device
- **Server**: The authoritative game server managing game state
- **Entity**: Any game object (player cell, food, virus, ejected mass)
- **Player**: A connected user who may control multiple cells
- **Cell**: A single circular entity controlled by a player
- **Tick**: A single game update cycle (default: 60 Hz / 60 ticks per second)
- **Snapshot**: Complete or partial world state sent from server to client

---

## 3. Transport Layer

### 3.1 Network Plugin

The protocol uses **ENet** as the underlying transport layer through the engine's `INetworkPlugin` interface.

### 3.2 Channel Configuration

ENet provides two channels:

- **Channel 0 (Reliable)**: MUST be used for critical packets that require guaranteed delivery
- **Channel 1 (Unreliable)**: SHOULD be used for high-frequency, loss-tolerant packets

### 3.3 Default Ports

- **Primary Port (ENet)**: 5002 (configurable)
- **Secondary Port**: 5003 (reserved, currently unused)

Implementations MAY use different ports via command-line arguments.

### 3.4 Connection Parameters

- **Maximum Clients**: 32 (configurable via `MAX_PLAYERS`)
- **Timeout**: Connection timeout is managed by ENet's built-in mechanisms
- **Bandwidth**: No artificial bandwidth limits imposed by protocol

---

## 4. Packet Structure

### 4.1 Packet Format

All packets follow this structure:

```
+------------------+
| PacketType (1B)  |  uint8_t
+------------------+
| Payload (N bytes)|  Variable, depends on PacketType
+------------------+
```

### 4.2 Byte Order

All multi-byte integers and floating-point numbers MUST be transmitted in **little-endian** byte order.

### 4.3 Packet Type Ranges

Packet types are organized into ranges for clarity:

| Range       | Direction       | Purpose                  |
|-------------|-----------------|--------------------------|
| 0x01-0x04   | Client → Server | Connection Management    |
| 0x10-0x1F   | Client → Server | Player Input             |
| 0x81-0x8F   | Server → Client | Connection Response      |
| 0xA0-0xAF   | Server → Client | World State              |
| 0xB0-0xBF   | Server → Client | Entity Events            |
| 0xC0-0xCF   | Server → Client | Game Events              |

### 4.4 Struct Packing

All payload structures MUST be packed without padding to ensure consistent binary representation across platforms.

- **MSVC**: `#pragma pack(push, 1)` and `#pragma pack(pop)`
- **GCC/Clang**: `__attribute__((packed))`

---

## 5. Client Packets

Client packets are sent from the game client to the server.

### 5.1 CLIENT_CONNECT (0x01)

**Channel**: Reliable (0)
**Size**: 33 bytes
**Purpose**: Initial connection request from client to server

#### Payload Structure

```cpp
struct ClientConnectPayload {
    uint8_t  client_version;    // Protocol version
    char     player_name[32];   // Null-terminated UTF-8 string
};
```

#### Fields

- **client_version**: MUST match server's `PROTOCOL_VERSION` (currently 1)
- **player_name**: Player's display name
  - MUST be null-terminated
  - MUST NOT exceed 31 visible characters
  - SHOULD contain only printable characters
  - Empty names SHOULD be rejected by server

#### Behavior

1. Client MUST send this packet immediately after establishing connection
2. Client MUST wait for `SERVER_ACCEPT` or `SERVER_REJECT` response
3. Client MUST NOT send other packets until receiving acceptance

---

### 5.2 CLIENT_DISCONNECT (0x02)

**Channel**: Reliable (0)
**Size**: 5 bytes
**Purpose**: Graceful disconnection notification

#### Payload Structure

```cpp
struct ClientDisconnectPayload {
    uint32_t         player_id;  // Assigned player ID
    DisconnectReason reason;     // Reason code
};
```

#### Disconnect Reasons

- `USER_QUIT (0x01)`: User closed the game
- `TIMEOUT (0x02)`: Connection timeout
- `KICKED (0x03)`: Kicked by admin
- `SERVER_SHUTDOWN (0x04)`: Server shutting down

#### Behavior

1. Client SHOULD send this before closing connection
2. Server MAY force disconnect if timeout occurs
3. Client MUST NOT send additional packets after this

---

### 5.3 CLIENT_PING (0x04)

**Channel**: Unreliable (1)
**Size**: 8 bytes
**Purpose**: Measure round-trip time (RTT)

#### Payload Structure

```cpp
struct ClientPingPayload {
    uint32_t player_id;         // Player ID
    uint32_t client_timestamp;  // Client's timestamp (ms)
};
```

#### Behavior

1. Client MAY send pings periodically (recommended: every 1-5 seconds)
2. Server MUST respond with `SERVER_PONG` containing same timestamp
3. Client calculates RTT: `RTT = current_time - client_timestamp`

---

### 5.4 CLIENT_INPUT (0x10)

**Channel**: Unreliable (1)
**Size**: 16 bytes
**Purpose**: Send player's movement target (mouse position)

#### Payload Structure

```cpp
struct ClientInputPayload {
    uint32_t player_id;    // Player ID
    float    target_x;     // Target X position (world coordinates)
    float    target_y;     // Target Y position (world coordinates)
    uint32_t sequence;     // Sequence number (optional)
};
```

#### Fields

- **target_x, target_y**: World coordinates where player is aiming
  - MUST be within map bounds [0, MAP_WIDTH] x [0, MAP_HEIGHT]
  - Coordinates outside bounds MAY be clamped by server
- **sequence**: Client-side sequence number for input reconciliation
  - MAY be used for client-side prediction
  - Server does not currently use this field

#### Behavior

1. Client SHOULD send this packet every frame or on mouse movement
2. High frequency (e.g., 60 Hz) is acceptable due to unreliable channel
3. Server updates player's movement target based on latest received input

---

### 5.5 CLIENT_SPLIT (0x11)

**Channel**: Reliable (0)
**Size**: 4 bytes
**Purpose**: Request to split player's cells

#### Payload Structure

```cpp
struct ClientSplitPayload {
    uint32_t player_id;  // Player ID
};
```

#### Behavior

1. Client sends this when player presses split key (e.g., SPACE)
2. Server validates split conditions:
   - Player has cells with mass ≥ `MIN_SPLIT_MASS`
   - Player has fewer than `MAX_CELLS_PER_PLAYER` cells
3. Server splits valid cells toward player's current movement target

---

### 5.6 CLIENT_EJECT_MASS (0x12)

**Channel**: Reliable (0)
**Size**: 12 bytes
**Purpose**: Request to eject mass from player's cells

#### Payload Structure

```cpp
struct ClientEjectMassPayload {
    uint32_t player_id;      // Player ID
    float    direction_x;    // Ejection direction X (normalized)
    float    direction_y;    // Ejection direction Y (normalized)
};
```

#### Fields

- **direction_x, direction_y**: Normalized direction vector
  - SHOULD be normalized (length = 1.0)
  - Server MAY normalize if not already normalized

#### Behavior

1. Client sends this when player presses eject key (e.g., W)
2. Server validates eject conditions:
   - Player has cells with mass > `MIN_EJECT_MASS`
3. Server creates ejected mass entities in specified direction

---

### 5.7 CLIENT_SET_SKIN (0x13)

**Channel**: Reliable (0)
**Size**: 4 + 17 + N bytes (minimum 21 bytes, variable)
**Purpose**: Set player's custom skin appearance

#### Payload Structure

```cpp
struct ClientSetSkinPayload {
    uint32_t player_id;  // Player ID
    // Followed by skin data (see below)
};
```

#### Skin Data Format

| Field           | Type     | Size    | Description                        |
|-----------------|----------|---------|-------------------------------------|
| pattern         | uint8_t  | 1 byte  | Skin pattern type                   |
| primary_color   | uint32_t | 4 bytes | Primary color (RGBA)                |
| secondary_color | uint32_t | 4 bytes | Secondary color (RGBA)              |
| tertiary_color  | uint32_t | 4 bytes | Tertiary color (RGBA)               |
| image_data_size | uint32_t | 4 bytes | Size of image data (0 if no image)  |
| image_data      | bytes    | N bytes | Raw image data (only if size > 0)   |

#### Pattern Types

- `SOLID (0x00)`: Single color fill
- `STRIPED (0x01)`: Striped pattern using primary/secondary colors
- `SPOTTED (0x02)`: Spotted pattern
- `GRADIENT (0x03)`: Gradient between colors

#### Behavior

1. Client SHOULD send this immediately after receiving `SERVER_ACCEPT`
2. Server stores skin data for the player
3. Server broadcasts `SERVER_PLAYER_SKIN` to all connected clients
4. Client MAY send this again to update skin during gameplay

---

## 6. Server Packets

Server packets are sent from the game server to clients.

### 6.1 SERVER_ACCEPT (0x81)

**Channel**: Reliable (0)
**Size**: 18 bytes
**Purpose**: Accept client connection and provide game configuration

#### Payload Structure

```cpp
struct ServerAcceptPayload {
    uint32_t assigned_player_id;  // Assigned unique player ID
    float    map_width;            // Map width in world units
    float    map_height;           // Map height in world units
    float    starting_mass;        // Initial cell mass
    uint8_t  server_tick_rate;     // Server update rate (Hz)
    uint8_t  max_players;          // Maximum players allowed
};
```

#### Behavior

1. Server MUST send this in response to valid `CLIENT_CONNECT`
2. Client MUST use `assigned_player_id` for all subsequent packets
3. Client SHOULD store game configuration for validation and prediction

---

### 6.2 SERVER_REJECT (0x82)

**Channel**: Reliable (0)
**Size**: 65 bytes
**Purpose**: Reject client connection with reason

#### Payload Structure

```cpp
struct ServerRejectPayload {
    RejectReason reason_code;      // Reason code
    char         reason_message[64]; // Human-readable message
};
```

#### Reject Reasons

- `SERVER_FULL (0x01)`: Server has reached max players
- `VERSION_MISMATCH (0x02)`: Client version incompatible
- `INVALID_NAME (0x03)`: Player name invalid or already taken

#### Behavior

1. Server sends this instead of `SERVER_ACCEPT` when rejecting connection
2. Client MUST disconnect after receiving this
3. Client SHOULD display `reason_message` to user

---

### 6.3 SERVER_PONG (0x85)

**Channel**: Unreliable (1)
**Size**: 8 bytes
**Purpose**: Respond to client ping for RTT measurement

#### Payload Structure

```cpp
struct ServerPongPayload {
    uint32_t client_timestamp;  // Original timestamp from CLIENT_PING
    uint32_t server_timestamp;  // Server's timestamp (ms)
};
```

#### Behavior

1. Server MUST respond to each `CLIENT_PING` with this packet
2. Server SHOULD copy `client_timestamp` unchanged
3. Client calculates RTT using `client_timestamp`

---

### 6.4 SERVER_SNAPSHOT (0xA0)

**Channel**: Unreliable (1)
**Size**: 6 + (25 × entity_count) bytes
**Purpose**: Send current world state to client

#### Payload Structure

```cpp
struct ServerSnapshotPayload {
    uint32_t server_tick;    // Current server tick number
    uint16_t entity_count;   // Number of entities in snapshot
    // Followed by entity_count × EntityState structures
};

struct EntityState {
    uint32_t   entity_id;     // Unique entity ID
    EntityType entity_type;   // Type of entity
    float      position_x;    // X position
    float      position_y;    // Y position
    float      mass;          // Entity mass
    uint32_t   color;         // RGBA color (0xRRGGBBAA)
    uint32_t   owner_id;      // Owning player ID (0 if none)
};
```

#### Entity Types

- `PLAYER_CELL (0x01)`: Player-controlled cell
- `FOOD (0x02)`: Food pellet
- `VIRUS (0x03)`: Virus (splits cells)
- `EJECTED_MASS (0x04)`: Ejected mass blob

#### Behavior

1. Server SHOULD send snapshots at regular intervals (e.g., 20-60 Hz)
2. Server MAY send only entities visible to player (spatial culling)
3. Client uses this for rendering and interpolation
4. Packet loss is acceptable; client waits for next snapshot

---

### 6.5 SERVER_ENTITY_SPAWN (0xB0)

**Channel**: Reliable (0)
**Size**: 29 bytes
**Purpose**: Notify client of new entity creation

#### Payload Structure

```cpp
struct ServerEntitySpawnPayload {
    uint32_t   entity_id;      // Unique entity ID
    EntityType entity_type;    // Type of entity
    float      spawn_x;        // Spawn X position
    float      spawn_y;        // Spawn Y position
    float      mass;           // Initial mass
    uint32_t   color;          // RGBA color
    uint32_t   owner_id;       // Owner player ID (0 if none)
    char       owner_name[4];  // Owner name prefix (for display)
};
```

#### Behavior

1. Server sends this when entities are created (food spawn, cell split, etc.)
2. Client MUST add entity to local simulation
3. Client SHOULD play spawn effects/animations

---

### 6.6 SERVER_ENTITY_DESTROY (0xB1)

**Channel**: Reliable (0)
**Size**: 17 bytes
**Purpose**: Notify client of entity destruction

#### Payload Structure

```cpp
struct ServerEntityDestroyPayload {
    uint32_t      entity_id;   // Entity being destroyed
    DestroyReason reason;      // Why it was destroyed
    float         position_x;  // Last known X position
    float         position_y;  // Last known Y position
    uint32_t      killer_id;   // ID of entity that caused destruction
};
```

#### Destroy Reasons

- `EATEN (0x01)`: Eaten by another cell
- `MERGED (0x02)`: Merged with another cell
- `DECAYED (0x03)`: Natural decay
- `OUT_OF_BOUNDS (0x04)`: Went outside map bounds

#### Behavior

1. Server sends this when entities are removed
2. Client MUST remove entity from local simulation
3. Client SHOULD play destruction effects based on reason

---

### 6.7 SERVER_CELL_MERGE (0xB2)

**Channel**: Reliable (0)
**Purpose**: Notify client that two cells merged (reserved for future use)

**Note**: This packet type is defined but not currently implemented.

---

### 6.8 SERVER_PLAYER_EATEN (0xC0)

**Channel**: Reliable (0)
**Size**: 12 bytes
**Purpose**: Notify that a player was completely eliminated

#### Payload Structure

```cpp
struct ServerPlayerEatenPayload {
    uint32_t player_id;    // Player that was eliminated
    uint32_t killer_id;    // Player who ate them
    float    final_mass;   // Total mass before elimination
};
```

#### Behavior

1. Server sends this when player loses all cells
2. Client SHOULD display elimination notification
3. Affected player MAY respawn (implementation-specific)

---

### 6.9 SERVER_LEADERBOARD (0xC1)

**Channel**: Reliable (0)
**Size**: 1 + (40 × entry_count) bytes (max 401 bytes)
**Purpose**: Send current leaderboard rankings

#### Payload Structure

```cpp
struct ServerLeaderboardPayload {
    uint8_t entry_count;  // Number of entries (max 10)
    // Followed by entry_count × LeaderboardEntry structures
};

struct LeaderboardEntry {
    uint32_t player_id;      // Player ID
    char     player_name[32]; // Player name
    float    total_mass;     // Total mass of all player's cells
};
```

#### Behavior

1. Server SHOULD send leaderboard periodically (e.g., every 5 seconds)
2. Server MUST sort entries by `total_mass` descending
3. Server SHOULD include top 10 players only
4. Client displays leaderboard in UI

---

### 6.10 SERVER_PLAYER_SKIN (0xC2)

**Channel**: Reliable (0)
**Size**: 4 + 17 + N bytes (minimum 21 bytes, variable)
**Purpose**: Broadcast a player's skin to all clients

#### Payload Structure

```cpp
struct ServerPlayerSkinPayload {
    uint32_t player_id;  // Player whose skin is being broadcast
    // Followed by skin data (same format as CLIENT_SET_SKIN)
};
```

#### Skin Data Format

| Field           | Type     | Size    | Description                        |
|-----------------|----------|---------|-------------------------------------|
| pattern         | uint8_t  | 1 byte  | Skin pattern type                   |
| primary_color   | uint32_t | 4 bytes | Primary color (RGBA)                |
| secondary_color | uint32_t | 4 bytes | Secondary color (RGBA)              |
| tertiary_color  | uint32_t | 4 bytes | Tertiary color (RGBA)               |
| image_data_size | uint32_t | 4 bytes | Size of image data (0 if no image)  |
| image_data      | bytes    | N bytes | Raw image data (only if size > 0)   |

#### Behavior

1. Server sends this to ALL clients when a player sets their skin
2. Server SHOULD send existing player skins to newly connected clients
3. Client MUST store skin data for rendering player cells
4. Client uses this to display other players with their custom appearances

---

## 7. Connection Flow

### 7.1 Connection Establishment

```
Client                                Server
  |                                      |
  |-------- CLIENT_CONNECT (0x01) ----->|
  |                                      |
  |                                      | [Validate version, name, capacity]
  |                                      |
  |<------- SERVER_ACCEPT (0x81) -------|  [If accepted]
  |        OR                            |
  |<------- SERVER_REJECT (0x82) -------|  [If rejected]
  |                                      |
  |------- CLIENT_SET_SKIN (0x13) ----->|  [Send custom skin]
  |                                      |
  |<----- SERVER_PLAYER_SKIN (0xC2) ----|  [Broadcast to all clients]
  |                                      |
```

#### Steps

1. Client establishes ENet connection to server
2. Client sends `CLIENT_CONNECT` with version and name
3. Server validates:
   - Protocol version matches
   - Player name is valid and unique
   - Server has capacity
4. Server responds with `SERVER_ACCEPT` or `SERVER_REJECT`
5. If accepted, client enters game loop
6. If rejected, client disconnects

---

### 7.2 Graceful Disconnection

```
Client                                Server
  |                                      |
  |------- CLIENT_DISCONNECT (0x02) --->|
  |                                      |
  |                                      | [Remove player, broadcast events]
  |<-------- [Connection Closed] --------|
  |                                      |
```

---

## 8. Game Loop

### 8.1 Server Loop

Server runs at fixed tick rate (default: 60 Hz):

```
Every Tick (16.67ms @ 60 Hz):
1. Process incoming CLIENT_INPUT packets
2. Update game state:
   - Move cells toward targets
   - Check collisions (eating)
   - Apply physics (mass decay, bounds)
   - Spawn food as needed
3. Send SERVER_SNAPSHOT to all clients (may be throttled to 20 Hz)
4. Send reliable events (ENTITY_SPAWN, ENTITY_DESTROY, etc.)
```

### 8.2 Client Loop

Client runs at rendering framerate (e.g., 60 FPS):

```
Every Frame:
1. Handle user input (mouse, keyboard)
2. Send CLIENT_INPUT with mouse position
3. Receive and process server packets
4. Interpolate entities between snapshots
5. Render game world
```

### 8.3 Typical Packet Flow

```
Client                                Server
  |                                      |
  |--- CLIENT_INPUT (60 Hz unreliable)->|
  |                                      |
  |<-- SERVER_SNAPSHOT (20 Hz unreliable)|
  |                                      |
  |<-- SERVER_ENTITY_SPAWN (reliable) ---|  [When food spawns]
  |                                      |
  |<-- SERVER_ENTITY_DESTROY (reliable)--|  [When entity eaten]
  |                                      |
  |--- CLIENT_SPLIT (reliable) --------->|  [On split key press]
  |                                      |
```

---

## 9. Error Handling

### 9.1 Packet Validation

Servers MUST validate all incoming packets:

1. **Packet Type**: Must be valid and expected from client
2. **Payload Size**: Must match expected size for packet type
3. **Field Values**: Must be within valid ranges
4. **Player ID**: Must match authenticated player

Invalid packets SHOULD be logged and ignored.

### 9.2 Connection Timeout

- ENet handles connection timeout automatically
- Server SHOULD remove player if no packets received for timeout period
- Default timeout: 30 seconds (ENet default)

### 9.3 Malformed Packets

Clients and servers MUST handle:

- **Truncated packets**: Discard and continue
- **Unknown packet types**: Log and ignore
- **Out-of-order packets**: Accept (UDP nature)
- **Duplicate packets**: Accept (idempotent handling)

---

## 10. Security Considerations

### 10.1 Authentication

Current protocol uses player name only for identification. Future versions SHOULD implement:

- Token-based authentication
- Encrypted connections (TLS/DTLS)
- Rate limiting per client

### 10.2 Input Validation

Servers MUST validate:

- Position coordinates are within map bounds
- Mass values are positive and reasonable
- Player IDs match connected clients
- Packet rates don't exceed reasonable thresholds

### 10.3 Denial of Service

Servers SHOULD implement:

- **Connection rate limiting**: Limit new connections per IP
- **Packet rate limiting**: Limit packets per second per client
- **Bandwidth throttling**: Limit total bandwidth per client
- **Resource limits**: Maximum entities, players, etc.

### 10.4 Cheating Prevention

Server is authoritative and validates:

- Movement speed doesn't exceed calculated max
- Actions are physically possible (e.g., can't eat larger cells)
- Player can only control their own cells

Client predictions MUST be reconciled with server state.

---

## Appendix A: Packet Type Reference

| Type | Hex  | Name                     | Size (bytes) | Channel   | Direction |
|------|------|--------------------------|--------------|-----------|-----------|
| 0x01 | 0x01 | CLIENT_CONNECT           | 33           | Reliable  | C → S     |
| 0x02 | 0x02 | CLIENT_DISCONNECT        | 5            | Reliable  | C → S     |
| 0x04 | 0x04 | CLIENT_PING              | 8            | Unreliable| C → S     |
| 0x10 | 0x10 | CLIENT_INPUT             | 16           | Unreliable| C → S     |
| 0x11 | 0x11 | CLIENT_SPLIT             | 4            | Reliable  | C → S     |
| 0x12 | 0x12 | CLIENT_EJECT_MASS        | 12           | Reliable  | C → S     |
| 0x13 | 0x13 | CLIENT_SET_SKIN          | 4 + 17 + n   | Reliable  | C → S     |
| 0x81 | 0x81 | SERVER_ACCEPT            | 18           | Reliable  | S → C     |
| 0x82 | 0x82 | SERVER_REJECT            | 65           | Reliable  | S → C     |
| 0x85 | 0x85 | SERVER_PONG              | 8            | Unreliable| S → C     |
| 0xA0 | 0xA0 | SERVER_SNAPSHOT          | 6 + 25n      | Unreliable| S → C     |
| 0xB0 | 0xB0 | SERVER_ENTITY_SPAWN      | 29           | Reliable  | S → C     |
| 0xB1 | 0xB1 | SERVER_ENTITY_DESTROY    | 17           | Reliable  | S → C     |
| 0xB2 | 0xB2 | SERVER_CELL_MERGE        | TBD          | Reliable  | S → C     |
| 0xC0 | 0xC0 | SERVER_PLAYER_EATEN      | 12           | Reliable  | S → C     |
| 0xC1 | 0xC1 | SERVER_LEADERBOARD       | 1 + 40n      | Reliable  | S → C     |
| 0xC2 | 0xC2 | SERVER_PLAYER_SKIN       | 4 + 17 + n   | Reliable  | S → C     |

---

## Appendix B: Configuration Constants

| Constant              | Value  | Description                          |
|-----------------------|--------|--------------------------------------|
| PROTOCOL_VERSION      | 1      | Current protocol version             |
| DEFAULT_TCP_PORT      | 5002   | Default primary port                 |
| DEFAULT_UDP_PORT      | 5003   | Default secondary port (unused)      |
| MAX_PLAYERS           | 32     | Maximum concurrent players           |
| TICK_RATE             | 60     | Server update rate (Hz)              |
| MAP_WIDTH             | 5000.0 | Map width in world units             |
| MAP_HEIGHT            | 5000.0 | Map height in world units            |
| STARTING_MASS         | 10.0   | Initial player cell mass             |
| MIN_SPLIT_MASS        | 35.0   | Minimum mass to split                |
| MAX_CELLS_PER_PLAYER  | 16     | Maximum cells per player             |
| FOOD_MASS             | 1.0    | Mass value of food pellets           |
| MAX_FOOD              | 1000   | Maximum food pellets on map          |

---

## Appendix C: Changelog

**Version 1.1 (2026-01-14)**:
- Added CLIENT_SET_SKIN (0x13) for custom player skins
- Added SERVER_PLAYER_SKIN (0xC2) for broadcasting skins to clients
- Updated connection flow to include skin exchange
- Documented skin data format (pattern, colors, optional image)

**Version 1.0 (2026-01-14)**:
- Initial protocol specification
- Defined all packet types and payloads
- Documented connection flow and game loop
- Added security considerations

---

## References

- [RFC 2119: Key words for use in RFCs to Indicate Requirement Levels](https://www.ietf.org/rfc/rfc2119.txt)
- [ENet: Reliable UDP networking library](http://enet.bespin.org/)
- Source Code: `src/bagario/shared/protocol/`

---

**End of Specification**
