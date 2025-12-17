# R-Type Game Protocol Specification

**Status**: Draft
**Version**: 1.0
**Date**: December 2025
**Authors**: R-Type Development Team

---

## 1. Introduction

### 1.1 Purpose

This document specifies the R-Type Game Protocol (RTGP), a UDP-based binary protocol for client-server communication in the R-Type multiplayer game. The protocol enables real-time gameplay synchronization, lobby management, and matchmaking for cooperative multiplayer sessions.

### 1.2 Scope

This protocol defines packet formats, message types, and communication patterns for:
- Client authentication and connection management
- Lobby system with game mode and difficulty selection
- Matchmaking and game session initialization
- Player input transmission
- Game state synchronization
- Entity management (players, enemies, projectiles, power-ups)
- Game events and scoring

### 1.3 Requirements Language

The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL NOT", "SHOULD", "SHOULD NOT", "RECOMMENDED", "MAY", and "OPTIONAL" in this document are to be interpreted as described in RFC 2119.

---

## 2. Protocol Overview

### 2.1 Transport Layer

RTGP operates over UDP (User Datagram Protocol). The default server port is 4242 (configurable).

**Rationale**: UDP is chosen over TCP for:
- Lower latency requirements for real-time gameplay
- Tolerance for occasional packet loss (state is regularly synchronized)
- Reduced overhead for high-frequency input packets

### 2.2 Packet Structure

All packets consist of a fixed-size header followed by a variable-length payload:

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Version    |     Type      |         Payload Length        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Sequence Number                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Payload Data                          |
   |                             ...                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### 2.3 Size Constraints

- **Maximum packet size**: 1400 bytes (MTU-safe for typical network configurations)
- **Header size**: 8 bytes (fixed)
- **Maximum payload size**: 1392 bytes
- Packets exceeding 1400 bytes MUST be fragmented or redesigned

---

## 3. Packet Header

### 3.1 Header Format

All packets begin with an 8-byte header in network byte order (big-endian).

| Field            | Size    | Offset | Description                              |
|------------------|---------|--------|------------------------------------------|
| Version          | 1 byte  | 0      | Protocol version (0x01)                  |
| Type             | 1 byte  | 1      | Packet type identifier                   |
| Payload Length   | 2 bytes | 2      | Length of payload in bytes (big-endian)  |
| Sequence Number  | 4 bytes | 4      | Monotonic sequence number (big-endian)   |

### 3.2 Field Definitions

**Version (8 bits)**
- MUST be set to `0x01` for this version of the protocol
- Receivers MUST reject packets with unknown versions
- Future versions MAY use different values for backward compatibility detection

**Type (8 bits)**
- Identifies the packet type (see Section 4)
- Values `0x00-0x7F`: Reserved for client-to-server packets
- Values `0x80-0xFF`: Reserved for server-to-client packets

**Payload Length (16 bits, big-endian)**
- Length of the payload in bytes
- MUST NOT exceed 1392 bytes
- Value of 0 indicates no payload (header-only packet)

**Sequence Number (32 bits, big-endian)**
- Monotonically increasing counter per connection
- Used for packet ordering, loss detection, and replay prevention
- Wraps around at 2^32
- Clients and servers maintain separate sequence counters

---

## 4. Packet Types

### 4.1 Type ID Allocation

Packet types are organized by functional category:

| Range       | Category                  | Description                           |
|-------------|---------------------------|---------------------------------------|
| 0x01-0x04   | Connection Management     | Connect, disconnect, keepalive        |
| 0x05-0x09   | Lobby & Matchmaking       | Join, leave, lobby state updates      |
| 0x10-0x1F   | Player Input              | Real-time input commands              |
| 0x20-0x3F   | Reserved                  | Future client-to-server use           |
| 0x81-0x8A   | Connection & Lobby (S→C)  | Accept, reject, lobby updates         |
| 0xA0-0xAF   | World State               | Game snapshots, delta updates         |
| 0xB0-0xBF   | Entity Events             | Spawn, destroy, damage                |
| 0xC0-0xCF   | Game Mechanics            | Powerups, scoring, waves, respawn     |
| 0xF0-0xFF   | System & Chat             | Broadcast messages, system events     |

### 4.2 Client-to-Server Packets

| Type ID | Name                  | Description                              |
|---------|-----------------------|------------------------------------------|
| 0x01    | CLIENT_CONNECT        | Initial connection request               |
| 0x02    | CLIENT_DISCONNECT     | Graceful disconnection                   |
| 0x04    | CLIENT_PING           | Keepalive and latency measurement        |
| 0x05    | CLIENT_JOIN_LOBBY     | Join lobby with mode/difficulty          |
| 0x06    | CLIENT_LEAVE_LOBBY    | Leave current lobby                      |
| 0x10    | CLIENT_INPUT          | Player input state                       |

### 4.3 Server-to-Client Packets

