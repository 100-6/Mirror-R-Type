# Lua Scripting System

This document explains how to create and configure Lua scripts for enemies and bosses in Mirror R-Type.

## Overview

The game uses Lua scripts to define AI behavior for enemies and bosses. Scripts are located in `assets/scripts/ai/` and are loaded dynamically at runtime by the `LuaSystem`.

## Directory Structure

```
assets/scripts/ai/
├── basic.lua           # Basic enemy: simple leftward movement
├── zigzag.lua          # Zigzag pattern movement
├── bouncer.lua         # Bounces off screen edges
├── kamikaze.lua        # Charges toward player
└── boss/
    ├── boss1_mars_guardian.lua
    ├── boss2_nebula_core.lua
    ├── boss3_uranus_sentinel.lua
    └── boss4_jupiter_titan.lua
```

---

## Enemy Scripts

### Script Signature

Enemy scripts must return a function with this signature:

```lua
return function(dt, pos, vel)
    -- dt: delta time (seconds)
    -- pos: position table {x, y}
    -- vel: velocity table {x, y}
end
```

### Available Global Functions

| Function | Description | Returns |
|----------|-------------|---------|
| `find_nearest_player(x, y)` | Find the closest player to position (x, y) | `targetX, targetY, found` |

### Example: Basic Enemy

```lua
-- Simple leftward movement
return function(dt, pos, vel)
    local speed = 100

    vel.x = -speed
    vel.y = 0

    pos.x = pos.x + vel.x * dt
    pos.y = pos.y + vel.y * dt
end
```

### Example: Kamikaze Enemy

```lua
-- Charges toward player when close
return function(dt, pos, vel)
    local chargeSpeed = 350
    local chargeDistance = 600

    local targetX, targetY, found = find_nearest_player(pos.x, pos.y)

    if found then
        local dx = targetX - pos.x
        local dy = targetY - pos.y
        local dist = math.sqrt(dx*dx + dy*dy)

        local speed = dist < chargeDistance and chargeSpeed or 60

        if dist > 1.0 then
            vel.x = (dx / dist) * speed
            vel.y = (dy / dist) * speed
        end
    else
        vel.x = -chargeSpeed * 0.5
        vel.y = 0
    end

    pos.x = pos.x + vel.x * dt
    pos.y = pos.y + vel.y * dt
end
```

### Example: Zigzag Enemy

```lua
-- Changes vertical direction based on horizontal distance
return function(dt, pos, vel)
    local speedX = 90
    local speedY = 70
    local zigzagWidth = 180

    local segment = math.floor(pos.x / zigzagWidth)

    if segment % 2 == 0 then
        vel.y = speedY
    else
        vel.y = -speedY
    end

    vel.x = -speedX

    pos.x = pos.x + vel.x * dt
    pos.y = pos.y + vel.y * dt

    -- Clamp Y position
    if pos.y < 80 then pos.y = 80 end
    if pos.y > 1000 then pos.y = 1000 end
end
```

### Enemy Types and Scripts

| Enemy Type | Script File | Behavior |
|------------|-------------|----------|
| `basic` | `basic.lua` | Simple leftward movement |
| `zigzag` | `zigzag.lua` | Zigzag pattern |
| `bouncer` | `bouncer.lua` | Bounces off screen edges |
| `kamikaze` | `kamikaze.lua` | Charges toward nearest player |
| `tank` | `basic.lua` | Slow movement, high HP |
| `fast` | `basic.lua` | Fast movement, low HP |

---

## Boss Scripts

### Script Signature

Boss scripts receive additional state information:

```lua
return function(dt, pos, vel, boss_state)
    -- dt: delta time (seconds)
    -- pos: position table {x, y}
    -- vel: velocity table {x, y}
    -- boss_state: table with boss info
    --   .phase: current phase (0, 1, or 2)
    --   .phase_timer: time in current phase
    --   .attack_timer: time since last attack
    --   .health_percent: current HP percentage (0.0-1.0)

    -- Return true if an attack was performed (resets attack_timer)
    return false
end
```

### Phase System

Bosses automatically transition between 3 phases based on health:

| Phase | Health Range | Behavior |
|-------|--------------|----------|
| 0 | 100% - 67% | Phase 1: Easiest patterns |
| 1 | 66% - 34% | Phase 2: Medium difficulty |
| 2 | 33% - 0% | Phase 3: Hardest patterns |

Phase transitions are handled automatically by `LuaSystem` - the script just needs to check `boss_state.phase`.

### Attack Pattern Functions

