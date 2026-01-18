-- Basic Enemy AI: Simple leftward movement
return function(dt, pos, vel)
    local speed = 100

    -- Simple leftward movement
    vel.x = -speed
    vel.y = 0

    pos.x = pos.x + vel.x * dt
    pos.y = pos.y + vel.y * dt
end
