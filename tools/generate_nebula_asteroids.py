#!/usr/bin/env python3
"""
Generate Nebula map segments with floating asteroids in the middle.
Wide open spaces with scattered asteroids throughout.
"""

import json
import os
import random

WIDTH = 60
HEIGHT = 68
NUM_SEGMENTS = 10
SEED = 4444

# Asteroid configuration
MIN_PASSAGE_HEIGHT = 55  # Very wide passages
ASTEROID_DENSITY = 0.002  # Only 0.2% - very few asteroids
ASTEROID_MIN_SIZE = 5
ASTEROID_MAX_SIZE = 12  # Really big asteroids (5-12 pixels)

random.seed(SEED)

def generate_asteroid_segment(segment_id):
    """Generate a segment with wide passages and floating asteroids."""
    tiles = [[0 for _ in range(WIDTH)] for _ in range(HEIGHT)]

    # Calculate passage boundaries (wide open space)
    center = HEIGHT // 2
    half_passage = MIN_PASSAGE_HEIGHT // 2
    top_wall = center - half_passage
    bottom_wall = center + half_passage

    # Add some variation to the walls
    for x in range(WIDTH):
        variation = random.randint(-3, 3)
        current_top = max(2, min(top_wall + variation, HEIGHT - MIN_PASSAGE_HEIGHT - 2))
        current_bottom = min(HEIGHT - 2, max(current_top + MIN_PASSAGE_HEIGHT, bottom_wall + variation))

        # Draw top wall
        for y in range(0, current_top):
            tiles[y][x] = 1

        # Draw bottom wall
        for y in range(current_bottom, HEIGHT):
            tiles[y][x] = 1

    # Generate floating asteroids in the middle area
    middle_start_y = top_wall + 5
    middle_end_y = bottom_wall - 5
    middle_height = middle_end_y - middle_start_y

    # Calculate number of asteroids based on density
    middle_area = WIDTH * middle_height
    num_asteroids = int(middle_area * ASTEROID_DENSITY)

    for _ in range(num_asteroids):
        # Random position in the middle area
        asteroid_x = random.randint(3, WIDTH - 4)
        asteroid_y = random.randint(middle_start_y, middle_end_y)

        # Random asteroid size
        asteroid_size = random.randint(ASTEROID_MIN_SIZE, ASTEROID_MAX_SIZE)

        # Draw asteroid (irregular blob shape)
        if asteroid_size == 1:
            # Single pixel asteroid
            if tiles[asteroid_y][asteroid_x] == 0:
                tiles[asteroid_y][asteroid_x] = 1
        else:
            # Multi-pixel asteroid (irregular shape)
            for dy in range(-asteroid_size//2, asteroid_size//2 + 1):
                for dx in range(-asteroid_size//2, asteroid_size//2 + 1):
                    ny = asteroid_y + dy
                    nx = asteroid_x + dx

                    # Check bounds
                    if 0 <= ny < HEIGHT and 0 <= nx < WIDTH:
                        # Create irregular shape (not perfect square)
                        if random.random() < 0.7:  # 70% chance to fill each cell
                            if tiles[ny][nx] == 0:  # Don't overwrite walls
                                tiles[ny][nx] = 1

    return tiles

# Generate all segments
output_dir = "src/r-type/assets/maps/nebula_outpost/segments"
os.makedirs(output_dir, exist_ok=True)

print(f"Generating {NUM_SEGMENTS} Nebula segments with floating asteroids...")
print(f"Output: {output_dir}")

for i in range(NUM_SEGMENTS):
    segment_id = i + 1
    tiles = generate_asteroid_segment(segment_id)

    # Save to file
    filename = f"map_segment_{segment_id:02d}.json"
    filepath = os.path.join(output_dir, filename)

    with open(filepath, 'w') as f:
        f.write("{\n")
        f.write(f'  "segmentId": {segment_id},\n')
        f.write(f'  "width": {WIDTH},\n')
        f.write(f'  "height": {HEIGHT},\n')
        f.write('  "tiles": [\n')
        for row_idx, row in enumerate(tiles):
            row_str = ", ".join(str(cell) for cell in row)
            comma = "," if row_idx < len(tiles) - 1 else ""
            f.write(f'    [{row_str}]{comma}\n')
        f.write('  ]\n')
        f.write('}\n')

    if (i + 1) % 10 == 0 or i + 1 == NUM_SEGMENTS:
        print(f"  Generated {i + 1}/{NUM_SEGMENTS} segments")

print(f"\n✓ Done! Generated {NUM_SEGMENTS} segments with floating asteroids.")
