# R-Type Client Architecture

## Overview

The R-Type client is a multiplayer graphical application that connects to the game server, displays the game rendering, and sends player inputs.

## Global Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                       main.cpp                              │
│                    (Entry point)                            │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                     ClientGame                              │
│              (Game orchestration)                           │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────┐ ┌─────────┐ ┌──────────┐ ┌───────────────┐ │
│  │ Texture  │ │ Input   │ │ Screen   │ │    Entity     │ │
│  │ Manager  │ │ Handler │ │ Manager  │ │   Manager     │ │
│  └──────────┘ └─────────┘ └──────────┘ └───────────────┘ │
│                                                             │
│  ┌──────────────┐ ┌────────────┐ ┌──────────────────────┐│
│  │    Status    │ │    Chat    │ │    NetworkClient     ││
│  │   Overlay    │ │   Overlay  │ │   (Network mgmt)     ││
│  └──────────────┘ └────────────┘ └──────────────────────┘│
│                                                             │
└─────────────────────────────────────────────────────────────┘
          │                        │                    │
          ▼                        ▼                    ▼
  ┌───────────────┐       ┌──────────────┐    ┌───────────────┐
  │   Registry    │       │   Plugins    │    │  INetwork     │
  │     (ECS)     │       │ (Graphics,   │    │   Plugin      │
  │               │       │  Input, etc) │    │               │
  └───────────────┘       └──────────────┘    └───────────────┘
```

## Main Components

### 1. ClientGame (Main Class)

**File**: `src/r-type/client/include/ClientGame.hpp`

**Responsibilities**:
- Orchestration of all game components
- Plugin initialization (graphics, input, audio, network)
- ECS registry configuration
- Game systems setup
- Main game loop
- Network callback management

**Key Members**:
```cpp
class ClientGame {
private:
    // Plugins
    engine::IGraphicsPlugin* graphics_plugin_;
    engine::IInputPlugin* input_plugin_;
    engine::IAudioPlugin* audio_plugin_;
    engine::INetworkPlugin* network_plugin_;

    // ECS
    std::unique_ptr<Registry> registry_;

    // Game components
    std::unique_ptr<TextureManager> texture_manager_;
    std::unique_ptr<ScreenManager> screen_manager_;
    std::unique_ptr<EntityManager> entity_manager_;
    std::unique_ptr<StatusOverlay> status_overlay_;
    std::unique_ptr<InputHandler> input_handler_;

    // Overlays
    std::unique_ptr<ChatOverlay> chat_overlay_;
    std::unique_ptr<ConsoleOverlay> console_overlay_;

    // Network
    std::unique_ptr<NetworkClient> network_client_;
};
```

**Lifecycle**:
```
initialize()
    ├─► load_plugins()
    ├─► setup_registry()
    ├─► setup_systems()
    ├─► load_textures()
    ├─► setup_background()
    ├─► setup_network_callbacks()
    └─► connect()
        ↓
run()  [Main loop]
    ├─► update(dt)
    ├─► handle_input()
    ├─► update_projectiles()
    ├─► run_systems()
    └─► display()
        ↓
shutdown()
    ├─► disconnect()
    └─► cleanup_plugins()
```

### 2. TextureManager

**File**: `src/r-type/client/include/TextureManager.hpp`

**Responsibilities**:
- Loading all textures at startup
- Centralized management of texture handles
- Fallbacks for missing textures

**Managed Textures**:
- `background` - Scrolling background
- `menu_background` - Menu background
- `player_frames[4]` - Player animation frames
- `enemy` - Enemy sprites
- `projectile` - Projectile sprites
- `wall` - Obstacle sprites

**Main Methods**:
```cpp
bool load_all();  // Load all textures
engine::TextureHandle get_background() const;
engine::TextureHandle get_player_frame(size_t index) const;
// ... etc
```

### 3. InputHandler

**File**: `src/r-type/client/include/InputHandler.hpp`

**Responsibilities**:
- Reading player inputs
- Conversion to network flags (protocol)
- Special key detection

**Key Mapping**:
```
W / Up Arrow    → INPUT_UP
S / Down Arrow  → INPUT_DOWN
A / Left Arrow  → INPUT_LEFT
D / Right Arrow → INPUT_RIGHT
Space           → INPUT_SHOOT
Shift           → INPUT_CHARGE
Ctrl            → INPUT_SPECIAL
E               → INPUT_SWITCH_WEAPON
T               → Open chat
F1              → Close chat
Tab             → Open admin console
Escape          → Quit game
```

**Usage**:
```cpp
uint16_t flags = input_handler_->gather_input();
network_client_->send_input(flags, client_tick_);
```

### 4. ScreenManager

**File**: `src/r-type/client/include/ScreenManager.hpp`

**Responsibilities**:
- Screen state management (State Pattern)
- Screen transitions
- Overlay show/hide
- Result screen management

**Available States**:
```cpp
enum class GameScreen {
    WAITING,   // Waiting for players
    PLAYING,   // In game
    VICTORY,   // Victory
    DEFEAT     // Defeat
};
```

**Transitions**:
```
WAITING (Lobby)
    ↓
