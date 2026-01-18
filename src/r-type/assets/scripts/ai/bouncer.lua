-- Bouncer Enemy AI: Bounces vertically off screen edges
-- Moves left while bouncing up and down like a ball
return function(dt, pos, vel)
    local speedX = 60           -- Slow horizontal movement
    local speedY = 200          -- Vertical bounce speed
    local minY = 100
    local maxY = 980

    -- Initialize vertical velocity if zero (first frame)
    if vel.y == 0 then
        -- Random initial direction based on position
        local seed = math.abs(pos.x * 2.71 + pos.y * 3.14) % 1000
        if seed > 500 then
            vel.y = speedY
        else
            vel.y = -speedY
        end
    end

    -- Maintain constant vertical speed (bounce)
    if vel.y > 0 then
        vel.y = speedY
    else
        vel.y = -speedY
    end

    -- Constant leftward movement
    vel.x = -speedX

    -- Update position
    pos.x = pos.x + vel.x * dt
    pos.y = pos.y + vel.y * dt

    -- Bounce off top boundary
    if pos.y < minY then
        pos.y = minY
        vel.y = speedY  -- Go down
    end

    -- Bounce off bottom boundary
    if pos.y > maxY then
        pos.y = maxY
        vel.y = -speedY  -- Go up
    end
end