| Type ID | Name                         | Description                           |
|---------|------------------------------|---------------------------------------|
| 0x81    | SERVER_ACCEPT                | Connection accepted                   |
| 0x82    | SERVER_REJECT                | Connection rejected                   |
| 0x83    | SERVER_PLAYER_JOINED         | Another player joined the server      |
| 0x84    | SERVER_PLAYER_LEFT           | A player disconnected                 |
| 0x85    | SERVER_PONG                  | Ping response                         |
| 0x87    | SERVER_LOBBY_STATE           | Current lobby state update            |
| 0x88    | SERVER_GAME_START_COUNTDOWN  | Game starting countdown               |
| 0x89    | SERVER_COUNTDOWN_CANCELLED   | Countdown cancelled (player left)     |
| 0x8A    | SERVER_GAME_START            | Game session begins                   |
| 0xA0    | SERVER_SNAPSHOT              | Complete world state snapshot         |
| 0xA1    | SERVER_DELTA_SNAPSHOT        | Delta-compressed state update         |
| 0xB0    | SERVER_ENTITY_SPAWN          | New entity spawned                    |
| 0xB1    | SERVER_ENTITY_DESTROY        | Entity destroyed                      |
| 0xB2    | SERVER_ENTITY_DAMAGE         | Entity took damage                    |
| 0xB3    | SERVER_PROJECTILE_SPAWN      | Projectile fired                      |
| 0xC0    | SERVER_POWERUP_COLLECTED     | Power-up collected by player          |
| 0xC1    | SERVER_SCORE_UPDATE          | Player score updated                  |
| 0xC2    | SERVER_WAVE_START            | New enemy wave starting               |
| 0xC3    | SERVER_WAVE_COMPLETE         | Wave completed                        |
| 0xC5    | SERVER_PLAYER_RESPAWN        | Player respawned                      |
| 0xC6    | SERVER_GAME_OVER             | Game session ended                    |

---

## 5. Payload Formats

All multi-byte integer fields are in network byte order (big-endian) unless otherwise specified. Floating-point values use IEEE 754 single precision (32-bit) format.

### 5.1 Connection Management Payloads

#### 5.1.1 CLIENT_CONNECT (0x01)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Client Ver   |                                               |
   +-+-+-+-+-+-+-+-+                                               +
   |                        Player Name (32 bytes)                 |
   +                                                               +
   |                             ...                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field         | Size     | Description                                    |
|---------------|----------|------------------------------------------------|
| Client Ver    | 1 byte   | Client version identifier (0x01)               |
| Player Name   | 32 bytes | UTF-8 player name (null-terminated/padded)     |

**Total Size**: 33 bytes

#### 5.1.2 CLIENT_DISCONNECT (0x02)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Player ID                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Reason     |
   +-+-+-+-+-+-+-+-+
```

| Field      | Size    | Description                              |
|------------|---------|------------------------------------------|
| Player ID  | 4 bytes | Player identifier                        |
| Reason     | 1 byte  | Disconnect reason code                   |

**Reason Codes**:
- `0x01`: USER_QUIT - Player voluntarily quit
- `0x02`: TIMEOUT - Connection timeout
- `0x03`: ERROR - Client error

**Total Size**: 5 bytes

#### 5.1.3 CLIENT_PING (0x04)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Player ID                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Client Timestamp                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field             | Size    | Description                         |
|-------------------|---------|-------------------------------------|
| Player ID         | 4 bytes | Player identifier                   |
| Client Timestamp  | 4 bytes | Client timestamp (milliseconds)     |

**Total Size**: 8 bytes

#### 5.1.4 SERVER_ACCEPT (0x81)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Assigned Player ID                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Server Tick R |  Max Players  |           Map ID              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field              | Size    | Description                            |
|--------------------|---------|----------------------------------------|
| Assigned Player ID | 4 bytes | Unique player ID assigned by server    |
| Server Tick Rate   | 1 byte  | Server tick rate in Hz (typically 60)  |
| Max Players        | 1 byte  | Maximum players per game session       |
| Map ID             | 2 bytes | Default map identifier                 |

**Total Size**: 8 bytes

#### 5.1.5 SERVER_REJECT (0x82)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Reason Code   |                                               |
   +-+-+-+-+-+-+-+-+                                               +
   |                    Reason Message (64 bytes)                  |
   +                                                               +
   |                             ...                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field           | Size     | Description                              |
|-----------------|----------|------------------------------------------|
| Reason Code     | 1 byte   | Rejection reason code                    |
| Reason Message  | 64 bytes | UTF-8 human-readable message             |

**Reason Codes**:
- `0x01`: SERVER_FULL - Server at maximum capacity
- `0x02`: VERSION_MISMATCH - Client version incompatible
- `0x03`: BANNED - Player is banned
- `0x04`: MAINTENANCE - Server in maintenance mode

**Total Size**: 65 bytes

#### 5.1.6 SERVER_PONG (0x85)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Client Timestamp                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Server Timestamp                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field             | Size    | Description                                |
|-------------------|---------|--------------------------------------------|
| Client Timestamp  | 4 bytes | Echo of client timestamp from CLIENT_PING  |
| Server Timestamp  | 4 bytes | Server timestamp (milliseconds)            |

**Total Size**: 8 bytes

### 5.2 Lobby & Matchmaking Payloads

#### 5.2.1 CLIENT_JOIN_LOBBY (0x05)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Player ID                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   Game Mode   |  Difficulty   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field       | Size    | Description                              |
|-------------|---------|------------------------------------------|
| Player ID   | 4 bytes | Player identifier                        |
| Game Mode   | 1 byte  | Requested game mode                      |
| Difficulty  | 1 byte  | Requested difficulty level               |

**Game Mode Values**:
- `0x01`: DUO - 2 players
- `0x02`: TRIO - 3 players
- `0x03`: SQUAD - 4 players

**Difficulty Values**:
- `0x01`: EASY
- `0x02`: NORMAL
- `0x03`: HARD

**Total Size**: 6 bytes

#### 5.2.2 CLIENT_LEAVE_LOBBY (0x06)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Player ID                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Lobby ID                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field      | Size    | Description                 |
|------------|---------|-----------------------------|
| Player ID  | 4 bytes | Player identifier           |
| Lobby ID   | 4 bytes | Lobby to leave              |

**Total Size**: 8 bytes

#### 5.2.3 SERVER_LOBBY_STATE (0x87)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Lobby ID                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   Game Mode   |  Difficulty   | Current Count | Required Count|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Player Entry (38 bytes)                  |
   |                             ...                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field                 | Size     | Description                          |
|-----------------------|----------|--------------------------------------|
| Lobby ID              | 4 bytes  | Unique lobby identifier              |
| Game Mode             | 1 byte   | Selected game mode (see 5.2.1)       |
| Difficulty            | 1 byte   | Selected difficulty (see 5.2.1)      |
| Current Player Count  | 1 byte   | Number of players currently in lobby |
| Required Player Count | 1 byte   | Players needed to start game         |
| Player Entries        | Variable | Array of player information          |

**Player Entry Format (38 bytes each)**:

| Field         | Size     | Offset | Description                    |
|---------------|----------|--------|--------------------------------|
| Player ID     | 4 bytes  | 0      | Player identifier              |
| Player Name   | 32 bytes | 4      | Player name (UTF-8)            |
| Player Level  | 2 bytes  | 36     | Player level (for display)     |

**Total Size**: 8 + (38 × Current Player Count) bytes

**Example**: For 3 players in a squad lobby: 8 + (38 × 3) = 122 bytes

#### 5.2.4 SERVER_GAME_START_COUNTDOWN (0x88)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Lobby ID                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Countdown Value|   Game Mode   |  Difficulty   |    Map ID     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Map ID     |
   +-+-+-+-+-+-+-+-+
```

