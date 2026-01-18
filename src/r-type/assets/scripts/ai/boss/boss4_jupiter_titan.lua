-- Boss 4: Jupiter Titan
-- Movement: Slow horizontal movement
-- Phase 1: Massive spray attacks
-- Phase 2 (66% HP): Chase + spiral + aimed
-- Phase 3 (33% HP): Erratic movement + all attack types

local ATTACK_COOLDOWNS = {2.0, 1.4, 0.9}
local MIN_X, MAX_X = 1200.0, 1800.0
local MIN_Y, MAX_Y = 100.0, 980.0

return function(dt, pos, vel, boss_state)
    local phase = boss_state.phase
    local timer = boss_state.phase_timer

    -- Movement based on phase
    if phase == 0 then
        -- Phase 1: Slow horizontal oscillation
        vel.x = math.sin(timer * 0.5) * 30.0
        vel.y = math.sin(timer * 1.5) * 60.0
    elseif phase == 1 then
        -- Phase 2: Chase toward player
        local tx, ty, found = find_nearest_player(pos.x, pos.y)
        if found then
            local dx = tx - pos.x
            local dy = ty - pos.y
            local dist = math.sqrt(dx * dx + dy * dy)
            if dist > 100 then
                vel.x = (dx / dist) * 80.0
                vel.y = (dy / dist) * 80.0
            else
                vel.x = math.sin(timer * 2.0) * 50.0
                vel.y = math.sin(timer * 3.0) * 70.0
            end
        else
            vel.x = math.sin(timer * 1.0) * 50.0
            vel.y = math.sin(timer * 2.0) * 80.0
        end
    else
        -- Phase 3: Erratic movement
        vel.x = math.sin(timer * 3.0) * 100.0 + math.sin(timer * 7.0) * 50.0
        vel.y = math.sin(timer * 4.0) * 120.0 + math.cos(timer * 5.0) * 60.0
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
            -- Phase 1: Massive 360 spray
            spawn_pattern_360(pos.x, pos.y, 20, 180, 20)
        elseif phase == 1 then
            -- Phase 2: Spiral + aimed
            spawn_pattern_spiral(pos.x, pos.y, 12, 220, 25, timer * 80)
            local tx, ty, found = find_nearest_player(pos.x, pos.y)
            if found then
                spawn_pattern_aimed(pos.x, pos.y, 4, 260, 25, 20.0)
            end
        else
            -- Phase 3: All attack types combined
            spawn_pattern_360(pos.x, pos.y, 14, 200, 28)
            spawn_pattern_spiral(pos.x, pos.y, 8, 240, 30, timer * 100)
            local tx, ty, found = find_nearest_player(pos.x, pos.y)
            if found then
                spawn_pattern_aimed(pos.x, pos.y, 5, 280, 32, 15.0)
            end
        end
        return true
    end
    return false
end