PLAYING (Game start)
    ↓
VICTORY / DEFEAT (Game over)
    ↓
(Possible return to lobby)
```

**Managed Screens**:
- Waiting screen (background + "Waiting for players..." text)
- Result screen (background + "VICTORY!" or "DEFEAT..." text)
- Status overlay (connection, lobby, ping)

### 5. StatusOverlay

**File**: `src/r-type/client/include/StatusOverlay.hpp`

**Responsibilities**:
- Connection status display
- Lobby state display
- Current session display
- Ping display

**Display Format**:
```
Connected (Player 1234) | Lobby 1: 3/4 | In game (session 5678) | Ping: 25ms
```

**Methods**:
```cpp
void set_connection(const std::string& status);
void set_lobby(const std::string& status);
void set_session(const std::string& status);
void set_ping(int ping_ms);
void refresh();  // Update display
```

### 6. ChatOverlay

**File**: `src/r-type/client/include/ui/ChatOverlay.hpp`

**Responsibilities**:
- Chat display as overlay (non-blocking)
- Message history management (scrollable)
- Message input and sending
- Unread message notifications

**Controls**:
```
T           → Open chat
F1          → Close chat
Enter       → Send message
PageUp/Down → Scroll through history
```

**Features**:
- Works in **lobby** (RoomLobbyScreen) and **in-game** (ClientGame)
- Game continues while chat is open
- Unique colors per player (based on player_id)
- Notification badge when chat is closed

**Methods**:
```cpp
void toggle();                    // Open/close chat
void set_visible(bool visible);   // Visibility control
bool is_visible() const;
void add_message(sender_id, sender_name, message);  // Add received message
void set_send_callback(callback); // Callback to send message
int get_unread_count() const;     // Number of unread messages
void clear_unread();              // Reset counter
void update(graphics, input);     // Update (input handling)
void draw(graphics);              // Rendering
void draw_notification_badge(graphics);  // Notification badge
```

### 7. EntityManager

**File**: `src/r-type/client/include/EntityManager.hpp`

**Responsibilities**:
- Complete network entity management
- Server synchronization
- Entity spawn/update/destroy
- Client-side prediction (projectiles)
- Stale entity management
- Player name tags

**Entity Tracking**:
```cpp
std::unordered_map<uint32_t, Entity> server_to_local_;      // server_id → entity
std::unordered_map<uint32_t, EntityType> server_types_;     // Entity type
std::unordered_map<uint32_t, uint8_t> stale_counters_;      // Aging counter
std::unordered_set<uint32_t> locally_integrated_;           // Projectiles (prediction)
```

**Main Methods**:
```cpp
Entity spawn_or_update_entity(server_id, type, x, y, health, subtype);
void remove_entity(server_id);
void clear_all();  // Complete cleanup
void process_snapshot_update(updated_ids);  // Stale entity detection
void update_projectiles(delta_time);  // Local prediction
void update_name_tags();  // Position update
```

**Sprite Construction**:

Each entity type has a specific sprite:
```cpp
switch (type) {
    case PLAYER:        → Animated cyan/white sprite
    case ENEMY_BASIC:   → Standard enemy sprite
    case ENEMY_FAST:    → Orange sprite
    case ENEMY_TANK:    → Red sprite
    case ENEMY_BOSS:    → Purple sprite
    case PROJECTILE:    → Cyan/red sprite
    case POWERUP:       → Green sprite
    // ...
}
```

### 8. NetworkClient

**File**: `src/r-type/client/include/NetworkClient.hpp`

**Responsibilities**:
- TCP/UDP connection management with server
- Packet sending
- Packet reception and decoding
- Network event callbacks

**Main Methods**:
```cpp
bool connect(host, tcp_port);
void disconnect();
void update();  // Process packets

// Sending
void send_connect(player_name);
void send_join_lobby(mode, difficulty);
void send_input(flags, tick);
void send_ping();
void send_chat_message(message);  // Chat

