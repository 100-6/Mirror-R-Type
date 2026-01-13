#!/usr/bin/env python3
"""
R-Type Map Segment Generator

Generates multiple interconnected map segments with intelligent path generation.
Creates navigable maps where the spaceship can pass through.

Usage:
    python generate_map_segments.py --count 100 --output ./generated_segments
    python generate_map_segments.py --count 1000 --output ./my_map/segments --seed 42
"""

import argparse
import json
import os
import random
import sys
from dataclasses import dataclass
from typing import List, Tuple, Optional

# Default segment dimensions (matching existing format)
DEFAULT_WIDTH = 60
DEFAULT_HEIGHT = 68

# Passage configuration - ensures the ship can always pass
MIN_PASSAGE_HEIGHT = 8  # Minimum vertical space for ship to pass
PASSAGE_BUFFER = 3      # Extra buffer for comfortable navigation


@dataclass
class PathState:
    """Tracks the current path state for continuity between segments."""
    top_y: int      # Top boundary of the passage
    bottom_y: int   # Bottom boundary of the passage
    
    @property
    def center_y(self) -> int:
        return (self.top_y + self.bottom_y) // 2
    
    @property
    def passage_height(self) -> int:
        return self.bottom_y - self.top_y


class MapSegmentGenerator:
    """Generates interconnected map segments with navigable paths."""
    
    def __init__(
        self,
        width: int = DEFAULT_WIDTH,
        height: int = DEFAULT_HEIGHT,
        seed: Optional[int] = None,
        style: str = "cave"
    ):
        self.width = width
        self.height = height
        self.style = style
        
        if seed is not None:
            random.seed(seed)
        
        # Define generation parameters based on style
        self.style_params = self._get_style_params(style)
    
    def _get_style_params(self, style: str) -> dict:
        """Get generation parameters for different map styles."""
        styles = {
            "cave": {
                "wall_density": 0.15,
                "obstacle_frequency": 0.15,
                "path_variation": 8,
                "min_passage": 40,  # Very wide passage - covers most of the screen
                "wall_thickness_range": (3, 10),
                "stalactite_chance": 0.25,
                "max_stalactite_length": 5,
            },
            "corridor": {
                "wall_density": 0.05,
                "obstacle_frequency": 0.1,
                "path_variation": 4,
                "min_passage": 50,  # Extra wide corridor
                "wall_thickness_range": (3, 8),
                "stalactite_chance": 0.15,
                "max_stalactite_length": 3,
            },
            "asteroid": {
                "wall_density": 0.25,
                "obstacle_frequency": 0.4,
                "path_variation": 15,
                "min_passage": MIN_PASSAGE_HEIGHT + 10,  # Still navigable
                "wall_thickness_range": (1, 4),
                "stalactite_chance": 0.35,
                "max_stalactite_length": 4,
            },
            "open": {
                "wall_density": 0.02,
                "obstacle_frequency": 0.03,
                "path_variation": 3,
                "min_passage": 55,  # Almost fully open
                "wall_thickness_range": (2, 5),
                "stalactite_chance": 0.08,
                "max_stalactite_length": 2,
            },
            "wide": {
                "wall_density": 0.1,
                "obstacle_frequency": 0.2,
                "path_variation": 5,
                "min_passage": 45,  # Wide passage with decoration on edges
                "wall_thickness_range": (5, 12),
                "stalactite_chance": 0.3,
                "max_stalactite_length": 6,
            },
        }
        return styles.get(style, styles["cave"])
    
    def generate_segment(
        self,
        segment_id: int,
        entry_state: Optional[PathState] = None
    ) -> Tuple[List[List[int]], PathState]:
        """
        Generate a single map segment.
        
        Args:
            segment_id: Identifier for this segment
            entry_state: Path state from previous segment for continuity
        
        Returns:
            Tuple of (tiles grid, exit path state)
        """
        # Initialize empty grid
        tiles = [[0 for _ in range(self.width)] for _ in range(self.height)]
        
        params = self.style_params
        
        # Determine entry path
        if entry_state is None:
            # First segment - start with centered open path
            center = self.height // 2
            half_passage = params["min_passage"] // 2 + PASSAGE_BUFFER
            entry_state = PathState(
                top_y=center - half_passage,
                bottom_y=center + half_passage
            )
        
        # Generate the path through this segment with gradual changes
        path_points = self._generate_path(entry_state, params)
        
        # Draw walls based on path
        self._draw_walls(tiles, path_points, params)
        
        # Add obstacles within safe areas
        self._add_obstacles(tiles, path_points, params)
        
        # Calculate exit state for next segment
        exit_state = PathState(
            top_y=path_points[-1][0],
            bottom_y=path_points[-1][1]
        )
        
        return tiles, exit_state
    
    def _generate_path(
        self,
        entry_state: PathState,
        params: dict
    ) -> List[Tuple[int, int]]:
        """
        Generate path points across the segment width.
        
        Returns list of (top_y, bottom_y) tuples for each column.
        """
        path_points = []
        
        current_top = entry_state.top_y
        current_bottom = entry_state.bottom_y
        
        for x in range(self.width):
            # Calculate target variation
            progress = x / self.width
            
            # Random walk for natural cave-like appearance
            variation = params["path_variation"]
            
            # Vertical movement of the entire passage
            center_shift = random.randint(-2, 2)
            
            # Width variation of the passage
            width_change = random.randint(-1, 1)
            
            # Apply changes gradually
            new_top = current_top + center_shift + width_change
            new_bottom = current_bottom + center_shift - width_change
            
            # Ensure minimum passage height
            passage_height = new_bottom - new_top
            if passage_height < params["min_passage"]:
                extra = params["min_passage"] - passage_height
                new_top -= extra // 2
                new_bottom += extra - (extra // 2)
            
            # Keep within bounds (leave some wall space at top and bottom)
            min_wall = 3
            new_top = max(min_wall, min(new_top, self.height - params["min_passage"] - min_wall))
            new_bottom = max(new_top + params["min_passage"], min(new_bottom, self.height - min_wall))
            
            # Smooth transition (prevent too abrupt changes)
            max_change = 2
            if abs(new_top - current_top) > max_change:
                new_top = current_top + max_change * (1 if new_top > current_top else -1)
            if abs(new_bottom - current_bottom) > max_change:
                new_bottom = current_bottom + max_change * (1 if new_bottom > current_bottom else -1)
            
            path_points.append((new_top, new_bottom))
            current_top = new_top
            current_bottom = new_bottom
        
        return path_points
    
    def _draw_walls(
        self,
        tiles: List[List[int]],
        path_points: List[Tuple[int, int]],
        params: dict
    ) -> None:
        """Draw walls above and below the path."""
        for x, (top_y, bottom_y) in enumerate(path_points):
            # Draw top wall
            for y in range(0, int(top_y)):
                tiles[y][x] = 1
            
            # Draw bottom wall
            for y in range(int(bottom_y), self.height):
                tiles[y][x] = 1
    
    def _add_obstacles(
        self,
        tiles: List[List[int]],
        path_points: List[Tuple[int, int]],
        params: dict
    ) -> None:
        """Add stalactites, stalagmites, and floating obstacles."""
        
        stalactite_chance = params["stalactite_chance"]
        
        for x in range(2, self.width - 2):  # Avoid edges
            top_y, bottom_y = path_points[x]
            passage_height = bottom_y - top_y
            
            # Only add obstacles if passage is wide enough
            if passage_height < params["min_passage"] + 4:
                continue
            
            # Stalactite from ceiling
            if random.random() < stalactite_chance:
                length = random.randint(1, min(3, (passage_height - params["min_passage"]) // 2))
                for dy in range(length):
                    if int(top_y) + dy < bottom_y - params["min_passage"]:
                        tiles[int(top_y) + dy][x] = 1
            
            # Stalagmite from floor
            if random.random() < stalactite_chance:
                length = random.randint(1, min(3, (passage_height - params["min_passage"]) // 2))
                for dy in range(length):
                    if int(bottom_y) - 1 - dy > top_y + params["min_passage"]:
                        tiles[int(bottom_y) - 1 - dy][x] = 1
        
        # Add some floating obstacles (sparse)
        obstacle_count = int(self.width * params["obstacle_frequency"] * 0.1)
        for _ in range(obstacle_count):
            x = random.randint(5, self.width - 5)
            top_y, bottom_y = path_points[x]
            safe_top = int(top_y) + params["min_passage"] // 2
            safe_bottom = int(bottom_y) - params["min_passage"] // 2
            
            if safe_bottom - safe_top > 4:
                y = random.randint(safe_top + 2, safe_bottom - 2)
                # Small floating obstacle (1x1 or 2x2)
                if 0 <= y < self.height and tiles[y][x] == 0:
                    tiles[y][x] = 1


def generate_segments(
    count: int,
    output_dir: str,
    width: int = DEFAULT_WIDTH,
    height: int = DEFAULT_HEIGHT,
    seed: Optional[int] = None,
    style: str = "cave",
    start_id: int = 1
) -> None:
    """
    Generate multiple interconnected map segments.
    
    Args:
        count: Number of segments to generate
        output_dir: Directory to save segment files
        width: Width of each segment
        height: Height of each segment
        seed: Random seed for reproducibility
        style: Map style (cave, corridor, asteroid, open)
        start_id: Starting segment ID
    """
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    # Initialize generator
    generator = MapSegmentGenerator(width, height, seed, style)
    
    # Generate segments
    current_state: Optional[PathState] = None
    
    print(f"Generating {count} segments in '{style}' style...")
    print(f"Output directory: {output_dir}")
    print(f"Segment dimensions: {width}x{height}")
    
    for i in range(count):
        segment_id = start_id + i
        
        # Generate segment
        tiles, exit_state = generator.generate_segment(segment_id, current_state)
        current_state = exit_state
        
        # Create segment data
        segment_data = {
            "segmentId": segment_id,
            "width": width,
            "height": height,
            "tiles": tiles
        }
        
        # Save to file with compact tile format (matching original files)
        filename = f"map_segment_{segment_id:02d}.json"
        if segment_id >= 100:
            filename = f"map_segment_{segment_id:03d}.json"
        if segment_id >= 1000:
            filename = f"map_segment_{segment_id:04d}.json"
        
        filepath = os.path.join(output_dir, filename)
        
        # Use custom JSON formatting to match original compact tile format
        with open(filepath, 'w') as f:
            f.write("{\n")
            f.write(f'  "segmentId": {segment_id},\n')
            f.write(f'  "width": {width},\n')
            f.write(f'  "height": {height},\n')
            f.write('  "tiles": [\n')
            for row_idx, row in enumerate(tiles):
                row_str = ", ".join(str(cell) for cell in row)
                comma = "," if row_idx < len(tiles) - 1 else ""
                f.write(f'    [{row_str}]{comma}\n')
            f.write('  ]\n')
            f.write('}\n')
        
        # Progress indicator
        if (i + 1) % 100 == 0 or i + 1 == count:
            print(f"  Generated {i + 1}/{count} segments")
    
    print(f"\nDone! Generated {count} segments in {output_dir}")
    print(f"Files: map_segment_01.json to map_segment_{start_id + count - 1:02d}.json")


def validate_segments(output_dir: str) -> bool:
    """Validate that all segments have continuous passages."""
    files = sorted([f for f in os.listdir(output_dir) if f.endswith('.json')])
    
    prev_exit = None
    all_valid = True
    
    for filename in files:
        filepath = os.path.join(output_dir, filename)
        with open(filepath, 'r') as f:
            segment = json.load(f)
        
        tiles = segment['tiles']
        width = segment['width']
        
        # Check entry (first column)
        entry_passage = sum(1 for row in tiles if row[0] == 0)
        
        # Check exit (last column)
        exit_passage = sum(1 for row in tiles if row[-1] == 0)
        
        if entry_passage < MIN_PASSAGE_HEIGHT:
            print(f"WARNING: {filename} has narrow entry passage: {entry_passage}")
            all_valid = False
        
        if exit_passage < MIN_PASSAGE_HEIGHT:
            print(f"WARNING: {filename} has narrow exit passage: {exit_passage}")
            all_valid = False
        
        prev_exit = exit_passage
    
    return all_valid


def preview_segment(filepath: str, scale: int = 1) -> None:
    """Display an ASCII preview of a map segment."""
    with open(filepath, 'r') as f:
        segment = json.load(f)
    
    tiles = segment['tiles']
    segment_id = segment['segmentId']
    width = segment['width']
    height = segment['height']
    
    print(f"\n=== Segment {segment_id} ({width}x{height}) ===\n")
    
    # Characters for visualization
    wall_char = "█"
    empty_char = " "
    
    # Apply scaling (show every nth row/column)
    display_height = height // scale
    display_width = width // scale
    
    for y in range(0, height, scale):
        row_chars = []
        for x in range(0, width, scale):
            if tiles[y][x] == 1:
                row_chars.append(wall_char)
            else:
                row_chars.append(empty_char)
        print("".join(row_chars))
    
    print()


def interactive_mode():
    """Interactive mode with user prompts."""
    print("\n" + "=" * 60)
    print("  🚀 R-Type Map Segment Generator - Interactive Mode")
    print("=" * 60 + "\n")
    
    # Style selection
    styles = ["cave", "corridor", "asteroid", "open", "wide"]
    print("Available styles:")
    print("  1. cave      - Wide passages with decoration on edges (default)")
    print("  2. corridor  - Extra wide clean corridors")
    print("  3. asteroid  - Dense asteroid field, tighter passages")
    print("  4. open      - Almost fully open space")
    print("  5. wide      - Wide with pronounced edge decoration")
    
    style_choice = input("\nSelect style [1-5] (default: 1): ").strip()
    if style_choice in ["1", ""]:
        style = "cave"
    elif style_choice == "2":
        style = "corridor"
    elif style_choice == "3":
        style = "asteroid"
    elif style_choice == "4":
        style = "open"
    elif style_choice == "5":
        style = "wide"
    else:
        print(f"Invalid choice, using 'cave'")
        style = "cave"
    
    print(f"✓ Style: {style}")
    
    # Number of segments
    count_input = input("\nNumber of segments to generate (default: 100): ").strip()
    try:
        count = int(count_input) if count_input else 100
    except ValueError:
        print("Invalid number, using 100")
        count = 100
    print(f"✓ Count: {count}")
    
    # Output directory
    default_output = "./generated_segments"
    output_input = input(f"\nOutput directory (default: {default_output}): ").strip()
    output_dir = output_input if output_input else default_output
    print(f"✓ Output: {output_dir}")
    
    # Seed
    seed_input = input("\nRandom seed for reproducibility (press Enter for random): ").strip()
    try:
        seed = int(seed_input) if seed_input else None
    except ValueError:
        print("Invalid seed, using random")
        seed = None
    print(f"✓ Seed: {seed if seed else 'random'}")
    
    # Dimensions (optional)
    use_custom_dims = input("\nUse custom dimensions? [y/N]: ").strip().lower()
    if use_custom_dims == 'y':
        width_input = input(f"  Width (default: {DEFAULT_WIDTH}): ").strip()
        height_input = input(f"  Height (default: {DEFAULT_HEIGHT}): ").strip()
        try:
            width = int(width_input) if width_input else DEFAULT_WIDTH
            height = int(height_input) if height_input else DEFAULT_HEIGHT
        except ValueError:
            print("  Invalid dimensions, using defaults")
            width, height = DEFAULT_WIDTH, DEFAULT_HEIGHT
    else:
        width, height = DEFAULT_WIDTH, DEFAULT_HEIGHT
    print(f"✓ Dimensions: {width}x{height}")
    
    # Preview
    preview_input = input("\nPreview segments after generation? [Y/n]: ").strip().lower()
    if preview_input in ['', 'y', 'yes']:
        preview_count_input = input("  How many to preview? (default: 2): ").strip()
        try:
            preview_count = int(preview_count_input) if preview_count_input else 2
        except ValueError:
            preview_count = 2
    else:
        preview_count = 0
    
    # Confirmation
    print("\n" + "-" * 40)
    print("Configuration Summary:")
    print(f"  Style: {style}")
    print(f"  Count: {count}")
    print(f"  Output: {output_dir}")
    print(f"  Seed: {seed if seed else 'random'}")
    print(f"  Dimensions: {width}x{height}")
    print(f"  Preview: {preview_count} segment(s)" if preview_count else "  Preview: disabled")
    print("-" * 40)
    
    confirm = input("\nGenerate segments? [Y/n]: ").strip().lower()
    if confirm in ['n', 'no']:
        print("Cancelled.")
        return
    
    # Generate!
    print()
    generate_segments(
        count=count,
        output_dir=output_dir,
        width=width,
        height=height,
        seed=seed,
        style=style,
        start_id=1
    )
    
    # Validate
    print("\nValidating segments...")
    if validate_segments(output_dir):
        print("✓ All segments validated successfully!")
    else:
        print("⚠ Some segments have issues (see warnings above)")
    
    # Preview
    if preview_count > 0:
        print(f"\nPreviewing first {preview_count} segment(s)...")
        files = sorted([f for f in os.listdir(output_dir) if f.endswith('.json')])[:preview_count]
        for filename in files:
            preview_segment(os.path.join(output_dir, filename))
    
    print("\n✓ Done!")


def main():
    parser = argparse.ArgumentParser(
        description="Generate interconnected R-Type map segments",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  Interactive mode (recommended):
    python generate_map_segments.py
    
  Command-line mode:
    python generate_map_segments.py --count 100 --output ./segments --style cave
  
  Generate 1000 asteroid-style segments with a specific seed:
    python generate_map_segments.py --count 1000 --output ./asteroid_field --style asteroid --seed 42

Styles:
  cave      - Wide passages with decoration on edges (default)
  corridor  - Extra wide clean corridors
  asteroid  - Dense asteroid field with tight passages
  open      - Almost fully open space
  wide      - Wide with pronounced edge decoration
        """
    )
    
    parser.add_argument(
        "--interactive", "-i",
        action="store_true",
        help="Run in interactive mode with prompts (default if no args)"
    )
    
    parser.add_argument(
        "--count", "-n",
        type=int,
        default=100,
        help="Number of segments to generate (default: 100)"
    )
    
    parser.add_argument(
        "--output", "-o",
        type=str,
        default="./generated_segments",
        help="Output directory for segment files (default: ./generated_segments)"
    )
    
    parser.add_argument(
        "--width", "-W",
        type=int,
        default=DEFAULT_WIDTH,
        help=f"Width of each segment (default: {DEFAULT_WIDTH})"
    )
    
    parser.add_argument(
        "--height", "-H",
        type=int,
        default=DEFAULT_HEIGHT,
        help=f"Height of each segment (default: {DEFAULT_HEIGHT})"
    )
    
    parser.add_argument(
        "--seed", "-s",
        type=int,
        default=None,
        help="Random seed for reproducible generation"
    )
    
    parser.add_argument(
        "--style", "-t",
        type=str,
        choices=["cave", "corridor", "asteroid", "open", "wide"],
        default="cave",
        help="Map generation style (default: cave)"
    )
    
    parser.add_argument(
        "--start-id",
        type=int,
        default=1,
        help="Starting segment ID (default: 1)"
    )
    
    parser.add_argument(
        "--validate",
        action="store_true",
        help="Validate generated segments after creation"
    )
    
    parser.add_argument(
        "--preview",
        type=int,
        nargs="?",
        const=3,
        default=0,
        help="Preview first N segments in ASCII (default: 3 if flag used)"
    )
    
    args = parser.parse_args()
    
    # If no arguments provided (except script name), run interactive mode
    if len(sys.argv) == 1 or args.interactive:
        interactive_mode()
        return
    
    # Command-line mode
    generate_segments(
        count=args.count,
        output_dir=args.output,
        width=args.width,
        height=args.height,
        seed=args.seed,
        style=args.style,
        start_id=args.start_id
    )
    
    # Optionally validate
    if args.validate:
        print("\nValidating segments...")
        if validate_segments(args.output):
            print("All segments validated successfully!")
        else:
            print("Some segments have issues (see warnings above)")
    
    # Optionally preview
    if args.preview > 0:
        print(f"\nPreviewing first {args.preview} segment(s)...")
        files = sorted([f for f in os.listdir(args.output) if f.endswith('.json')])[:args.preview]
        for filename in files:
            preview_segment(os.path.join(args.output, filename))


if __name__ == "__main__":
    main()

