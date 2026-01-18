# Wave System Documentation

## Overview

The Wave System allows automatic spawning of entities (enemies, walls, obstacles, powerups) based on scrolling progression and configured via JSON files.

## Architecture

### Components

The following components have been added in [`GameComponents.hpp`](src/r-type/game-logic/include/components/GameComponents.hpp):

- `WaveSpawnData`: Data for spawning an entity
- `WaveTrigger`: Wave trigger conditions
- `WaveController`: Global wave system controller
- `SpawnPattern`: Enum for spawn patterns
- `EntitySpawnType`: Enum for entity types

### Configuration

Configuration constants are defined in [`WaveConfig.hpp`](src/r-type/game-logic/include/components/WaveConfig.hpp):

```cpp
#define WAVE_DEFAULT_SPAWN_INTERVAL     2.0f    // Default interval between spawns
#define WAVE_DEFAULT_BETWEEN_WAVES      5.0f    // Time between waves
#define WAVE_SPAWN_OFFSET_X             50.0f   // Spawn offset to the right
#define WAVE_SPAWN_MIN_Y                50.0f   // Minimum Y position
#define WAVE_SPAWN_MAX_Y                1030.0f // Maximum Y position
// ... and more
```

### Systems

- **WaveSpawnerSystem**: Main system that manages wave spawning
- **WaveConfigLoader**: Utility for loading and validating JSON configurations

## JSON Configuration Format

### Basic Structure

```json
{
  "defaultSpawnInterval": 2.0,
  "loopWaves": false,
  "waves": [...]
}
```

### Wave Format

```json
{
  "trigger": {
    "chunkId": 1,        // Map chunk index (0-based, 1 chunk = 480px)
    "offset": 0.5,       // Position within chunk (0.0 - 1.0)
    "timeDelay": 2.0     // Delay after trigger (in seconds)
  },
  "spawns": [...]
}
```

### Spawn Format

```json
{
  "type": "enemy",           // Type: "enemy", "wall", "obstacle", "powerup"
  "enemyType": "basic",      // For enemies: "basic", "fast", "tank", "boss"
  "bonusType": "health",     // For powerups: "health", "shield", "speed"
  "positionX": 1920,         // Absolute X position
  "positionY": 300,          // Absolute Y position
  "count": 3,                // Number of entities to spawn
  "pattern": "line",         // Pattern: "single", "line", "grid", "random", "formation"
  "spacing": 150             // Spacing between entities (for patterns)
}
```

## Spawn Patterns

### SINGLE
Spawns a single entity at the specified position.

```json
{
  "pattern": "single",
  "count": 1
}
```

### LINE
Spawns entities in a vertical line.

```json
{
  "pattern": "line",
  "count": 5,
  "spacing": 100  // Vertical spacing
}
```

### GRID
Spawns entities in a grid.

```json
{
  "pattern": "grid",
  "count": 9,
  "spacing": 80  // Vertical spacing
}
```

### RANDOM
Spawns entities at random Y positions.

```json
{
  "pattern": "random",
  "count": 4
}
```

### FORMATION
Spawns entities in a V formation.

```json
{
  "pattern": "formation",
  "count": 6,
  "spacing": 80
}
```

## Usage Example

### 1. Code Integration

```cpp
#include "systems/WaveSpawnerSystem.hpp"

// In your initialization function
auto waveSystem = std::make_unique<WaveSpawnerSystem>(graphics);
registry.register_system(std::move(waveSystem));

// Load configuration
auto* waveSpawner = registry.get_system<WaveSpawnerSystem>();
if (waveSpawner) {
    waveSpawner->loadWaveConfiguration("assets/waves_example.json");
}
```

### 2. Simple Configuration Example

See [`waves_simple.json`](src/r-type/assets/waves_simple.json):

```json
{
  "defaultSpawnInterval": 2.0,
  "loopWaves": false,
  "waves": [
    {
      "trigger": {
        "chunkId": 0,
        "offset": 0.0,
        "timeDelay": 0
      },
      "spawns": [
        {
          "type": "enemy",
          "enemyType": "basic",
          "positionX": 1920,
          "positionY": 300,
          "count": 1,
          "pattern": "single",
          "spacing": 0
        }
      ]
    }
  ]
}
```

### 3. Advanced Configuration Example

See [`waves_example.json`](src/r-type/assets/waves_example.json) for a complete example with:
- 7 different waves
- Multiple spawn patterns
- Combinations of enemies and walls
- Final boss

