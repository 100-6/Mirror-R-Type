# R-Type Server Development Guide

This guide outlines the architecture and implementation steps for the R-Type Server MVP. It is based on the existing ECS architecture, the shared protocol definitions, and the project requirements.

## 1. Project Overview & Goals

**Objective:** Implement a robust, multiplayer game server for R-Type capable of handling 2-4 players via UDP.

**Key Requirements:**
- **Capacity:** 2-4 concurrent players.
- **Networking:** UDP via `INetworkPlugin`.
- **Architecture:** Server-authoritative (server decides truth).
- **Stability:** 10+ minutes runtime without crashes.
- **Latency:** <100ms processing overhead.

## 2. Architecture

The server will be built as a standalone application located in `src/r-type/server/`. It will leverage the core engine libraries (`src/engine/`) and the shared protocol (`src/r-type/shared/protocol/`).

### High-Level Components

1.  **Network Layer (`NetworkSystem`)**:
    -   Uses the `INetworkPlugin` to send/receive raw bytes.
    -   Deserializes incoming packets into events or components.
    -   Serializes game state into outgoing packets (Snapshots).
    
2.  **ECS Core (`Registry`)**:
    -   The central source of truth.
    -   Stores all entities (Players, Enemies, Projectiles) and their state (Position, Velocity, Health).
    
3.  **Game Loop**:
    -   **Tick Rate:** Fixed (e.g., 60 TPS).
    -   **Steps:**
        1.  **Poll Network:** Read all incoming packets from the plugin.
        2.  **Process Events:** Handle joins, leaves, and inputs.
        3.  **Update Systems:** Run physics, collision, AI, and game logic systems.
        4.  **Broadcast State:** Send `SERVER_SNAPSHOT` or specific events to clients.

## 3. Protocol Implementation

The protocol is already defined in `src/r-type/shared/protocol/`. The server must strictly adhere to these definitions.

### Key Files
-   `PacketHeader.hpp`: The 8-byte header (Version, Type, Length, Sequence).
-   `PacketTypes.hpp`: Message IDs (e.g., `CLIENT_INPUT` = 0x10).
-   `Payloads.hpp`: Packed structs for message bodies.

### Packet Handling Flow
1.  **Receive**: `plugin->receive()` returns a `NetworkPacket`.
2.  **Parse Header**: Read the first 8 bytes as `PacketHeader`.
3.  **Validate**: Check `version`, `payload_length`, and `type`.
4.  **Route**: Switch on `header.type` to cast the payload to the correct struct.

## 4. Implementation Phases

### Phase 1: Infrastructure Setup
**Goal:** A running server that listens on a port.
1.  Modify `src/r-type/server/src/main.cpp`.
2.  Load the Network Plugin (DLL/SO) using `PluginManager`.
3.  Initialize `INetworkPlugin` and call `start_server(port, UDP)`.
4.  Implement the basic `while (running)` loop.

### Phase 2: Connection Management
**Goal:** Clients can connect and see the "Lobby".
1.  Handle `CLIENT_CONNECT` (0x01).
    -   Check if server is full.
    -   Create a "Player" entity in the ECS.
    -   Send `SERVER_ACCEPT` (0x81) with the assigned Player ID.
    -   Broadcast `SERVER_PLAYER_JOINED` (0x83) to others.
2.  Handle `CLIENT_DISCONNECT` (0x02) and timeouts.
    -   Remove entity.
    -   Broadcast `SERVER_PLAYER_LEFT` (0x84).
3.  Implement a `LobbySystem` to manage the "Waiting for players" state.

### Phase 3: Input & Movement
**Goal:** Players can move their ships.
1.  Handle `CLIENT_INPUT` (0x10).
    -   This packet contains `InputFlags` (Up, Down, Left, Right, Shoot).
    -   **Do not trust the client's position.**
    -   Update the player entity's `Velocity` component based on input flags.
2.  Create/Use `PhysicsSystem`:
    -   Update `Position` based on `Velocity`.
    -   Enforce map boundaries (clamp position).

### Phase 4: State Synchronization (The "Netcode")
**Goal:** Clients see moving objects.
1.  Create `NetworkBroadcastSystem`.
2.  Periodically (e.g., every tick or every other tick):
    -   Iterate all networked entities.
    -   Construct `EntityState` structs.
    -   Pack them into a `SERVER_SNAPSHOT` (0xA0) packet.
    -   Broadcast to all clients.

### Phase 5: Gameplay Logic
**Goal:** Enemies, Shooting, Scoring.
1.  **Shooting:**
    -   When `INPUT_SHOOT` is active and cooldown allows:
    -   Spawn a `Projectile` entity.
    -   Broadcast `SERVER_PROJECTILE_SPAWN` (0xB3).
2.  **Enemies (AI):**
    -   `WaveSystem`: Spawns enemies based on time/wave config.
    -   `AISystem`: Updates enemy velocities (patterns).
3.  **Collision:**
    -   Server checks overlaps (AABB).
    -   On hit:
        -   Deduct health.
        -   If dead: `registry.kill_entity()`.
        -   Broadcast `SERVER_ENTITY_DESTROY` (0xB1) and `SERVER_SCORE_UPDATE` (0xC1).

## 5. Code Structure Proposal

```cpp
// src/r-type/server/src/Server.hpp

class Server {
public:
    Server(const std::string& plugin_path);
    void run();

private:
    void handle_packets();
    void update_game(float dt);
    void broadcast_state();
    
    // Packet Handlers
    void on_connect(ClientId id, const ClientConnectPayload& payload);
    void on_input(ClientId id, const ClientInputPayload& payload);

    std::unique_ptr<engine::INetworkPlugin> _network;
    engine::Registry _registry;
    // ...
};
```

## 6. Serialization Helper

Since the protocol uses packed structs, you can cast raw bytes directly, but be careful with endianness if crossing architectures (Standard is Big Endian for network fields).

```cpp
// Example: Sending a packet
template<typename Payload>
void send_packet(engine::INetworkPlugin* net, ClientId target, PacketType type, const Payload& payload) {
    std::vector<uint8_t> buffer(sizeof(PacketHeader) + sizeof(Payload));
    
    PacketHeader header(static_cast<uint8_t>(type), sizeof(Payload), _seq_num++);
    // TODO: Convert header fields to Big Endian if needed (htons/htonl)
    
    std::memcpy(buffer.data(), &header, sizeof(Header));
    std::memcpy(buffer.data() + sizeof(Header), &payload, sizeof(Payload));
    
    net->send_to(engine::NetworkPacket(buffer), target);
}
```

## 7. Todo List for Developer

1.  [ ] **Setup**: Link `Server` executable with `Engine` and `PluginManager`.
2.  [ ] **Network**: Load `asio_network_plugin`.
3.  [ ] **Lobby**: Implement room logic (Waiting -> Playing).
4.  [ ] **Game Loop**: Connect ECS `run_systems`.
5.  [ ] **Replication**: Implement `Snapshot` builder.

## 8. Common Pitfalls
-   **Don't trust the client**: Never accept position updates from clients. Only accept inputs (intent).
-   **Packet Fragmentation**: UDP is packet-based, so fragmentation is less of an issue than TCP stream parsing, but ensure payloads don't exceed MTU (keep snapshots small or split them).
-   **Concurrency**: The ECS is not thread-safe by default. Ensure network callbacks allow thread-safe queueing of events, or run network polling on the main thread.
