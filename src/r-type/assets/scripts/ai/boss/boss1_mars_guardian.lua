-- Boss 1: Mars Guardian
-- Movement: Vertical sine wave (slow)
-- Phase 1: Aimed burst every 2.5s
-- Phase 2 (66% HP): Spiral pattern
-- Phase 3 (33% HP): 360 spray

local ATTACK_COOLDOWNS = {2.5, 1.8, 1.2}
local MIN_X, MAX_X = 1200.0, 1800.0
local MIN_Y, MAX_Y = 100.0, 980.0

return function(dt, pos, vel, boss_state)
    local phase = boss_state.phase
    local timer = boss_state.phase_timer

    -- Movement based on phase
    if phase == 0 then
        -- Phase 1: Slow vertical sine
        vel.x =  0.0
        vel.y = math.sin(timer * 2.0) * 80.0
    elseif phase == 1 then
        -- Phase 2: Horizontal + faster vertical sine
        vel.x = math.sin(timer * 1.5) * 50.0
        vel.y = math.sin(timer * 3.0) * 100.0
    else
        -- Phase 3: Figure-8 pattern
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
            -- Phase 1: Aimed burst at player
            local tx, ty, found = find_nearest_player(pos.x, pos.y)
            if found then
                spawn_pattern_aimed(pos.x, pos.y, 5, 280, 20, 20.0)
            end
        elseif phase == 1 then
            -- Phase 2: Spiral pattern
            spawn_pattern_spiral(pos.x, pos.y, 10, 240, 25, timer * 60)
        else
            -- Phase 3: 360 spray
            spawn_pattern_360(pos.x, pos.y, 16, 220, 30)
        end
        return true  -- Signal attack performed
    end
    return false
end