## Entity Configuration

### Enemies

Enemy statistics are defined in [`CombatConfig.hpp`](src/r-type/game-logic/include/components/CombatConfig.hpp):

- **BASIC**: Health 30, Speed 100, Cooldown 2.0s
- **FAST**: Health 20, Speed 200, Cooldown 1.5s
- **TANK**: Health 100, Speed 50, Cooldown 3.0s
- **BOSS**: Health 500, Speed 80, Cooldown 0.8s

### Walls & Obstacles

Configured in [`WaveConfig.hpp`](src/r-type/game-logic/include/components/WaveConfig.hpp):

- **Walls**: Health 100, Size 50x100
- **Obstacles**: Health 50, Size 50x100

### Powerups (Bonuses)

Three types of powerups are available:

- **health** (Green): +20 HP to player
- **shield** (Purple): Protection from 1 hit (purple circle around player)
- **speed** (Blue): +50% speed for 20 seconds

Powerup spawn example:

```json
{
  "type": "powerup",
  "bonusType": "health",
  "positionX": 1920,
  "positionY": 500,
  "count": 1,
  "pattern": "single"
}
```

## Tips & Best Practices

### 1. Wave Planning

- Start with simple waves (SINGLE, LINE)
- Progressively increase difficulty
- Use `chunkId` to position waves relative to scenery (1 chunk = 30 tiles)
- Use `timeDelay` to create suspense before bosses

### 2. Spawn Patterns

- **LINE**: Perfect for defensive formations
- **GRID**: Good for creating enemy walls
- **RANDOM**: Adds unpredictability
- **FORMATION**: Ideal for organized squadrons

### 3. Entity Combinations

Combine different types in the same wave:

```json
{
  "spawns": [
    {
      "type": "wall",
      "positionX": 1920,
      "positionY": 200,
      "count": 2,
      "pattern": "line",
      "spacing": 400
    },
    {
      "type": "enemy",
      "enemyType": "tank",
      "positionX": 2000,
      "positionY": 400,
      "count": 1,
      "pattern": "single"
    }
  ]
}
```

### 4. Testing

Use [`waves_simple.json`](src/r-type/assets/waves_simple.json) to test:
- Verify spawns work
- Adjust positions
- Test patterns

## System Limits

Defined in [`WaveConfig.hpp`](src/r-type/game-logic/include/components/WaveConfig.hpp):

- `WAVE_MAX_ACTIVE_WAVES`: Maximum 10 waves
- `WAVE_MAX_ENTITIES_PER_WAVE`: Maximum 50 entities per wave
- `WAVE_MIN_SPAWN_INTERVAL`: Minimum 0.5s between spawns
- `WAVE_MAX_SPAWN_INTERVAL`: Maximum 10.0s between spawns
- `WAVE_SPAWN_MIN_Y`: Minimum Y position 50px
- `WAVE_SPAWN_MAX_Y`: Maximum Y position 1030px

## Troubleshooting

### Waves Don't Trigger

- Check that scrolling system is active
- Verify `chunkId` and `offset` are correct (0-based)
- Look at console logs for debug messages

### Entities Spawn at Wrong Location

- Check `positionX` and `positionY`
- Verify Y positions are between `WAVE_SPAWN_MIN_Y` and `WAVE_SPAWN_MAX_Y`
- Adjust `spacing` for patterns

### JSON Won't Load

- Check JSON syntax (commas, quotes, braces)
- Verify file path
- Look at error messages in console

## Files Structure

```
src/r-type/game-logic/
├── include/
│   ├── components/
│   │   ├── GameComponents.hpp     # Wave components definitions
│   │   └── WaveConfig.hpp         # Wave configuration defines
│   ├── systems/
│   │   └── WaveSpawnerSystem.hpp  # Wave spawner system header
│   └── utils/
│       └── WaveConfigLoader.hpp   # JSON loader header
└── src/
    ├── systems/
    │   └── WaveSpawnerSystem.cpp  # Wave spawner implementation
    └── utils/
        └── WaveConfigLoader.cpp   # JSON loader implementation

src/r-type/assets/
├── waves_example.json             # Complex example
└── waves_simple.json              # Simple example for testing
```

## Future Improvements

- [x] Support for powerups (health, shield, speed)
- [ ] More complex spawn patterns (circle, spiral)
- [ ] Scriptable events (music change, visual effects)
- [ ] Multiple trigger conditions (time AND distance)
- [ ] Support for spawn animations
- [ ] Visual wave editor
