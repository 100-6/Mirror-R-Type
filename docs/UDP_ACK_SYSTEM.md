# UDP ACK System - R-Type Network Protocol

**Status**: Implementation Documentation
**Version**: 1.0
**Date**: January 2026
**Author**: R-Type Development Team

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Why ACK over UDP?](#2-why-ack-over-udp)
3. [ACK System Architecture](#3-ack-system-architecture)
4. [Implementation](#4-implementation)
5. [Data Flow](#5-data-flow)
6. [Advantages and Trade-offs](#6-advantages-and-trade-offs)
7. [Metrics and Monitoring](#7-metrics-and-monitoring)

---

## 1. Introduction

### 1.1 Context

The R-Type protocol uses **UDP (User Datagram Protocol)** for real-time gameplay communication. UDP, unlike TCP, is an **unreliable** protocol: it provides no guarantees about packet delivery, ordering, or duplicate detection.

However, for certain critical data types in a multiplayer game, we need **acknowledgment (ACK)** to:
- Confirm that player inputs have been received
- Enable client-server reconciliation
- Detect packet loss
- Optimize bandwidth usage

### 1.2 UDP Challenges

The main challenges with UDP:
- **Packet loss**: Packets can be lost in transit
- **Out-of-order delivery**: Packets can arrive in the wrong order
- **No acknowledgment**: The sender doesn't know if the packet arrived
- **No flow control**: No native mechanism to prevent congestion

Our ACK system solves these problems **while maintaining UDP's low latency**.

---

## 2. Why ACK over UDP?

### 2.1 Real-time Gameplay Requirements

In R-Type, we have two types of data:

#### Non-critical data (loss-tolerant)
- **State snapshots**: Sent at 60 Hz, losing one snapshot isn't critical since a new one arrives 16ms later
- **Entity positions**: Interpolated client-side, a missing position can be compensated
- **Visual effects**: Not essential to gameplay

#### Critical data (requiring confirmation)
- **Player inputs**: Must be confirmed for client prediction
- **Critical entity spawns**: Bosses, important power-ups
- **Score and progression**: Must be reliable
- **Important game events**: Wave start/end, victory/defeat

### 2.2 Why not TCP?

TCP provides native ACKs, but:
- **Head-of-line blocking**: A lost packet blocks all subsequent packets
- **Increased latency**: Automatic retransmissions increase latency
- **Overhead**: Larger headers (20 bytes vs 8 bytes for UDP)
- **No control**: Can't choose which packets are critical

**Our solution**: UDP + Selective ACK = Best of both worlds!

---

## 3. ACK System Architecture

### 3.1 Sequence Number-Based ACK System

Our implementation uses an **implicit ACK via piggybacking** system:

```
┌─────────────────────────────────────────────────────────┐
│              R-Type UDP ACK Architecture                │
└─────────────────────────────────────────────────────────┘

CLIENT                                    SERVER
  │                                          │
  │  ┌─────────────────────────┐            │
  │  │ CLIENT_INPUT            │            │
  │  │ ├─ sequence_number: 42  │            │
  │  │ ├─ input_flags: 0x05    │            │
  │  │ └─ timestamp            │            │
  │  └─────────────────────────┘            │
  │────────────────────────────────────────►│
  │                                          │
  │                               ┌──────────────────────┐
  │                               │ Process input #42    │
  │                               │ Update game state    │
  │                               │ Store last_ack = 42  │
  │                               └──────────────────────┘
  │                                          │
  │            ┌─────────────────────────┐   │
  │            │ SERVER_SNAPSHOT         │   │
  │            │ ├─ entity_count         │   │
  │            │ └─ entities[]           │   │
  │            │    ├─ player_id: 123    │   │
  │            │    ├─ position: (x,y)   │   │
  │            │    └─ last_ack_seq: 42  │◄──┘ Implicit ACK!
  │            └─────────────────────────┘   │
  │◄────────────────────────────────────────┤
  │                                          │
  │  ┌──────────────────────────┐            │
  │  │ Client receives ACK #42  │            │
  │  │ → Confirms input handled │            │
  │  │ → Can clean buffer       │            │
  │  └──────────────────────────┘            │
  │                                          │
```

### 3.2 Key Components

#### A) PacketHeader (all packets)

Every UDP packet contains a **monotonic sequence number**:

```cpp
struct PacketHeader {
    uint8_t  version;          // Protocol version (0x01)
    uint8_t  type;             // Packet type
    uint8_t  flags;            // Flags (compression, etc.)
    uint16_t payload_length;   // Payload size
    uint32_t sequence_number;  // ← Monotonic sequence for ordering
    // ... optional compression fields
} __attribute__((packed));
```

**Usage**:
- **Monotonic**: Increments with each sent packet
- **Ordering**: Allows detection of out-of-order packets
- **Loss detection**: Gaps in sequence = packet loss

#### B) ClientInputPayload (player inputs)

```cpp
struct ClientInputPayload {
    uint32_t sequence_number;  // ← Input-specific sequence
    uint16_t input_flags;      // Pressed buttons
    uint32_t timestamp;        // Client timestamp
    float    delta_time;       // Delta since last input
} __attribute__((packed));
```

**Usage**:
- Client increments `sequence_number` for each input sent
- Server must ACK this sequence to confirm processing

#### C) EntityState (state snapshots)

```cpp
struct EntityState {
    uint32_t entity_id;
    uint16_t entity_type;
    float    position_x;
    float    position_y;
    int16_t  velocity_x;
    int16_t  velocity_y;
    uint16_t health;
    uint16_t flags;
    uint32_t last_ack_sequence;  // ← ACK of last processed input
} __attribute__((packed));
```

**Usage**:
- For player-controlled entities, `last_ack_sequence` contains the **last processed input_sequence**
- Client compares this value with its pending inputs
- If `last_ack_sequence >= input.sequence`, the input is confirmed ✅

---

## 4. Implementation

### 4.1 Client-Side (ClientPredictionSystem)

**Files**:
- `src/r-type/client/include/systems/ClientPredictionSystem.hpp`
- `src/r-type/client/src/systems/ClientPredictionSystem.cpp`

#### Data Structure

```cpp
class ClientPredictionSystem {
private:
    uint32_t local_player_id_;
    uint32_t next_sequence_number_;  // Next sequence number
    std::deque<PredictedInput> pending_inputs_;  // Buffer of unacknowledged inputs

    static constexpr size_t MAX_PENDING_INPUTS = 64;
};

struct PredictedInput {
    uint32_t sequence_number;   // Unique sequence number
    uint16_t input_flags;       // Copy of input flags
    uint32_t timestamp;         // Send timestamp
};
```

#### Sending Input with Sequence

```cpp
void ClientPredictionSystem::on_input_sent(uint16_t input_flags) {
    // Create input with sequence
    PredictedInput predicted = {
        .sequence_number = next_sequence_number_++,
        .input_flags = input_flags,
        .timestamp = get_current_timestamp()
    };

    // Store in buffer (for future reconciliation)
    pending_inputs_.push_back(predicted);

    // Limit buffer size
    if (pending_inputs_.size() > MAX_PENDING_INPUTS) {
        pending_inputs_.pop_front();
    }

    // Packet is sent by NetworkSystem with this sequence_number
    // CLIENT_INPUT { sequence_number: predicted.sequence_number, ... }
}
```

#### Receiving ACK from Server

```cpp
void ClientPredictionSystem::on_snapshot_received(
    const SnapshotPayload& snapshot
) {
    // Iterate through snapshot entities
    for (const auto& entity : snapshot.entities) {
        // Find local player entity
        if (entity.entity_id == local_player_id_) {
            uint32_t acked_sequence = entity.last_ack_sequence;

            // Confirm all inputs up to this sequence
            acknowledge_input(acked_sequence);
            break;
        }
    }
}

void ClientPredictionSystem::acknowledge_input(uint32_t ack_sequence) {
    // Remove all confirmed inputs (≤ ack_sequence)
    while (!pending_inputs_.empty() &&
           pending_inputs_.front().sequence_number <= ack_sequence) {
        pending_inputs_.pop_front();
    }

    // Remaining inputs in pending_inputs_ are unacknowledged
    // → Can be used for reconciliation if needed
}
```

### 4.2 Server-Side (ServerNetworkSystem)

**Files**:
- `src/r-type/server/include/ServerNetworkSystem.hpp`
- `src/r-type/server/src/ServerNetworkSystem.cpp`

#### Data Structure

```cpp
class ServerNetworkSystem {
private:
    // Tracking last processed input per player
    std::unordered_map<uint32_t, uint32_t> last_processed_input_seq_;
    // Key: player_id, Value: last processed input sequence
};
```

#### Processing Input with Sequence

```cpp
void ServerNetworkSystem::on_input_received(
    uint32_t player_id,
    const ClientInputPayload& input
) {
    uint32_t input_seq = input.sequence_number;

    // Check if this input has already been processed (duplication)
    if (input_seq <= last_processed_input_seq_[player_id]) {
        // Input already processed or old out-of-order → ignore
        return;
    }

    // Process input (apply movement, shooting, etc.)
    process_player_input(player_id, input);

    // Update last processed input
    last_processed_input_seq_[player_id] = input_seq;

    // Next snapshot will automatically include this last_ack_sequence
}
```

#### Generating Snapshot with ACK

```cpp
void ServerNetworkSystem::send_snapshot() {
    SnapshotPayload snapshot;
    snapshot.server_tick = current_tick_;

    // For each game entity
    for (const auto& entity : game_entities_) {
        EntityState state;
        state.entity_id = entity.id;
        state.position_x = entity.position.x;
        state.position_y = entity.position.y;
        // ... other fields ...

        // If it's a player, include last_ack_sequence
        if (entity.is_player()) {
            uint32_t player_id = entity.get_player_id();
            state.last_ack_sequence = last_processed_input_seq_[player_id];
        } else {
            state.last_ack_sequence = 0;  // Not applicable
        }

        snapshot.entities.push_back(state);
    }

    // Broadcast snapshot to all clients
    broadcast_snapshot(snapshot);
}
```

---

## 5. Data Flow

### 5.1 Normal Scenario (no packet loss)

```
Time  │ Client (sequence)           │ Server (last_ack)       │ Network
──────┼─────────────────────────────┼─────────────────────────┼──────────
t=0   │ Send INPUT seq=100 ────────►│                         │ ✓ Delivered
t=16  │                             │ Process seq=100         │
      │                             │ last_ack[player] = 100  │
t=16  │                             │◄────── SNAPSHOT         │ last_ack=100
t=32  │ Receive ACK(100)            │                         │
      │ → Confirm input 100         │                         │
      │ → Remove from pending       │                         │
──────┼─────────────────────────────┼─────────────────────────┼──────────
t=33  │ Send INPUT seq=101 ────────►│                         │ ✓ Delivered
t=49  │                             │ Process seq=101         │
      │                             │ last_ack[player] = 101  │
t=49  │                             │◄────── SNAPSHOT         │ last_ack=101
t=65  │ Receive ACK(101)            │                         │
      │ → Confirm input 101         │                         │
```

**Result**: Input RTT ≈ 32-65ms (typical for 60 Hz updates)

### 5.2 Packet Loss Scenario

```
Time  │ Client (sequence)           │ Server (last_ack)       │ Network
──────┼─────────────────────────────┼─────────────────────────┼──────────
t=0   │ Send INPUT seq=200 ────────►│                         │ ✗ LOST!
      │ pending: [200]              │                         │
──────┼─────────────────────────────┼─────────────────────────┼──────────
t=16  │                             │ No input received       │
      │                             │ last_ack[player] = 199  │ (unchanged)
t=16  │                             │◄────── SNAPSHOT         │ last_ack=199
t=32  │ Receive ACK(199)            │                         │
      │ → Input 200 still pending   │                         │
      │ pending: [200]              │                         │
──────┼─────────────────────────────┼─────────────────────────┼──────────
t=33  │ Send INPUT seq=201 ────────►│                         │ ✓ Delivered
      │ pending: [200, 201]         │                         │
t=49  │                             │ Process seq=201         │
      │                             │ last_ack[player] = 201  │ ← Skip seq=200!
t=49  │                             │◄────── SNAPSHOT         │ last_ack=201
t=65  │ Receive ACK(201)            │                         │
      │ → Confirm inputs ≤ 201      │                         │
      │ → Remove 200 AND 201        │                         │
      │ pending: []                 │                         │
```

**Result**:
- ✅ Server processes seq=201 even if seq=200 is lost
- ✅ Client cleans all inputs ≤ 201
- ⚠️ Input seq=200 is **permanently lost** (acceptable for high-frequency inputs)

### 5.3 Out-of-Order Delivery Scenario

```
Time  │ Client (sequence)           │ Server (last_ack)       │ Network
──────┼─────────────────────────────┼─────────────────────────┼──────────
t=0   │ Send INPUT seq=300 ────────►│                         │ ⏱ Delayed
t=16  │ Send INPUT seq=301 ────────►│                         │ ✓ Fast path
──────┼─────────────────────────────┼─────────────────────────┼──────────
t=20  │                             │ Receive seq=301 FIRST!  │
      │                             │ Process seq=301         │
      │                             │ last_ack = 301          │
──────┼─────────────────────────────┼─────────────────────────┼──────────
t=30  │                             │ Receive seq=300 (late)  │
      │                             │ → seq=300 < last_ack    │
      │                             │ → REJECT (already past) │
──────┼─────────────────────────────┼─────────────────────────┼──────────
t=32  │                             │◄────── SNAPSHOT         │ last_ack=301
t=48  │ Receive ACK(301)            │                         │
      │ → Confirm inputs ≤ 301      │                         │
      │ → Remove 300 and 301        │                         │
```

**Result**:
- ✅ Server rejects old out-of-order inputs
- ✅ Prevents duplicate processing
- ✅ Client cleans correctly via last_ack

---

## 6. Advantages and Trade-offs

### 6.1 Advantages

#### ✅ Bandwidth Efficiency
- **Piggybacking**: ACK is included in snapshot (no dedicated packet)
- **No overhead**: 4 bytes per player entity (already present in EntityState)
- **No retransmissions**: Unlike TCP, no automatic retransmissions

#### ✅ Low Latency
- **No head-of-line blocking**: Each input is independent
- **No waiting**: Client can send seq=N+1 before receiving ACK(N)
- **Native UDP**: Minimal latency (~1-2ms overhead vs raw UDP)

#### ✅ Packet Loss Detection
- **Sequence gaps**: If ACK jumps from 100 to 103, we know 101-102 are lost
- **Metrics**: Can automatically calculate packet loss rate
- **Adaptation**: Can adjust strategy based on loss rate

#### ✅ Client-Server Reconciliation
- **Client prediction**: Client can apply inputs locally immediately
- **Server confirmation**: ACK confirms server has same view
- **Correction**: If divergence, client can correct based on snapshot

### 6.2 Trade-offs

#### ⚠️ No Delivery Guarantee
- **Lost inputs**: A lost input will never be recovered
- **Mitigation**: High input frequency (60 Hz) makes loss of one input acceptable
- **Impact**: Player may experience micro-freeze if multiple consecutive inputs are lost

#### ⚠️ ACK Delay
- **Latency**: ACK arrives with next snapshot (~16-33ms @ 60-30 Hz)
- **No immediate ACK**: Unlike TCP, no instant ACK
- **Trade-off**: We sacrifice immediate ACK to save packets

#### ⚠️ Out-of-Order Handling
- **Rejection**: Old out-of-order inputs are rejected
- **Consequence**: A delayed input can be lost even if it arrives
- **Acceptable**: For a 60 Hz game, a 50ms delayed input has no value

---

## 7. Metrics and Monitoring

### 7.1 Client-Side Metrics

#### Pending Inputs
```cpp
size_t pending_count = pending_inputs_.size();

if (pending_count > 30) {
    // WARNING: More than 30 unacknowledged inputs
    // → Probable network issues or server lag
    display_warning("High latency detected");
}

if (pending_count > 60) {
    // CRITICAL: Buffer full
    // → Connection timeout probable
    disconnect_with_error("Connection lost");
}
```

#### Input Acknowledgment Delay
```cpp
uint32_t input_rtt_ms = current_time - pending_inputs_.front().timestamp;

if (input_rtt_ms > 200) {
    // Input ACK takes more than 200ms
    // → Display lag indicator
    show_lag_indicator();
}
```

#### Packet Loss Rate
```cpp
uint32_t total_inputs_sent = 0;
uint32_t total_inputs_acked = 0;

void on_input_sent() { total_inputs_sent++; }
void on_ack_received(uint32_t ack_seq) {
    // Count how many inputs were ACKed
    total_inputs_acked++;
}

float packet_loss_rate = 1.0f - (float(total_inputs_acked) / total_inputs_sent);
// Example: 0.023 = 2.3% packet loss
```

### 7.2 Server-Side Metrics

#### Input Reception Rate
```cpp
std::unordered_map<uint32_t, uint32_t> inputs_received_per_player;
std::unordered_map<uint32_t, uint32_t> last_input_time_per_player;

void on_input_received(uint32_t player_id) {
    inputs_received_per_player[player_id]++;
    last_input_time_per_player[player_id] = current_time_ms();
}

void check_player_timeout() {
    for (const auto& [player_id, last_time] : last_input_time_per_player) {
        uint32_t silence_duration = current_time_ms() - last_time;

        if (silence_duration > 10000) {  // 10 seconds
            // No input for 10s → Disconnect
            disconnect_player(player_id, "Timeout");
        } else if (silence_duration > 5000) {
            // 5 seconds → Warning
            log_warning("Player {} inactive for 5s", player_id);
        }
    }
}
```

#### Sequence Gap Detection
```cpp
void on_input_received(uint32_t player_id, uint32_t input_seq) {
    uint32_t expected_seq = last_processed_input_seq_[player_id] + 1;

    if (input_seq > expected_seq) {
        uint32_t gap = input_seq - expected_seq;

        // Log packet loss
        log_info("Player {} lost {} inputs (seq {}-{})",
                 player_id, gap, expected_seq, input_seq - 1);

        // Statistics
        packet_loss_stats_[player_id] += gap;
    }
}
```

### 7.3 Logging and Debugging

#### Recommended Log Format
```
[Client] INPUT_SENT    seq=12345 flags=0x05 ts=1234567890
[Client] ACK_RECEIVED  seq=12340 (5 inputs pending)
[Server] INPUT_RECV    player=1 seq=12345 (gap=0)
[Server] INPUT_PROCESS player=1 seq=12345 last_ack=12345
[Server] SNAPSHOT_SENT tick=6789 entities=20 (player1_ack=12345)
```

#### Debug Visualization (optional)
```
Client Sequence Timeline:
[100][101][102][X][104][105][106]  ← X = packet lost
       ^                    ^
       └─ ACK(101)         └─ ACK(106) → clears all ≤106

Server Processed Sequence:
[100][101][102][  ][104][105][106]  ← Gap detected between 102-104
                    ^
                    └─ Server continues with seq=104
```

---

## 8. Comparison with Other Approaches

### 8.1 TCP (reliable stream)

| Criteria | TCP | Our UDP ACK |
|----------|-----|-------------|
| Delivery guarantee | ✅ Yes | ❌ No (acceptable) |
| Order guarantee | ✅ Yes | ❌ No (we reject old inputs) |
| Latency | ⚠️ Variable (retransmissions) | ✅ Constant and low |
| Head-of-line blocking | ❌ Yes | ✅ No |
| Overhead | ~20 bytes header | 8 bytes header |
| Congestion control | ✅ Automatic | ⚠️ Manual (if needed) |

**Verdict**: Our approach is better for real-time gameplay.

### 8.2 QUIC (UDP + reliable streams)

| Criteria | QUIC | Our UDP ACK |
|----------|------|-------------|
| Complexity | ⚠️ High (crypto, streams) | ✅ Simple |
| Latency | ✅ Low | ✅ Low |
| Multiplexed streams | ✅ Yes | ❌ No (not needed) |
| Overhead | ~30-50 bytes | 9-13 bytes |
| Adoption | 🌐 Web standard | 🎮 Gaming-specific |

**Verdict**: QUIC is overkill for our simple use case.

### 8.3 ENet (gaming library)

| Criteria | ENet | Our UDP ACK |
|----------|------|-------------|
| Reliable channels | ✅ Yes | ⚠️ Partial (implicit ACK) |
| Sequenced channels | ✅ Yes | ✅ Yes (sequence numbers) |
| Unsequenced channels | ✅ Yes | ✅ Yes (events) |
| Overhead | ~8-12 bytes | 9-13 bytes |
| External dependency | ⚠️ Yes | ✅ No (custom) |

**Verdict**: Our approach is similar to ENet but custom-tailored for R-Type.

---

## 9. Conclusion

### 9.1 Summary

The R-Type UDP ACK system is an **implicit acknowledgment system via piggybacking** that:

- ✅ Uses `sequence_number` to order and track inputs
- ✅ Includes `last_ack_sequence` in state snapshots (no dedicated packet)
- ✅ Enables packet loss and out-of-order delivery detection
- ✅ Supports client prediction and server reconciliation
- ✅ Maintains low latency and optimal bandwidth

### 9.2 Key Advantages

1. **Efficient**: No network overhead (ACK in existing snapshot)
2. **Low latency**: No blocking, independent inputs
3. **Robust**: Packet loss and timeout detection
4. **Simple**: Clear and maintainable implementation

### 9.3 Acceptable Limitations

1. **No delivery guarantee**: A lost input is lost (OK @ 60 Hz)
2. **ACK delay**: ~16-33ms depending on snapshot rate (acceptable)
3. **Out-of-order rejection**: Old inputs rejected (OK for real-time)

### 9.4 Use Cases

This system is **ideal** for:
- 🎮 Real-time action games (shooters, racing, fighting games)
- 🏃 Applications requiring low latency and high frequency
- 📊 Systems tolerant to occasional data loss

This system is **less suitable** for:
- 💬 Chat or messaging (prefer reliable channels)
- 💾 File transfers (prefer TCP)
- 🔐 Critical transactions (prefer TCP with explicit ACK)

---

## 10. References

### 10.1 Source Files

- **Protocol definition**: `src/r-type/shared/protocol/PacketHeader.hpp`
- **Payloads**: `src/r-type/shared/protocol/Payloads.hpp`
- **Client prediction**: `src/r-type/client/src/systems/ClientPredictionSystem.cpp`
- **Server network**: `src/r-type/server/src/ServerNetworkSystem.cpp`

### 10.2 Related Documentation

- [PROTOCOL.md](PROTOCOL.md) - Complete protocol specification
- [CLIENT_ARCHITECTURE.md](CLIENT_ARCHITECTURE.md) - Client architecture
- [SERVER_ARCHITECTURE.md](SERVER_ARCHITECTURE.md) - Server architecture

### 10.3 External Resources

- [Gaffer on Games - Networked Physics](https://gafferongames.com/post/networked_physics_in_virtual_reality/)
- [Valve - Latency Compensating Methods](https://developer.valvesoftware.com/wiki/Latency_Compensating_Methods_in_Client/Server_In-game_Protocol_Design_and_Optimization)
- [Glenn Fiedler - UDP vs TCP](https://gafferongames.com/post/udp_vs_tcp/)

---

**Document written by R-Type Development Team**
**Last updated**: January 2026
**Version**: 1.0