// Callbacks
void set_on_accepted(callback);
void set_on_lobby_state(callback);
void set_on_game_start(callback);
void set_on_entity_spawn(callback);
void set_on_snapshot(callback);
void set_on_chat_message(callback);  // Chat
// ... etc
```

## ECS Architecture (Entity Component System)

### Registry

The ECS registry manages all game components and entities.

**Registered Components**:
```cpp
registry->register_component<Position>();
registry->register_component<Velocity>();
registry->register_component<Sprite>();
registry->register_component<SpriteAnimation>();
registry->register_component<Collider>();
registry->register_component<Health>();
registry->register_component<Score>();
registry->register_component<Controllable>();
registry->register_component<NetworkId>();
registry->register_component<UIText>();
// ... etc
```

### Systems

**Execution Order** (each frame):
1. `ScrollingSystem` - Background scrolling
2. `SpriteAnimationSystem` - Sprite animation
3. `RenderSystem` - Graphics rendering
4. `HUDSystem` - Interface display

## Network Flow

### Server Connection

```
Client                                  Server
  │                                       │
  ├──► connect(host, tcp_port) ─────────►│
  │                                       │
  ├──► CLIENT_CONNECT ──────────────────►│
  │     (player_name)                     │
  │                                       │
  │◄──── SERVER_ACCEPT ───────────────────┤
  │      (assigned_player_id)             │
  │                                       │
  │  [Callback: on_accepted()]            │
  │                                       │
  ├──► CLIENT_JOIN_LOBBY ────────────────►│
  │                                       │
  │◄──── SERVER_LOBBY_STATE ──────────────┤
  │                                       │
```

### Game Start

```
Client                                  Server
  │                                       │
  │◄──── SERVER_GAME_START_COUNTDOWN ────┤
  │      (10, 9, 8, ...)                  │
  │                                       │
  │  [Callback: on_countdown()]           │
  │                                       │
  │◄──── SERVER_GAME_START ───────────────┤
  │      (session_id, udp_port)           │
  │                                       │
  │  [Callback: on_game_start()]          │
  │  - Clear entities                     │
  │  - Switch to PLAYING screen           │
  │                                       │
  ├══► CLIENT_UDP_HANDSHAKE ═════════════►│
  │                                       │
  │◄═══ SERVER_ENTITY_SPAWN ══════════════┤
  │     (players, enemies, walls...)      │
  │                                       │
```

**Legend**: `───►` TCP, `═══►` UDP

### Game Loop

```
Client                                  Server
  │                                       │
  │  [Every 30ms]                         │
  ├══► CLIENT_INPUT ═════════════════════►│
  │     (input_flags, client_tick)        │
  │                                       │
  │  [Every ~16ms]                        │
  │◄═══ SERVER_DELTA_SNAPSHOT ════════════┤
  │     (entity states)                   │
  │                                       │
  │  [EntityManager processes]            │
  │  - Update positions                   │
  │  - Update velocities                  │
  │  - Update health                      │
  │  - Remove stale entities              │
  │                                       │
  │◄═══ SERVER_ENTITY_SPAWN ══════════════┤
  │◄═══ SERVER_ENTITY_DESTROY ════════════┤
  │◄═══ SERVER_PROJECTILE_SPAWN ══════════┤
  │                                       │
```

### Client-Side Prediction (Projectiles)

To reduce perceived latency, projectiles are updated locally:

```cpp
// Marking during spawn
locally_integrated_.insert(projectile_id);

// Local update each frame
void EntityManager::update_projectiles(float dt) {
    for (auto projectile_id : locally_integrated_) {
        if (!in_latest_snapshot) {
            // Predictive update
            pos.x += vel.x * dt;
            pos.y += vel.y * dt;

            // Despawn if off-screen
            if (out_of_bounds) {
                remove_entity(projectile_id);
            }
        }
    }
}
```

### Game End

```
Client                                  Server
  │                                       │
  │◄═══ SERVER_GAME_OVER ═════════════════┤
  │     (result: VICTORY/DEFEAT)          │
  │                                       │
  │  [Callback: on_game_over()]           │
  │  - Show result screen                 │
  │  - Update status overlay              │
  │                                       │
```

## Stale Entity Management

To detect entities that no longer exist server-side but haven't received a DESTROY packet:

```cpp
void EntityManager::process_snapshot_update(updated_ids) {
    for (auto& [server_id, entity] : server_to_local_) {
        if (!updated_ids.contains(server_id)) {
            // Entity not updated in this snapshot
            stale_counters_[server_id]++;

            if (stale_counters_[server_id] > THRESHOLD) {
                // Removal after ~6 frames without update
                remove_entity(server_id);
            }
        } else {
            // Reset counter
            stale_counters_[server_id] = 0;
        }
    }
}
```

## Audio-Visual Synchronization

### Network Callbacks → Visual/Audio Effects

```cpp
network_client_->set_on_projectile_spawn([this](...) {
    // Visual spawn
    entity_manager_->spawn_or_update_entity(...);

    // Shot sound (if audio_plugin available)
    if (audio_plugin_) {
        audio_plugin_->play_sound("laser.wav");
    }
});

