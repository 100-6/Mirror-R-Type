# Level System

This document explains how the level system works in Mirror R-Type, including level progression, boss spawning, and level transitions.

## Overview

The level system manages:
- Level state machine (start, waves, boss, complete)
- Wave spawning based on scroll distance
- Boss spawning and defeat detection
- Level transitions and victory conditions

## Level State Machine

```
┌─────────────────┐
│  LEVEL_START    │  (0.5s intro)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│     WAVES       │  (enemy waves based on scroll)
└────────┬────────┘
         │ all waves complete + no enemies
         ▼
┌─────────────────┐
│ BOSS_TRANSITION │  (3s warning)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   BOSS_FIGHT    │  (scroll stops, boss spawns)
└────────┬────────┘
         │ boss defeated
         ▼
┌─────────────────┐
│ LEVEL_COMPLETE  │  (5s victory)
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Next Level or   │
│ FINAL VICTORY   │
└─────────────────┘
```

### State Descriptions

| State | Duration | Description |
|-------|----------|-------------|
| `LEVEL_START` | 0.5s | Brief intro, scroll begins |
| `WAVES` | Variable | Enemy waves spawn based on scroll distance |
| `BOSS_TRANSITION` | 3s | "WARNING: BOSS APPROACHING!" message |
| `BOSS_FIGHT` | Variable | Scroll stops, boss spawns and fights |
| `LEVEL_COMPLETE` | 5s | Victory animation, score display |
| `GAME_OVER` | - | All players dead or final victory |

---

## Scroll System

The game scrolls automatically at a constant speed defined per level.

### Scroll Configuration

```json
{
    "level_id": 1,
    "base_scroll_speed": 60.0,
    "total_scroll_distance": 7000.0
}
```

| Parameter | Description | Example |
|-----------|-------------|---------|
| `base_scroll_speed` | Pixels per second | 60.0 |
| `total_scroll_distance` | Total level length | 7000.0 |

### When Does Scroll Stop?

**Scroll stops when the boss spawns** (state = `BOSS_FIGHT`):
- Camera velocity is set to 0
- Boss appears at configured spawn position
- Players fight boss in fixed arena

---

## Wave Spawning

Waves spawn based on scroll distance, not time.

### Wave Configuration

```json
{
    "wave_number": 1,
    "trigger": {
        "scrollDistance": 500,
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
            "spacing": 100
        }
    ]
}
```

### Trigger Conditions

| Field | Description |
|-------|-------------|
| `scrollDistance` | Scroll position (pixels) when wave spawns |
| `timeDelay` | Additional delay after scroll trigger (seconds) |

### Spawn Patterns

| Pattern | Description |
|---------|-------------|
| `single` | Single entity |
| `line` | Horizontal line with spacing |
| `formation` | Custom formation |

---

## Boss Spawning

### When Does the Boss Spawn?

1. **All waves triggered** (`all_waves_triggered = true`)
2. **No enemies remaining** (all wave enemies defeated)
3. **State transitions** to `BOSS_TRANSITION` (3s warning)
4. **State transitions** to `BOSS_FIGHT`
5. **Boss spawns** at configured position
6. **Scroll stops**

### Boss Configuration

```json
"boss": {
    "boss_name": "Mars Guardian",
    "spawn_scroll_distance": 6500.0,
    "spawn_position_x": 1600.0,
    "spawn_position_y": 540.0,
    "script_path": "boss/boss1_mars_guardian.lua",
    "total_phases": 3
}
```

| Field | Description |
|-------|-------------|
| `boss_name` | Display name |
| `spawn_scroll_distance` | Minimum scroll before boss can spawn |
| `spawn_position_x` | X position where boss appears |
| `spawn_position_y` | Y position where boss appears |
| `script_path` | Lua script for boss behavior |
| `total_phases` | Number of boss phases (usually 3) |

### Boss Defeat Detection

Boss is considered defeated when:
- Boss entity no longer exists in registry, OR
- Boss health reaches 0

---

## Level Transitions

### Next Level Loading

When a level is completed:

1. **Wait 5 seconds** (victory animation)
2. **Check level number:**
   - Level < 3: Load next level
   - Level >= 3: Final victory
3. **Clear all enemies and projectiles**
4. **Reset level controller**
5. **Transition to `LEVEL_START`**

### Victory Conditions

| Condition | Result |
|-----------|--------|
| Level 1 complete | Load Level 2 |
| Level 2 complete | Load Level 3 |
| Level 3 complete | **FINAL VICTORY** |

---

## Level Configuration Files

### File Location

```
assets/levels/
├── level_1_mars_assault.json
├── level_2_nebula_station.json
├── level_3_uranus_station.json
└── level_4_jupiter_orbit.json
```

### Full Level Structure

