# Wave Configuration Guide - 2-Chunks Pattern

This guide explains how to configure waves (enemy waves) for new R-Type game maps.

## Table of Contents

1. [Overview](#overview)
2. [Chunk System](#chunk-system)
3. [Standard Pattern: 2-Chunks](#standard-pattern-2-chunks)
4. [JSON Wave Structure](#json-wave-structure)
5. [Enemy Composition by Difficulty](#enemy-composition-by-difficulty)
6. [Boss Configuration](#boss-configuration)
7. [Complete Examples](#complete-examples)
8. [Validation](#validation)

---

## Overview

The R-Type wave system is based on **chunk**-based triggering (map pieces) rather than absolute scroll distance.

### Key Concepts

- **Chunk**: Basic map unit (1 chunk = 480 pixels = 30 tiles × 16px)
- **ChunkId**: Chunk position on the map (0-indexed)
- **Offset**: Relative position within chunk (0.0 = start, 1.0 = end)
- **Wave**: Enemy wave triggered at a specific chunk position
- **Phase**: Group of waves with consistent difficulty (easy/medium/hard)

---

## Chunk System

### Dimensions

```
1 chunk = 480 pixels
1 tile = 16 pixels
1 chunk = 30 tiles
```

### Position Calculation

To calculate scroll distance for a chunk:

```
scroll_distance = chunkId × 480
```

**Examples:**
- Chunk 0: 0 pixels
- Chunk 2: 960 pixels
- Chunk 10: 4800 pixels
- Chunk 20: 9600 pixels (standard level end)

### ChunkId + Offset

A wave trigger uses `chunkId` (integer) + `offset` (decimal):

```json
{
  "chunkId": 4,
  "offset": 0.0
}
```

- `offset: 0.0` → Chunk start (exact position = 4 × 480 = 1920px)
- `offset: 0.5` → Chunk middle (exact position = 1920 + 240 = 2160px)
- `offset: 1.0` → Chunk end (exact position = 1920 + 480 = 2400px)

---

## Standard Pattern: 2-Chunks

### Official Rule

**Each level must have waves spaced 2 chunks apart.**

### Standard Configuration (20 total chunks)

For a level with 20 chunks:

| Wave | ChunkId | Scroll Distance | Position |
|------|---------|-----------------|----------|
| 1    | 0       | 0 px            | Start    |
| 2    | 2       | 960 px          |          |
| 3    | 4       | 1920 px         |          |
| 4    | 6       | 2880 px         |          |
| 5    | 8       | 3840 px         |          |
| 6    | 10      | 4800 px         |          |
| 7    | 12      | 5760 px         |          |
| 8    | 14      | 6720 px         |          |
| 9    | 16      | 7680 px         |          |
| 10   | 18      | 8640 px         | Last     |
| Boss | N/A     | 9600 px         | After chunk 20 |

### 2-Chunks Pattern Advantages

✓ Regular and predictable rhythm
✓ 10 waves per level = good gameplay balance
✓ Constant preparation time between waves
✓ Easy to test and debug

---

## JSON Wave Structure

### Wave Template

```json
{
  "wave_number": 1,
  "trigger": {
    "chunkId": 0,
    "offset": 0.0,
    "timeDelay": 0
  },
  "spawns": [
    {
      "type": "enemy",
      "enemyType": "basic",
      "positionX": 2100,
      "positionY": 300,
      "count": 3,
      "pattern": "line",
      "spacing": 120
    },
    {
      "type": "powerup",
      "bonusType": "health",
      "positionX": 2300,
      "positionY": 540,
      "count": 1,
      "pattern": "single"
    }
  ]
}
```

### Required Fields

#### wave_number (integer)
- Sequential wave number (1, 2, 3...)
- **IMPORTANT**: Must be unique and sequential
- Used for server/client tracking

#### trigger (object)
- **chunkId** (integer): Chunk position (0-19 for 20 chunks)
- **offset** (float): Position within chunk (0.0-1.0)
- **timeDelay** (integer): Additional delay in seconds (usually 0)

#### spawns (array)
- List of entities to spawn in this wave
- Can contain enemies and powerups
- At least 1 spawn required

### Spawn Types

#### Enemy Spawn
```json
{
  "type": "enemy",
  "enemyType": "basic|zigzag|bouncer|tank|kamikaze",
  "positionX": 2100,
  "positionY": 300,
  "count": 3,
  "pattern": "line|grid|formation|random|single",
  "spacing": 120
}
```

**Available Enemy Types:**
- `basic`: Standard enemy, linear movement
- `zigzag`: Vertical zigzag movement
- `bouncer`: Bounces off screen edges
- `tank`: Slow but resistant enemy
- `kamikaze`: Rushes toward player

**Spawn Patterns:**
- `single`: Single enemy (ignores count/spacing)
- `line`: Vertical line of enemies
- `grid`: 2D grid (requires horizontal/vertical spacing)
- `formation`: Predefined formation
- `random`: Random positions in an area

#### Powerup Spawn
```json
{
  "type": "powerup",
  "bonusType": "health|shield|speed|firepower",
  "positionX": 2300,
  "positionY": 540,
  "count": 1,
  "pattern": "single"
}
```

**Bonus Types:**
- `health`: Restores health
- `shield`: Temporary shield
- `speed`: Increases speed
- `firepower`: Improves shots

---

## Enemy Composition by Difficulty

### Phase 1: Easy (Waves 1-3, Chunks 0-4)

**Objective**: Progressive introduction, low threat

**Recommendations:**
- Mostly `basic` (60-80%)
- Some `zigzag` for variety (20-30%)
- 1 `health` powerup in wave 3

**Example Wave 1:**
```json
{
  "wave_number": 1,
  "trigger": {"chunkId": 0, "offset": 0.0, "timeDelay": 0},
  "spawns": [
    {
      "type": "enemy",
      "enemyType": "basic",
      "positionX": 2100,
      "positionY": 300,
      "count": 3,
      "pattern": "line",
      "spacing": 150
    }
  ]
}
```

### Phase 2: Medium (Waves 4-7, Chunks 6-12)

**Objective**: Difficulty increase, introduction of advanced enemies

**Recommendations:**
- Mix `zigzag`, `bouncer`, `tank`
- First appearances of `kamikaze`
- Strategic powerups (`shield`, `speed`)
- Multiple spawns in one wave (2-3 groups)

**Example Wave 5:**
```json
{
  "wave_number": 5,
  "trigger": {"chunkId": 8, "offset": 0.0, "timeDelay": 0},
  "spawns": [
    {
      "type": "enemy",
      "enemyType": "tank",
      "positionX": 2100,
      "positionY": 250,
      "count": 2,
      "pattern": "line",
      "spacing": 280
    },
    {
      "type": "enemy",
      "enemyType": "bouncer",
      "positionX": 2200,
      "positionY": 600,
      "count": 3,
      "pattern": "line",
      "spacing": 130
    },
    {
      "type": "powerup",
      "bonusType": "shield",
      "positionX": 2300,
      "positionY": 450,
      "count": 1,
      "pattern": "single"
    }
  ]
}
```

### Phase 3: Hard (Waves 8-10, Chunks 14-18)

**Objective**: Maximum difficulty, boss preparation

**Recommendations:**
- Many `tank`, `kamikaze`, `bouncer`
- Multiple and simultaneous spawns (3-4 groups)
- Rare but powerful powerups
- Last `health` powerup in wave 10 (before boss)

**Example Wave 10:**
```json
{
  "wave_number": 10,
  "trigger": {"chunkId": 18, "offset": 0.0, "timeDelay": 0},
  "spawns": [
    {
      "type": "enemy",
      "enemyType": "tank",
      "positionX": 2100,
      "positionY": 300,
      "count": 3,
      "pattern": "line",
      "spacing": 180
    },
    {
      "type": "enemy",
      "enemyType": "bouncer",
      "positionX": 2200,
      "positionY": 150,
      "count": 4,
      "pattern": "line",
      "spacing": 140
    },
    {
      "type": "enemy",
      "enemyType": "kamikaze",
      "positionX": 2300,
      "positionY": 750,
      "count": 3,
      "pattern": "line",
      "spacing": 120
    },
    {
      "type": "powerup",
      "bonusType": "health",
      "positionX": 2400,
      "positionY": 540,
      "count": 1,
      "pattern": "single"
    }
  ]
}
```

---

## Boss Configuration

### Standard Spawn Distance

**The boss must always spawn after chunk 20:**

```json
"boss": {
  "boss_name": "Boss Name",
  "spawn_scroll_distance": 9600.0,
  "spawn_position_x": 1600.0,
  "spawn_position_y": 540.0,
  "enemy_type": "boss",
  "total_phases": 3,
  "script_path": "boss/boss_script.lua",
  "phases": [...]
}
```

### spawn_scroll_distance Calculation

```
spawn_scroll_distance = (total_chunks) × 480
                      = 20 × 480
                      = 9600.0 pixels
```

### Recommended Spawn Position

- **spawn_position_x**: 1600.0 (center-right of screen)
- **spawn_position_y**: 540.0 (vertical center, 1920×1080 screen)

### Boss Phases

Each boss generally has **3 phases** based on health threshold:

```json
"phases": [
  {
    "phase_number": 1,
    "health_threshold": 1.0,     // 100% → 66% HP
    "movement_pattern": "vertical_sine",
    "movement_speed_multiplier": 1.0,
    "attack_patterns": [...]
  },
  {
    "phase_number": 2,
    "health_threshold": 0.66,    // 66% → 33% HP
    "movement_pattern": "figure_eight",
    "movement_speed_multiplier": 1.3,
    "attack_patterns": [...]
  },
  {
    "phase_number": 3,
    "health_threshold": 0.33,    // 33% → 0% HP
    "movement_pattern": "chase_player",
    "movement_speed_multiplier": 1.6,
    "attack_patterns": [...]
  }
]
```

---

## Complete Examples

### Example: Minimal Level Structure

```json
{
  "level_id": 3,
  "level_name": "Level Name",
  "level_description": "Short description",
  "map_id": 3,
  "base_scroll_speed": 70.0,
  "total_chunks": 20,
  "phases": [
    {
      "phase_number": 1,
      "phase_name": "Easy Phase",
      "scroll_start": 0.0,
      "scroll_end": 2400.0,
      "difficulty": "easy",
      "waves": [
        {
          "wave_number": 1,
          "trigger": {"chunkId": 0, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 2,
          "trigger": {"chunkId": 2, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 3,
          "trigger": {"chunkId": 4, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        }
      ]
    },
    {
      "phase_number": 2,
      "phase_name": "Medium Phase",
      "scroll_start": 2400.0,
      "scroll_end": 6240.0,
      "difficulty": "medium",
      "waves": [
        {
          "wave_number": 4,
          "trigger": {"chunkId": 6, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 5,
          "trigger": {"chunkId": 8, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 6,
          "trigger": {"chunkId": 10, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 7,
          "trigger": {"chunkId": 12, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        }
      ]
    },
    {
      "phase_number": 3,
      "phase_name": "Hard Phase",
      "scroll_start": 6240.0,
      "scroll_end": 9600.0,
      "difficulty": "hard",
      "waves": [
        {
          "wave_number": 8,
          "trigger": {"chunkId": 14, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 9,
          "trigger": {"chunkId": 16, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 10,
          "trigger": {"chunkId": 18, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        }
      ]
    }
  ],
  "boss": {
    "boss_name": "Boss Name",
    "spawn_scroll_distance": 9600.0,
    "spawn_position_x": 1600.0,
    "spawn_position_y": 540.0,
    "enemy_type": "boss",
    "total_phases": 3,
    "script_path": "boss/boss_script.lua",
    "phases": [...]
  }
}
```

### Checklist for New Level

- [ ] Unique `level_id`
- [ ] `total_chunks: 20`
- [ ] Appropriate `base_scroll_speed` (50-80)
- [ ] 3 phases (easy/medium/hard)
- [ ] Exactly 10 waves (chunkIds: 0,2,4,6,8,10,12,14,16,18)
- [ ] Sequential `wave_number` (1-10)
- [ ] Strategically placed powerups (minimum 1 per phase)
- [ ] Boss at `spawn_scroll_distance: 9600.0`
- [ ] Lua boss script exists in `assets/scripts/boss/`

---

## Validation

### JSON Syntax Validation

Before testing in game, validate JSON syntax:

```bash
python3 -m json.tool src/r-type/assets/levels/level_X.json > /dev/null
```

If no errors, the file is valid.

### Logic Validation

**Check:**
1. ChunkIds in ascending order (0,2,4,6...)
2. All offsets at 0.0 (strict pattern)
3. Sequential wave numbers (1,2,3...)
4. EnemyTypes exist (basic, zigzag, bouncer, tank, kamikaze)
5. Powerup in at least 1 wave per phase
6. Boss spawns after last wave

### In-Game Testing

```bash
# Build the game
cmake --build build

# Launch server
./build/bin/r-type_server

# Launch client
./build/bin/r-type_client

# Select level to test
```

**Test points:**
- ✓ HUD displays "WAVE 1 / 10" immediately
- ✓ Wave 1 triggers at chunk 0 (~0px)
- ✓ Wave 2 triggers at chunk 2 (~960px)
- ✓ Waves continue every 2 chunks
- ✓ Boss spawns after wave 10 completed
- ✓ No errors in server/client logs

---

## Rules Summary

### 2-Chunks Pattern (Absolute Rule)
```
Waves at chunks: 0, 2, 4, 6, 8, 10, 12, 14, 16, 18
Total waves: 10
Boss spawn: 9600.0 (chunk 20)
```

### Distribution by Phase
```
Phase 1 (Easy):   Waves 1-3  (chunks 0-4)
Phase 2 (Medium): Waves 4-7  (chunks 6-12)
Phase 3 (Hard):   Waves 8-10 (chunks 14-18)
```

### Enemies by Difficulty
```
Easy:   basic (70%) + zigzag (30%)
Medium: zigzag, bouncer, tank, kamikaze (mix)
Hard:   tank, kamikaze, bouncer (priority)
```

### Powerups
```
Phase 1: health (wave 3)
Phase 2: shield or speed (wave 5 or 7)
Phase 3: health (wave 10, before boss)
```

---

## References

- [LEVEL_SYSTEM.md](../../../docs/LEVEL_SYSTEM.md): Complete level system documentation
- [WAVE_SYSTEM.md](../../../docs/WAVE_SYSTEM.md): Wave system documentation
- [LUA_SCRIPTING.md](../../../docs/LUA_SCRIPTING.md): Lua scripting guide for enemies/bosses

---

**Last updated**: 2025-01-17
**Pattern version**: 2-Chunks (v2.0)