network_client_->set_on_entity_destroy([this](...) {
    // Explosion effect
    create_particle_effect(...);

    // Explosion sound
    if (audio_plugin_) {
        audio_plugin_->play_sound("explosion.wav");
    }
});
```

## Screen Resolution and Scaling

**Default Resolution**: 1920x1080

The game uses relative dimensions to adapt:
```cpp
constexpr int SCREEN_WIDTH = 1920;
constexpr int SCREEN_HEIGHT = 1080;

// Centered positions
float center_x = SCREEN_WIDTH / 2.0f;
float center_y = SCREEN_HEIGHT / 2.0f;
```

## Performance and Optimization

### Update Frequencies

- **Network (input sending)**: 30 Hz (~33ms)
- **Network (snapshot reception)**: 60 Hz (~16ms)
- **Rendering**: VSync (typically 60 FPS)
- **Overlay update**: 2 Hz (500ms)
- **Ping**: Every 5 seconds

### Optimizations

1. **Local Prediction**: Client-side projectile updates
2. **Delta Snapshots**: Only changed entities
3. **Stale Entity Removal**: Automatic cleanup
4. **Sprite Batching**: By RenderSystem
5. **Dirty Flags**: Conditional name tag updates

## Configuration

**Command Line**:
```bash
./r-type_client [host] [tcp_port] [player_name]

# Examples:
./r-type_client                           # Defaults: 127.0.0.1:4242, "Pilot"
./r-type_client 192.168.1.100             # Custom host
./r-type_client 192.168.1.100 5000        # Custom host + port
./r-type_client 192.168.1.100 5000 "Bob"  # + custom name
```

## Error Handling

### Connection Failure

```cpp
if (!network_client_->connect(host, tcp_port)) {
    std::cerr << "Failed to connect to server\n";
    return 1;
}
```

### Unexpected Disconnection

```cpp
network_client_->set_on_disconnected([this]() {
    status_overlay_->set_connection("Disconnected");
    status_overlay_->refresh();
    running_ = false;  // Stop game loop
});
```

### Server Rejection

```cpp
network_client_->set_on_rejected([this](reason, message) {
    status_overlay_->set_connection("Rejected: " + message);
    status_overlay_->refresh();
});
```

## Complete Flow Diagram

```
┌─────────┐
│  main() │
└────┬────┘
     │
     ▼
┌─────────────────────────────────────────┐
│  ClientGame::initialize()               │
├─────────────────────────────────────────┤
│  1. Load plugins                        │
│  2. Create window                       │
│  3. Setup Registry + Systems            │
│  4. Load textures (TextureManager)      │
│  5. Create managers                     │
│  6. Setup network callbacks             │
│  7. Connect to server                   │
└────┬────────────────────────────────────┘
     │
     ▼
┌─────────────────────────────────────────┐
│  ClientGame::run()                      │
│  ┌─────────────────────────────────┐   │
│  │  GAME LOOP (60 FPS)             │   │
│  ├─────────────────────────────────┤   │
│  │  1. Network update              │   │
│  │  2. Process packets             │   │
│  │  3. Handle input                │   │
│  │  4. Update projectiles          │   │
│  │  5. Update name tags            │   │
│  │  6. Run ECS systems             │   │
│  │  7. Render                      │   │
│  │  8. Check exit condition        │   │
│  └─────────────────────────────────┘   │
└────┬────────────────────────────────────┘
     │
     ▼
┌─────────────────────────────────────────┐
│  ClientGame::shutdown()                 │
├─────────────────────────────────────────┤
│  1. Disconnect from server              │
│  2. Clear entities                      │
│  3. Shutdown plugins                    │
└─────────────────────────────────────────┘
```

## Deployment

### Compilation

```bash
cmake -B build
cmake --build build --target r-type_client
```

### Required Resources

The client requires the `assets/` folder with:
- `sprite/symmetry.png` - Background
- `sprite/background_rtype_menu.png` - Menu background
- `sprite/ship1.png`, `ship2.png`, `ship3.png`, `ship4.png` - Player frames
- `sprite/enemy.png` - Enemy sprite
- `sprite/bullet.png` - Projectile sprite
- `sprite/lock.png` - Wall sprite

## Dependencies

- **game_engine**: ECS, plugins, systems
- **rtype_logic**: Shared logic (dimensions, etc.)
- **Raylib**: Graphics and input via plugins
- **Miniaudio**: Audio via plugin
- **Boost.Asio**: Network via plugin

## Conclusion

The client's modular architecture provides:
- **Separation of Concerns**: Each component has a clear role
- **Maintainability**: Organized and easy-to-modify code
- **Extensibility**: Simplified feature additions
- **Performance**: Network and rendering optimizations
- **UX**: Clear visual feedback and responsiveness