| Field           | Size    | Description                                 |
|-----------------|---------|---------------------------------------------|
| Lobby ID        | 4 bytes | Lobby identifier                            |
| Countdown Value | 1 byte  | Seconds remaining (5, 4, 3, 2, 1)           |
| Game Mode       | 1 byte  | Game mode (see 5.2.1)                       |
| Difficulty      | 1 byte  | Difficulty (see 5.2.1)                      |
| Map ID          | 2 bytes | Map to be loaded                            |

**Total Size**: 9 bytes

#### 5.2.5 SERVER_COUNTDOWN_CANCELLED (0x89)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Lobby ID                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Reason     | New Plr Count |Required Count |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field                | Size    | Description                        |
|----------------------|---------|------------------------------------|
| Lobby ID             | 4 bytes | Lobby identifier                   |
| Reason               | 1 byte  | Cancellation reason code           |
| New Player Count     | 1 byte  | Current player count after event   |
| Required Count       | 1 byte  | Players needed to restart countdown|

**Reason Codes**:
- `0x01`: PLAYER_LEFT - A player left during countdown
- `0x02`: SERVER_ERROR - Server error occurred

**Total Size**: 7 bytes

#### 5.2.6 SERVER_GAME_START (0x8A)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Game Session ID                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   Game Mode   |  Difficulty   |                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
   |                        Server Tick                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Level Seed                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                   Player Spawn Data (12 bytes each)           |
   |                             ...                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field            | Size     | Description                               |
|------------------|----------|-------------------------------------------|
| Game Session ID  | 4 bytes  | Unique session identifier                 |
| Game Mode        | 1 byte   | Game mode (see 5.2.1)                     |
| Difficulty       | 1 byte   | Difficulty (see 5.2.1)                    |
| Server Tick      | 4 bytes  | Starting server tick                      |
| Level Seed       | 4 bytes  | Random seed for procedural generation     |
| Spawn Data Array | Variable | Player spawn positions                    |

**Player Spawn Data Format (12 bytes each)**:

| Field      | Size    | Offset | Description                    |
|------------|---------|--------|--------------------------------|
| Player ID  | 4 bytes | 0      | Player identifier              |
| Spawn X    | 4 bytes | 4      | X coordinate (IEEE 754 float)  |
| Spawn Y    | 4 bytes | 8      | Y coordinate (IEEE 754 float)  |

**Total Size**: 14 + (12 × Player Count) bytes

### 5.3 Player Input Payloads

#### 5.3.1 CLIENT_INPUT (0x10)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Player ID                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Input Flags           |                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
   |                       Client Tick                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field        | Size    | Description                              |
|--------------|---------|------------------------------------------|
| Player ID    | 4 bytes | Player identifier                        |
| Input Flags  | 2 bytes | Bitfield of pressed inputs               |
| Client Tick  | 4 bytes | Client tick number (for prediction)      |

**Input Flags Bitfield**:
- Bit 0: UP - Move up
- Bit 1: DOWN - Move down
- Bit 2: LEFT - Move left
- Bit 3: RIGHT - Move right
- Bit 4: SHOOT - Fire weapon
- Bit 5: CHARGE - Charge weapon (hold)
- Bit 6: SPECIAL - Special ability
- Bits 7-15: Reserved (MUST be 0)

**Total Size**: 10 bytes

**Frequency**: Clients SHOULD send CLIENT_INPUT at the game frame rate (typically 60 Hz).