```json
{
    "level_id": 1,
    "level_name": "Mars Assault",
    "level_description": "Storm the Martian defense installations",
    "map_id": 1,
    "base_scroll_speed": 60.0,
    "total_scroll_distance": 7000.0,

    "phases": [
        {
            "phase_number": 1,
            "phase_name": "Red Planet Entry",
            "scroll_start": 0.0,
            "scroll_end": 2000.0,
            "difficulty": "easy",
            "waves": [...]
        },
        {
            "phase_number": 2,
            "phase_name": "Industrial Zone",
            "scroll_start": 2000.0,
            "scroll_end": 4500.0,
            "difficulty": "medium",
            "waves": [...]
        },
        {
            "phase_number": 3,
            "phase_name": "Final Approach",
            "scroll_start": 4500.0,
            "scroll_end": 6500.0,
            "difficulty": "hard",
            "waves": [...]
        }
    ],

    "boss": {
        "boss_name": "Mars Guardian",
        "spawn_scroll_distance": 6500.0,
        "spawn_position_x": 1600.0,
        "spawn_position_y": 540.0,
        "script_path": "boss/boss1_mars_guardian.lua",
        "total_phases": 3
    }
}
```

---

## Level Summary

### Level 1: Mars Assault

| Property | Value |
|----------|-------|
| Scroll Speed | 60 px/s |
| Total Distance | 7000 px |
| Boss Spawn | 6500 px |
| Boss | Mars Guardian |
| Difficulty | Easy → Medium → Hard |

### Level 2: Nebula Station

| Property | Value |
|----------|-------|
| Scroll Speed | 65 px/s |
| Total Distance | 9000 px |
| Boss Spawn | 8500 px |
| Boss | Nebula Core |
| Difficulty | Easy → Medium → Hard |

### Level 3: Uranus Station

| Property | Value |
|----------|-------|
| Scroll Speed | 60 px/s |
| Total Distance | 8000 px |
| Boss Spawn | 7500 px |
| Boss | Uranus Sentinel |
| Difficulty | Medium → Hard → Very Hard |

### Level 4: Jupiter Orbit

| Property | Value |
|----------|-------|
| Scroll Speed | 60 px/s |
| Total Distance | 9000 px |
| Boss Spawn | 8500 px |
| Boss | Jupiter Titan |
| Difficulty | Medium → Hard → Extreme |

---

## Timing Calculations

### Example: Level 1

```
Scroll speed: 60 px/s
Boss spawn distance: 6500 px

Time to boss: 6500 / 60 = ~108 seconds (~1m 48s)
+ Boss transition: 3s
+ Boss fight: Variable (depends on player skill)
+ Victory: 5s
```

### Estimated Level Durations

| Level | To Boss | Boss Fight | Total (approx) |
|-------|---------|------------|----------------|
| 1 | ~1m 48s | ~1-2 min | ~3-4 min |
| 2 | ~2m 10s | ~1-2 min | ~3-5 min |
| 3 | ~2m 5s | ~1-2 min | ~3-5 min |
| 4 | ~2m 22s | ~2-3 min | ~4-6 min |

---

## Technical Implementation

### Key Components

| Component | Description |
|-----------|-------------|
| `LevelController` | Stores current level state, boss entity, etc. |
| `ScrollState` | Current scroll position |
| `BossScript` | Lua script path for boss behavior |

### Key Systems

| System | Responsibility |
|--------|----------------|
| `LevelSystem` | State machine, transitions |
| `LuaSystem` | Enemy and boss AI scripts |
| `WaveManager` | Wave spawning based on scroll |

### GameSession Integration

The `GameSession` class coordinates:
- Scroll updates (`current_scroll_`)
- Wave manager updates
- Boss spawning when `BOSS_FIGHT` state begins
- Level loading when `LEVEL_COMPLETE` ends

---

## Debug Information

Console logs show level progression:

```
[LevelSystem] Transition: LEVEL_START
[LevelSystem] Transition: WAVES
[LevelSystem] Waves started for level 1
[LevelSystem] All waves complete and no enemies remaining - starting boss transition
[LevelSystem] Transition: BOSS_TRANSITION
[LevelSystem] Boss transition - WARNING: BOSS APPROACHING!
[LevelSystem] Transition: BOSS_FIGHT
[LevelSystem] Boss fight started!
[GameSession] Scroll stopped for boss fight
[GameSession] Boss spawned: Mars Guardian (script: boss/boss1_mars_guardian.lua)
[LuaSystem] Loaded boss script: boss/boss1_mars_guardian.lua
[LuaSystem] Boss phase transition: 0 -> 1
[LuaSystem] Boss phase transition: 1 -> 2
[LevelSystem] Transition: LEVEL_COMPLETE
[LevelSystem] Level 1 completed!
[LevelSystem] Loading level 2...
```
