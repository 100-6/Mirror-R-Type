-- Boss 3: Uranus Sentinel
-- Movement: Circular pattern (slow)
-- Phase 1: Aimed triple shots
-- Phase 2 (66% HP): Fast vertical sine + spiral
-- Phase 3 (33% HP): Figure-8 + combo attacks

local ATTACK_COOLDOWNS = {2.2, 1.6, 1.1}
local MIN_X, MAX_X = 1200.0, 1800.0
local MIN_Y, MAX_Y = 100.0, 980.0
local CENTER_X = 1500.0
local CENTER_Y = 540.0
local ORBIT_RADIUS = 150.0

return function(dt, pos, vel, boss_state)
    local phase = boss_state.phase
    local timer = boss_state.phase_timer

    -- Movement based on phase
    if phase == 0 then
        -- Phase 1: Slow circular orbit
        local angle = timer * 0.8
        local targetX = CENTER_X + math.cos(angle) * ORBIT_RADIUS
        local targetY = CENTER_Y + math.sin(angle) * ORBIT_RADIUS
        vel.x = (targetX - pos.x) * 2.0
        vel.y = (targetY - pos.y) * 2.0
    elseif phase == 1 then
        -- Phase 2: Fast vertical sine
        vel.x = math.sin(timer * 1.0) * 40.0
        vel.y = math.sin(timer * 4.0) * 150.0
    else
        -- Phase 3: Figure-8 pattern (fast)
        vel.x = math.sin(timer * 2.5) * 100.0
        vel.y = math.sin(timer * 5.0) * 130.0
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
            -- Phase 1: Aimed triple shots
            local tx, ty, found = find_nearest_player(pos.x, pos.y)
            if found then
                spawn_pattern_aimed(pos.x, pos.y, 3, 260, 20, 25.0)
            end
        elseif phase == 1 then
            -- Phase 2: Spiral pattern
            spawn_pattern_spiral(pos.x, pos.y, 14, 230, 25, timer * 75)
        else
            -- Phase 3: Combo attack - 360 spray + aimed burst
            spawn_pattern_360(pos.x, pos.y, 10, 200, 25)
            local tx, ty, found = find_nearest_player(pos.x, pos.y)
            if found then
                spawn_pattern_aimed(pos.x, pos.y, 4, 280, 30, 18.0)
            end
        end
        return true
    end
    return false
end