### 5.4 World State Payloads

#### 5.4.1 SERVER_SNAPSHOT (0xA0)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Server Tick                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       Entity Count            |                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
   |                   Entity State Array (20 bytes each)          |
   |                             ...                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field         | Size     | Description                         |
|---------------|----------|-------------------------------------|
| Server Tick   | 4 bytes  | Current server tick number          |
| Entity Count  | 2 bytes  | Number of entities in snapshot      |
| Entity States | Variable | Array of entity states              |

**Entity State Format (20 bytes each)**:

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Entity ID                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Entity Type   |                Position X (float)             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Position X    |           Position Y (float)                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Position Y    |    Velocity X (int16)     |  Velocity Y       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Velocity Y   |         Health            |      Flags        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     Flags     |
   +-+-+-+-+-+-+-+-+
```

| Field       | Size    | Offset | Description                            |
|-------------|---------|--------|----------------------------------------|
| Entity ID   | 4 bytes | 0      | Unique entity identifier               |
| Entity Type | 1 byte  | 4      | Entity type code                       |
| Position X  | 4 bytes | 5      | X position (IEEE 754 float)            |
| Position Y  | 4 bytes | 9      | Y position (IEEE 754 float)            |
| Velocity X  | 2 bytes | 13     | X velocity (int16, scaled by 100)      |
| Velocity Y  | 2 bytes | 15     | Y velocity (int16, scaled by 100)      |
| Health      | 2 bytes | 17     | Current health points                  |
| Flags       | 2 bytes | 19     | Entity state flags bitfield            |

**Entity Type Codes**:
- `0x01`: PLAYER
- `0x02`: ENEMY_BASIC
- `0x03`: ENEMY_ELITE
- `0x04`: ENEMY_BOSS
- `0x05`: PROJECTILE_PLAYER
- `0x06`: PROJECTILE_ENEMY
- `0x07`: POWERUP_WEAPON
- `0x08`: POWERUP_SHIELD
- `0x09`: POWERUP_SCORE
- `0x0A`: ENEMY_FAST
- `0x0B`: ENEMY_TANK
- `0x0C`: POWERUP_HEALTH
- `0x0D`: POWERUP_SPEED

**Entity State Flags**:
- Bit 0: INVULNERABLE
- Bit 1: CHARGING
- Bit 2: DAMAGED (visual feedback)
- Bits 3-15: Reserved

**Total Size**: 6 + (20 × Entity Count) bytes

**Maximum Entities**: With 1392 byte payload limit: (1392 - 6) / 20 = 69 entities per snapshot

**Frequency**: Servers SHOULD send snapshots at 20-30 Hz to conserve bandwidth.

### 5.5 Entity Event Payloads

#### 5.5.1 SERVER_ENTITY_SPAWN (0xB0)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Entity ID                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Entity Type   |              Spawn X (float)                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Spawn X      |              Spawn Y (float)                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Spawn Y      |   Subtype     |            Health             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field       | Size    | Description                                 |
|-------------|---------|---------------------------------------------|
| Entity ID   | 4 bytes | Unique entity identifier                    |
| Entity Type | 1 byte  | Entity type code (see 5.4.1)                |
| Spawn X     | 4 bytes | X spawn position (IEEE 754 float)           |
| Spawn Y     | 4 bytes | Y spawn position (IEEE 754 float)           |
| Subtype     | 1 byte  | Entity subtype (0=Basic, 1=Fast, 2=Tank, 3=Boss) |
| Health      | 2 bytes | Initial health points                       |

**Enemy Subtype Codes**:
- `0x00`: BASIC - Standard enemy
- `0x01`: FAST - Fast-moving enemy
- `0x02`: TANK - High-health enemy
- `0x03`: BOSS - Boss enemy

**Total Size**: 16 bytes

#### 5.5.2 SERVER_ENTITY_DESTROY (0xB1)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Entity ID                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Reason     |            Position X (float)                 |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Position X    |            Position Y (float)                 |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Position Y    |
   +-+-+-+-+-+-+-+-+
```

| Field      | Size    | Description                                |
|------------|---------|--------------------------------------------|
| Entity ID  | 4 bytes | Entity identifier                          |
| Reason     | 1 byte  | Destruction reason code                    |
| Position X | 4 bytes | Final X position for effects (float)       |
| Position Y | 4 bytes | Final Y position for effects (float)       |

**Reason Codes**:
- `0x01`: KILLED - Destroyed by damage
- `0x02`: OUT_OF_BOUNDS - Left playable area
- `0x03`: COLLECTED - Power-up collected by player
- `0x04`: TIMEOUT - Timed entity expired

**Total Size**: 13 bytes

#### 5.5.3 SERVER_PROJECTILE_SPAWN (0xB3)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Projectile ID                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Owner ID                             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Projectile Type|            Spawn X (float)                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Spawn X      |            Spawn Y (float)                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Spawn Y      |   Velocity X (int16)      |  Velocity Y       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Velocity Y   |
   +-+-+-+-+-+-+-+-+
