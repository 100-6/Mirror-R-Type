-- Zigzag Enemy AI: Changes direction based on horizontal distance traveled
-- Creates a true zigzag pattern (not bouncing off edges like bouncer)
return function(dt, pos, vel)
    local speedX = 90           -- Horizontal speed
    local speedY = 70           -- Vertical speed (slower than bouncer)
    local zigzagWidth = 180     -- Horizontal distance before changing vertical direction

    -- Initialize direction on first frame
    if vel.y == 0 then
        -- Alternate starting direction based on spawn Y position
        if pos.y < 540 then
            vel.y = speedY  -- Start going down
        else
            vel.y = -speedY -- Start going up
        end
    end

    -- Calculate which "segment" we're in based on X position
    -- Every zigzagWidth pixels, we switch direction
    local segment = math.floor(pos.x / zigzagWidth)

    -- Alternate direction based on segment number
    if segment % 2 == 0 then
        vel.y = speedY   -- Go down
    else
        vel.y = -speedY  -- Go up
    end

    -- Horizontal: constant leftward movement
    vel.x = -speedX

    -- Update position
    pos.x = pos.x + vel.x * dt
    pos.y = pos.y + vel.y * dt

    -- Soft boundary clamp (don't bounce, just limit)
    if pos.y < 80 then
        pos.y = 80
    elseif pos.y > 1000 then
        pos.y = 1000
    end
end
