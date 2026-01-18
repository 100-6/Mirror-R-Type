                -- Kamikaze Enemy AI: Aggressive dive-bomb attack
-- Waits briefly then rushes directly at the player at high speed
return function(dt, pos, vel)
    local approachSpeed = 60   -- Initial slow approach speed
    local chargeSpeed = 350    -- High-speed charge
    local chargeDistance = 600 -- Distance at which to start charging
    local waitTime = 0.8       -- Brief pause before charging

    local targetX, targetY, found = find_nearest_player(pos.x, pos.y)

    if found then
        local dx = targetX - pos.x
        local dy = targetY - pos.y
        local dist = math.sqrt(dx*dx + dy*dy)

        -- Determine behavior based on distance
        local speed
        if dist < chargeDistance then
            -- Close enough: CHARGE!
            speed = chargeSpeed
        else
            -- Far away: slow approach
            speed = approachSpeed
        end

        -- Normalize and apply speed
        if dist > 1.0 then
            vel.x = (dx / dist) * speed
            vel.y = (dy / dist) * speed
        end
    else
        -- No player found: move left aggressively
        vel.x = -chargeSpeed * 0.5
        vel.y = 0
    end

    pos.x = pos.x + vel.x * dt
    pos.y = pos.y + vel.y * dt
end
