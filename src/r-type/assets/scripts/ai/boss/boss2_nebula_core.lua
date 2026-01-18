-- Boss 2: Nebula Core
-- Movement: Stationary with slight sine
-- Phase 1: Regular 360 spray
-- Phase 2 (66% HP): Figure-8 + laser sweep
-- Phase 3 (33% HP): Aggressive chase + random barrage

local ATTACK_COOLDOWNS = {2.0, 1.5, 1.0}
local MIN_X, MAX_X = 1200.0, 1800.0
local MIN_Y, MAX_Y = 100.0, 980.0
local SPAWN_X = 1600.0
local SPAWN_Y = 540.0

return function(dt, pos, vel, boss_state)
    local phase = boss_state.phase
    local timer = boss_state.phase_timer

    -- Movement based on phase
    if phase == 0 then
        -- Phase 1: Slight vertical sine (almost stationary)
        vel.x = 0
        vel.y = math.sin(timer * 1.5) * 40.0
    elseif phase == 1 then
        -- Phase 2: Figure-8 pattern
        vel.x = math.sin(timer * 2.0) * 60.0
        vel.y = math.sin(timer * 4.0) * 80.0
    else
        -- Phase 3: Aggressive chase toward nearest player
        local tx, ty, found = find_nearest_player(pos.x, pos.y)
        if found then
            local dx = tx - pos.x
            local dy = ty - pos.y
            local dist = math.sqrt(dx * dx + dy * dy)
            if dist > 50 then
                vel.x = (dx / dist) * 120.0
                vel.y = (dy / dist) * 120.0
            else
                vel.x = 0
                vel.y = 0
            end
        else
            vel.x = 0
            vel.y = math.sin(timer * 3.0) * 100.0
        end
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
            -- Phase 1: 360 spray
            spawn_pattern_360(pos.x, pos.y, 12, 200, 20)
        elseif phase == 1 then
            -- Phase 2: Spiral pattern
            spawn_pattern_spiral(pos.x, pos.y, 12, 220, 25, timer * 90)
        else
            -- Phase 3: Random barrage + aimed
            spawn_pattern_random(pos.x, pos.y, 8, 250, 30)
            local tx, ty, found = find_nearest_player(pos.x, pos.y)
            if found then
                spawn_pattern_aimed(pos.x, pos.y, 3, 300, 35, 15.0)
            end
        end
        return true
    end
    return false
end