```

| Field           | Size    | Description                               |
|-----------------|---------|-------------------------------------------|
| Projectile ID   | 4 bytes | Unique projectile identifier              |
| Owner ID        | 4 bytes | Entity that fired the projectile          |
| Projectile Type | 1 byte  | Projectile type code                      |
| Spawn X         | 4 bytes | X spawn position (IEEE 754 float)         |
| Spawn Y         | 4 bytes | Y spawn position (IEEE 754 float)         |
| Velocity X      | 2 bytes | X velocity (int16, scaled by 100)         |
| Velocity Y      | 2 bytes | Y velocity (int16, scaled by 100)         |

**Projectile Type Codes**:
- `0x01`: BULLET - Standard projectile
- `0x02`: MISSILE - Homing missile
- `0x03`: LASER - Laser beam
- `0x04`: CHARGE_SHOT - Charged weapon shot

**Total Size**: 21 bytes

**Rationale**: This packet enables instant client-side visual feedback for shooting without waiting for the next snapshot.

### 5.6 Game Mechanics Payloads

#### 5.6.1 SERVER_POWERUP_COLLECTED (0xC0)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Player ID                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Powerup Type  |New Weapon Lvl |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field            | Size    | Description                        |
|------------------|---------|------------------------------------|
| Player ID        | 4 bytes | Player who collected the power-up  |
| Powerup Type     | 1 byte  | Power-up type code                 |
| New Weapon Level | 1 byte  | Updated weapon level (0-255)       |

**Powerup Type Codes**:
- `0x01`: WEAPON_UPGRADE - Weapon level increase
- `0x02`: SHIELD - Shield/invulnerability
- `0x03`: SPEED - Movement speed boost
- `0x04`: SCORE - Score bonus
- `0x05`: HEALTH - Health restoration (+20 HP)

**Total Size**: 6 bytes

#### 5.6.2 SERVER_SCORE_UPDATE (0xC1)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Player ID                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Score Delta                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       New Total Score                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |Combo Multiplier|
   +-+-+-+-+-+-+-+-+
```

| Field            | Size    | Description                          |
|------------------|---------|--------------------------------------|
| Player ID        | 4 bytes | Player identifier                    |
| Score Delta      | 4 bytes | Points gained (signed int32)         |
| New Total Score  | 4 bytes | Updated total score                  |
| Combo Multiplier | 1 byte  | Current combo multiplier (1-255)     |

**Total Size**: 13 bytes

#### 5.6.3 SERVER_WAVE_START (0xC2)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Wave Number                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Total Waves           |      Scroll Distance (float)  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Scroll Distance               |      Expected Enemies         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                                                               |
   +                    Wave Name (32 bytes)                       +
   |                             ...                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field             | Size     | Description                                  |
|-------------------|----------|----------------------------------------------|
| Wave Number       | 4 bytes  | Current wave number (from JSON config)       |
| Total Waves       | 2 bytes  | Total number of waves in configuration       |
| Scroll Distance   | 4 bytes  | Scroll distance at which wave triggered (float) |
| Expected Enemies  | 2 bytes  | Number of enemies that will spawn this wave  |
| Wave Name         | 32 bytes | Optional wave name/description (UTF-8)       |

**Total Size**: 44 bytes

**Rationale**: This packet notifies clients when a new wave begins, allowing them to display wave progress UI and prepare for incoming enemies. The scroll distance enables deterministic wave triggering validation.

#### 5.6.4 SERVER_WAVE_COMPLETE (0xC3)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Wave Number                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Completion Time                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       Enemies Killed          |         Bonus Points          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |All Waves Done |
   +-+-+-+-+-+-+-+-+
```

| Field               | Size    | Description                                    |
|---------------------|---------|------------------------------------------------|
| Wave Number         | 4 bytes | Completed wave number                          |
| Completion Time     | 4 bytes | Time taken to complete wave (milliseconds)     |
| Enemies Killed      | 2 bytes | Number of enemies destroyed in this wave       |
| Bonus Points        | 2 bytes | Bonus points awarded for wave completion       |
| All Waves Complete  | 1 byte  | 1 if this was the final wave, 0 otherwise      |

**Total Size**: 13 bytes

**Rationale**: Notifies clients of wave completion, allowing them to display completion statistics, award bonus points, and potentially trigger victory conditions if all waves are complete.

#### 5.6.5 SERVER_PLAYER_RESPAWN (0xC5)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          Player ID                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Respawn X (float)                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Respawn Y (float)                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  Invuln Duration (ms)         |Lives Remaining|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field                   | Size    | Description                            |
|-------------------------|---------|----------------------------------------|
| Player ID               | 4 bytes | Player identifier                      |
| Respawn X               | 4 bytes | X respawn position (float)             |
| Respawn Y               | 4 bytes | Y respawn position (float)             |
| Invulnerability Duration| 2 bytes | Invulnerability time (milliseconds)    |
| Lives Remaining         | 1 byte  | Remaining lives (0 = game over)        |

**Total Size**: 15 bytes

#### 5.6.6 SERVER_GAME_OVER (0xC6)

```
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    Result     |                                               |
   +-+-+-+-+-+-+-+-+                                               +
   |                  Final Score Entry (12 bytes each)            |
   |                             ...                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Total Time (seconds)                     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                        Enemies Killed                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

| Field           | Size     | Description                            |
|-----------------|----------|----------------------------------------|
| Result          | 1 byte   | Game result code                       |
| Score Entries   | Variable | Array of player scores (max 4)         |
| Total Time      | 4 bytes  | Total game time in seconds             |
| Enemies Killed  | 4 bytes  | Total enemies killed by team           |

