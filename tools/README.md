# R-Type Tools

This directory contains utility tools for R-Type game development.

---

## 🗺️ Map Segment Generator

**File:** `generate_map_segments.py`

A Python script for procedurally generating interconnected map segments with navigable paths for the spaceship.

### Quick Start

```bash
# Interactive mode (recommended)
python3 tools/generate_map_segments.py

# Command-line mode
python3 tools/generate_map_segments.py --count 100 --output ./segments --style cave
```

### Features

- 🎮 **Navigable paths** - Guarantees the ship can always pass through
- 🔗 **Connected segments** - Each segment connects seamlessly to the next
- 🎨 **Multiple styles** - Cave, corridor, asteroid, open, wide
- 🎲 **Reproducible** - Use seeds for consistent generation
- 👁️ **ASCII preview** - Visualize generated maps in terminal

### Styles

| Style | Description | Min Passage |
|-------|-------------|-------------|
| `cave` | Wide passages with edge decoration (default) | 40 tiles |
| `corridor` | Extra wide clean corridors | 50 tiles |
| `asteroid` | Dense asteroid field, tighter passages | 18 tiles |
| `open` | Almost fully open space | 55 tiles |
| `wide` | Wide with pronounced edge decoration | 45 tiles |

### Command-Line Options

| Option | Short | Description |
|--------|-------|-------------|
| `--interactive` | `-i` | Run interactive mode with prompts |
| `--count` | `-n` | Number of segments (default: 100) |
| `--output` | `-o` | Output directory |
| `--style` | `-t` | Map style |
| `--seed` | `-s` | Random seed for reproducibility |
| `--width` | `-W` | Segment width (default: 60) |
| `--height` | `-H` | Segment height (default: 68) |
| `--start-id` | | Starting segment ID (default: 1) |
| `--validate` | | Validate passages after generation |
| `--preview` | | Preview N segments in ASCII |

### Examples

```bash
# Generate 100 cave-style segments
python3 tools/generate_map_segments.py -n 100 -o ./my_map/segments -t cave

# Generate 1000 asteroid-style segments with seed
python3 tools/generate_map_segments.py -n 1000 -o ./asteroid_field -t asteroid -s 42

# Generate and preview
python3 tools/generate_map_segments.py -n 50 -o ./test --preview 3

# Validate after generation
python3 tools/generate_map_segments.py -n 100 -o ./segments --validate
```

### Output Format

Each segment is saved as a JSON file with the format:

```json
{
  "segmentId": 1,
  "width": 60,
  "height": 68,
  "tiles": [
    [1, 1, 1, ...],
    [1, 0, 0, ...],
    ...
  ]
}
```

Where:
- `0` = Empty space (passable)
- `1` = Wall (solid)

### Integration

Generated segments should be placed in:
```
src/r-type/assets/maps/<map_name>/segments/
```

The game's `ChunkManagerSystem` will automatically load and chain segments together based on their `segmentId`.

---

## Requirements

- Python 3.6+
- No external dependencies (uses only standard library)