| Function | Parameters | Description |
|----------|------------|-------------|
| `spawn_boss_projectile(x, y, vx, vy, damage)` | Position, velocity, damage | Spawn single projectile |
| `spawn_pattern_360(x, y, count, speed, damage)` | Position, projectile count, speed, damage | 360-degree spray |
| `spawn_pattern_aimed(x, y, count, speed, damage, spread)` | Position, count, speed, damage, spread angle (degrees) | Aimed burst at nearest player |
| `spawn_pattern_spiral(x, y, count, speed, damage, rotation)` | Position, count, speed, damage, rotation offset (degrees) | Rotating spiral pattern |
| `spawn_pattern_random(x, y, count, speed, damage)` | Position, count, speed, damage | Random direction barrage |

### Example: Boss Script

```lua
-- Boss with 3 phases
local ATTACK_COOLDOWNS = {2.5, 1.8, 1.2}  -- Per phase
local MIN_X, MAX_X = 1200.0, 1800.0
local MIN_Y, MAX_Y = 100.0, 980.0

return function(dt, pos, vel, boss_state)
    local phase = boss_state.phase
    local timer = boss_state.phase_timer

    -- Movement based on phase
    if phase == 0 then
        -- Phase 1: Slow vertical sine
        vel.x = 0
        vel.y = math.sin(timer * 2.0) * 80.0
    elseif phase == 1 then
        -- Phase 2: Figure-8 pattern
        vel.x = math.sin(timer * 1.5) * 50.0
        vel.y = math.sin(timer * 3.0) * 100.0
    else
        -- Phase 3: Aggressive movement
        vel.x = math.sin(timer * 2.0) * 70.0
        vel.y = math.sin(timer * 4.0) * 120.0
    end

    -- Apply movement
    pos.x = pos.x + vel.x * dt
    pos.y = pos.y + vel.y * dt

    -- Clamp to boundaries
    pos.x = math.max(MIN_X, math.min(MAX_X, pos.x))
    pos.y = math.max(MIN_Y, math.min(MAX_Y, pos.y))

    -- Attacks
    local cooldown = ATTACK_COOLDOWNS[phase + 1]
    if boss_state.attack_timer >= cooldown then
        if phase == 0 then
            spawn_pattern_aimed(pos.x, pos.y, 5, 280, 20, 20.0)
        elseif phase == 1 then
            spawn_pattern_spiral(pos.x, pos.y, 10, 240, 25, timer * 60)
        else
            spawn_pattern_360(pos.x, pos.y, 16, 220, 30)
        end
        return true  -- Attack performed
    end
    return false
end
```

### Boss Configuration in Level JSON

Each level specifies its boss script in the level configuration:

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

### Current Bosses

| Level | Boss Name | Script | Movement Pattern |
|-------|-----------|--------|------------------|
| 1 | Mars Guardian | `boss1_mars_guardian.lua` | Vertical sine, Figure-8 |
| 2 | Nebula Core | `boss2_nebula_core.lua` | Stationary, Chase |
| 3 | Uranus Sentinel | `boss3_uranus_sentinel.lua` | Circular orbit |
| 4 | Jupiter Titan | `boss4_jupiter_titan.lua` | Erratic, all attack types |

---

## Creating a New Enemy Script

1. Create a new `.lua` file in `assets/scripts/ai/`
2. Return a function with signature `function(dt, pos, vel)`
3. Modify `pos` and `vel` to control movement
4. Use `find_nearest_player()` for player tracking

## Creating a New Boss Script

1. Create a new `.lua` file in `assets/scripts/ai/boss/`
2. Return a function with signature `function(dt, pos, vel, boss_state)`
3. Implement phase-based behavior using `boss_state.phase`
4. Use attack pattern functions for projectiles
5. Return `true` after performing an attack to reset cooldown
6. Add the script path to the level JSON configuration

---

## Technical Details

### Script Loading

Scripts are cached after first load for performance. The `LuaSystem` handles:
- Loading scripts from `assets/scripts/ai/` base path
- Caching compiled Lua functions
- Passing component data to scripts
- Applying script results back to components

### Component Requirements

**For enemies:**
- `Script` component with `path` field (e.g., `"zigzag.lua"`)
- `Position` component
- `Velocity` component

**For bosses:**
- `BossScript` component with `path` field (e.g., `"boss/boss1_mars_guardian.lua"`)
- `Position` component
- `Velocity` component
- `Health` component (for phase calculations)

### Screen Boundaries

- **Screen size:** 1920x1080
- **Boss area:** X: 1200-1800, Y: 100-980
- **Enemy spawn:** Usually X > 1920 (off-screen right)