**Result Codes**:
- `0x01`: VICTORY - Players won
- `0x02`: DEFEAT - All players died
- `0x03`: TIMEOUT - Time limit reached

**Score Entry Format (12 bytes each)**:

| Field       | Size    | Offset | Description          |
|-------------|---------|--------|----------------------|
| Player ID   | 4 bytes | 0      | Player identifier    |
| Final Score | 4 bytes | 4      | Player's final score |
| Deaths      | 2 bytes | 8      | Number of deaths     |
| Kills       | 2 bytes | 10     | Enemies killed       |

**Total Size**: 9 + (12 × Player Count) bytes

---

## 6. Communication Patterns

### 6.1 Connection Establishment

```
Client                                Server
  |                                     |
  |------- CLIENT_CONNECT (0x01) ----->|
  |                                     | (Validate version, capacity)
  |                                     |
  |<------ SERVER_ACCEPT (0x81) -------|  (if accepted)
  |        or SERVER_REJECT (0x82)     |  (if rejected)
  |                                     |
```

Upon successful connection:
1. Client sends CLIENT_CONNECT with player name and version
2. Server validates the request and responds with SERVER_ACCEPT (assigning player ID) or SERVER_REJECT
3. Client enters lobby selection state

### 6.2 Lobby and Matchmaking

```
Client                                Server
  |                                     |
  |---- CLIENT_JOIN_LOBBY (0x05) ----->|
  |      (mode=SQUAD, diff=HARD)       |
  |                                     | (Find/create lobby)
  |                                     |
  |<--- SERVER_LOBBY_STATE (0x87) -----|
  |         (1/4 players)               |
  |                                     |
  |         [Wait for more players...]  |
  |                                     |
  |<--- SERVER_LOBBY_STATE (0x87) -----|
  |         (4/4 players)               |
  |                                     |
  |<- SERVER_GAME_START_COUNTDOWN ----|
  |         (5 seconds)                 |
  |         ... (4) ... (3) ...         |
  |                                     |
  |<--- SERVER_GAME_START (0x8A) ------|
  |                                     |
```

Matchmaking flow:
1. Client selects game mode and difficulty, sends CLIENT_JOIN_LOBBY
2. Server finds an existing compatible lobby or creates a new one
3. Server broadcasts SERVER_LOBBY_STATE to all players in the lobby whenever a player joins/leaves
4. When the lobby reaches the required player count, server initiates countdown
5. Server sends SERVER_GAME_START_COUNTDOWN every second (5, 4, 3, 2, 1)
6. If a player leaves during countdown, server sends SERVER_COUNTDOWN_CANCELLED
7. After countdown completes, server sends SERVER_GAME_START and transitions to gameplay

### 6.3 Gameplay Loop

```
Client                                Server
  |                                     |
  |---- CLIENT_INPUT (0x10) --------->| (60 Hz)
  |---- CLIENT_INPUT (0x10) --------->|
  |---- CLIENT_INPUT (0x10) --------->|
  |                                     |
  |<--- SERVER_SNAPSHOT (0xA0) --------|  (20-30 Hz)
  |                                     |
  |<- SERVER_PROJECTILE_SPAWN (0xB3)---|  (event-driven)
  |<- SERVER_ENTITY_DESTROY (0xB1) ----|  (event-driven)
  |<- SERVER_SCORE_UPDATE (0xC1) ------|  (event-driven)
  |                                     |
```

Gameplay communication:
1. **Client sends inputs at 60 Hz**: CLIENT_INPUT packets with current input state
2. **Server processes inputs**: Server simulation runs at server tick rate (60 Hz)
3. **Server sends snapshots at 20-30 Hz**: Complete world state via SERVER_SNAPSHOT
4. **Server sends critical events immediately**: Spawns, destructions, scoring, etc.

**Client-side prediction**: Clients MAY predict local player movement using CLIENT_INPUT and reconcile with SERVER_SNAPSHOT.

**Interpolation**: Clients SHOULD interpolate entity positions between snapshots for smooth rendering.

### 6.4 Keepalive and Latency Measurement

```
Client                                Server
  |                                     |
  |------ CLIENT_PING (0x04) --------->|  (every 1-2 seconds)
  |        (client_timestamp)           |
  |                                     |
  |<----- SERVER_PONG (0x85) -----------|
  |   (echo client_timestamp + server_timestamp)
  |                                     |
```

Clients SHOULD send CLIENT_PING every 1-2 seconds to:
- Maintain connection (prevent server timeout)
- Measure round-trip time (RTT)
- Detect connection quality issues

RTT calculation: `RTT = current_time - client_timestamp` (from echoed value)

### 6.5 Disconnection

**Graceful disconnection**:
```
Client                                Server
  |                                     |
  |--- CLIENT_DISCONNECT (0x02) ------>|
  |                                     |
  |<-- SERVER_PLAYER_LEFT (0x84) ------|  (broadcast to others)
  |                                     |
```

**Timeout disconnection**:
- If server doesn't receive any packet from client for 10 seconds, server SHOULD disconnect the client
- Server broadcasts SERVER_PLAYER_LEFT to remaining players

---

## 7. Error Handling

### 7.1 Version Mismatch

- Receivers MUST silently drop packets with unknown protocol versions
- Servers SHOULD respond to CLIENT_CONNECT with version mismatch using SERVER_REJECT (reason code 0x02)

### 7.2 Packet Size Violations

- Senders MUST NOT send packets exceeding 1400 bytes total size
- Receivers MAY drop packets exceeding the maximum size
- Implementations SHOULD log oversized packet attempts for debugging

### 7.3 Invalid Payload Data

- Receivers MUST validate all numeric fields for reasonable ranges
- String fields MUST be checked for null-termination
- Invalid packets SHOULD be silently dropped (do not crash)

### 7.4 Sequence Number Handling

- Receivers SHOULD track sequence numbers to detect packet loss
- Out-of-order packets MAY be processed or dropped based on packet type
- Duplicate packets (same sequence number) SHOULD be dropped

### 7.5 Connection Timeout

- Clients not sending packets for 10 seconds SHOULD be disconnected
- Servers not responding for 10 seconds SHOULD trigger client reconnection attempt

---

## 8. Security Considerations

### 8.1 Denial of Service

- Servers SHOULD implement rate limiting per source IP address
- Servers SHOULD validate all packet sizes before processing
- Servers SHOULD limit the number of concurrent connections per IP

### 8.2 Data Validation

- All numeric fields MUST be validated for reasonable ranges
- String fields MUST be validated for proper encoding and length
- Player names SHOULD be sanitized to prevent injection attacks

### 8.3 Replay Attacks

- Servers MAY use sequence numbers to detect and reject replayed packets
- Servers SHOULD maintain a sliding window of recent sequence numbers

### 8.4 Cheating Prevention

- Server is authoritative for all game state
- Client predictions are reconciled against server state
- Servers SHOULD validate input feasibility (e.g., movement speed)
- Servers SHOULD implement server-side hit detection

---

## 9. Implementation Notes

### 9.1 Byte Order

- All multi-byte integers are in network byte order (big-endian)
- Use `htons()`, `htonl()`, `ntohs()`, `ntohl()` for conversions on little-endian systems
- Floating-point values use IEEE 754 single precision format

### 9.2 Structure Packing

All C/C++ protocol structures MUST use `__attribute__((packed))` or equivalent to prevent compiler padding:

```cpp
struct __attribute__((packed)) PacketHeader {
    uint8_t version;
    uint8_t type;
    uint16_t payload_length;
    uint32_t sequence_number;
};
```

### 9.3 Encoding and Decoding

Implementations SHOULD provide encoder/decoder functions that:
- Validate packet sizes before encoding
- Perform byte order conversions automatically
- Return errors for invalid data

### 9.4 Memory Management

- Packet buffers SHOULD be reused to minimize allocation overhead
- Fixed-size buffers of 1400 bytes are RECOMMENDED for packet reception

### 9.5 Performance Optimization

**Snapshot compression**:
- Implement delta compression (SERVER_DELTA_SNAPSHOT) for reduced bandwidth
- Only send changed entities between snapshots

**Input batching**:
- Clients MAY batch multiple inputs into a single packet if tick rate allows

**Priority system**:
- Critical packets (inputs, spawns) SHOULD be sent with higher priority
- Non-critical packets (chat) MAY be delayed under congestion

---

## 10. References

- **RFC 768**: User Datagram Protocol
- **RFC 2119**: Key words for use in RFCs to Indicate Requirement Levels
- **IEEE 754**: IEEE Standard for Floating-Point Arithmetic

---

## Appendix A: Packet Type Quick Reference

| ID   | Name                      | C→S | S→C | Frequency    |
|------|---------------------------|-----|-----|--------------|
| 0x01 | CLIENT_CONNECT            | ✓   |     | Once         |
| 0x02 | CLIENT_DISCONNECT         | ✓   |     | Once         |
| 0x04 | CLIENT_PING               | ✓   |     | 1-2 Hz       |
| 0x05 | CLIENT_JOIN_LOBBY         | ✓   |     | Once         |
| 0x06 | CLIENT_LEAVE_LOBBY        | ✓   |     | Once         |
| 0x10 | CLIENT_INPUT              | ✓   |     | 60 Hz        |
| 0x81 | SERVER_ACCEPT             |     | ✓   | Once         |
| 0x82 | SERVER_REJECT             |     | ✓   | Once         |
| 0x83 | SERVER_PLAYER_JOINED      |     | ✓   | Event        |
| 0x84 | SERVER_PLAYER_LEFT        |     | ✓   | Event        |
| 0x85 | SERVER_PONG               |     | ✓   | 1-2 Hz       |
| 0x87 | SERVER_LOBBY_STATE        |     | ✓   | Event        |
| 0x88 | SERVER_GAME_START_COUNTDOWN|    | ✓   | 1 Hz         |
| 0x89 | SERVER_COUNTDOWN_CANCELLED|     | ✓   | Event        |
| 0x8A | SERVER_GAME_START         |     | ✓   | Once         |
| 0xA0 | SERVER_SNAPSHOT           |     | ✓   | 20-30 Hz     |
| 0xB0 | SERVER_ENTITY_SPAWN       |     | ✓   | Event        |
| 0xB1 | SERVER_ENTITY_DESTROY     |     | ✓   | Event        |
| 0xB3 | SERVER_PROJECTILE_SPAWN   |     | ✓   | High freq    |
| 0xC0 | SERVER_POWERUP_COLLECTED  |     | ✓   | Event        |
| 0xC1 | SERVER_SCORE_UPDATE       |     | ✓   | Event        |
| 0xC2 | SERVER_WAVE_START         |     | ✓   | Event        |
| 0xC3 | SERVER_WAVE_COMPLETE      |     | ✓   | Event        |
| 0xC5 | SERVER_PLAYER_RESPAWN     |     | ✓   | Event        |
| 0xC6 | SERVER_GAME_OVER          |     | ✓   | Once         |

---

## Appendix B: Example Packet Hex Dumps

### B.1 CLIENT_CONNECT Example

**Scenario**: Player "Alice" connects with client version 1

```
Header:
  Version: 0x01
  Type: 0x01 (CLIENT_CONNECT)
  Payload Length: 0x0021 (33 bytes)
  Sequence: 0x00000001

Payload:
  Client Version: 0x01
  Player Name: "Alice" (null-padded to 32 bytes)
```

**Hex Dump**:
```
01 01 00 21 00 00 00 01  01 41 6C 69 63 65 00 00
00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00  00
```

### B.2 CLIENT_INPUT Example

**Scenario**: Player 42 presses UP + SHOOT (bits 0 and 4 set) at tick 1000

```
Header:
  Version: 0x01
  Type: 0x10 (CLIENT_INPUT)
  Payload Length: 0x000A (10 bytes)
  Sequence: 0x00000064 (100)

Payload:
  Player ID: 0x0000002A (42)
  Input Flags: 0x0011 (UP=bit0, SHOOT=bit4)
  Client Tick: 0x000003E8 (1000)
```

**Hex Dump**:
```
01 10 00 0A 00 00 00 64  00 00 00 2A 00 11 00 00
03 E8
```

### B.3 SERVER_GAME_START Example

**Scenario**: Squad game starts with 4 players on HARD difficulty

```
Header:
  Version: 0x01
  Type: 0x8A (SERVER_GAME_START)
  Payload Length: 0x003E (62 bytes)
  Sequence: 0x00000001

Payload:
  Game Session ID: 0x12345678
  Game Mode: 0x03 (SQUAD)
  Difficulty: 0x03 (HARD)
  Server Tick: 0x00000000
  Level Seed: 0xABCDEF12

  Player 1 Spawn:
    Player ID: 0x0000002A (42)
    Spawn X: 100.0 (0x42C80000)
    Spawn Y: 200.0 (0x43480000)

  Player 2 Spawn:
    Player ID: 0x0000002B
    Spawn X: 100.0
    Spawn Y: 250.0 (0x43790000)

  [... 2 more players ...]
```

**Hex Dump** (partial):
```
01 8A 00 3E 00 00 00 01  12 34 56 78 03 03 00 00
00 00 AB CD EF 12 00 00  00 2A 42 C8 00 00 43 48
00 00 00 00 00 2B 42 C8  00 00 43 79 00 00 ...
```

---

## Appendix C: State Machine Diagrams

### C.1 Client State Machine

```
     ┌─────────────┐
     │DISCONNECTED │
     └──────┬──────┘
            │ CLIENT_CONNECT
            ▼
     ┌─────────────┐
     │  CONNECTED  │◄────┐
     │   (Lobby)   │     │ CLIENT_LEAVE_LOBBY
     └──────┬──────┘     │
            │ CLIENT_JOIN_LOBBY
            ▼            │
     ┌─────────────┐     │
     │  IN_LOBBY   │─────┘
     └──────┬──────┘
            │ SERVER_GAME_START_COUNTDOWN
            ▼
     ┌─────────────┐
     │  COUNTDOWN  │
     └──────┬──────┘
            │ SERVER_GAME_START
            ▼
     ┌─────────────┐
     │   IN_GAME   │
     └──────┬──────┘
            │ SERVER_GAME_OVER
            ▼
     ┌─────────────┐
     │  GAME_OVER  │
     └──────┬──────┘
            │
            └──────► (back to CONNECTED)
```

### C.2 Server Lobby State Machine

```
     ┌─────────────┐
     │ WAITING     │◄────┐
     │ (< required │     │ Player leaves
     │  players)   │     │
     └──────┬──────┘     │
            │ Lobby full │
            ▼            │
     ┌─────────────┐     │
     │  COUNTDOWN  │─────┘
     │  (5...1)    │
     └──────┬──────┘
            │ Countdown complete
            ▼
     ┌─────────────┐
     │   STARTED   │
     │ (transition │
     │  to game)   │
     └─────────────┘
```

---

## Appendix D: Bandwidth Analysis

### D.1 Typical Traffic Per Player

**Client Upload** (per second):
- CLIENT_INPUT: 60 Hz × 18 bytes (8 header + 10 payload) = 1,080 bytes/s
- CLIENT_PING: 1 Hz × 16 bytes = 16 bytes/s
- **Total**: ~1,096 bytes/s = **~8.8 Kbps upload per player**

**Client Download** (per second, 4-player game):
- SERVER_SNAPSHOT: 25 Hz × 200 bytes (avg) = 5,000 bytes/s
- Event packets (spawns, scores, etc.): ~100 bytes/s (variable)
- **Total**: ~5,100 bytes/s = **~40.8 Kbps download per player**

**Total Bandwidth** (per player): ~50 Kbps (well within typical broadband)

### D.2 Server Bandwidth (4-player game)

- Upload: 4 players × 40 Kbps = 160 Kbps
- Download: 4 players × 8.8 Kbps = 35 Kbps
- **Total per game session**: ~200 Kbps

---

**End of Protocol Specification**
