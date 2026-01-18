# Enemy Guide

This document catalogs the various enemy types, their statistics, behaviors, and visual representations in Mirror R-Type.

## Stats Overview

Base statistics for enemies (Difficuly: Normal). Stats scale with difficulty settings.

| Class | Health (HP) | Speed (px/s) | Dimensions (px) | Damage |
| :--- | :--- | :--- | :--- | :--- |
| **Basic** | 50 | 100 | 120x120 | 25 |
| **Fast** | 30 | 200 | 100x100 | 25 |
| **Tank** | 150 | 50 | 160x160 | 40 |
| **Boss** | 500+ | 30 | 280x280 | 50 |

## Difficulty Scaling

Game difficulty adjusts enemy attributes by the following multipliers:

| Difficulty | Health Multiplier | Speed Multiplier | Damage Multiplier |
| :--- | :--- | :--- | :--- |
| **Easy** | 0.7x | 0.8x | 0.7x |
| **Normal** | 1.0x | 1.0x | 1.0x |
| **Hard** | 1.5x | 1.3x | 1.5x |

## Enemy Classes

### Basic Class
The standard fodder enemy. They generally move in simple patterns (straight lines) and serve as the primary obstacle.

*   **Sprite**: Varied (Enemie9 - Enemie13).
    *   **Variants**: `basic`, `basic_v1` through `basic_v5`.
    *   *Note*: Variants are visually distinct but functionally identical to provide visual variety.
*   **Special Variations**:
    *   **Bouncer**: Moves left but bounces off the top and bottom of the screen. (Uses **Enemie11** sprite visually).

### Fast Class
Low health but high speed enemies designed to flank or surprise the player.

*   **Sprite**: Uniformly uses **Enemie13** (Sleek Gold/Blue ship).
*   **Variations**:
    *   **Standard Fast**: Moves rapidly in a straight line.
    *   **Zigzag**: Moves in a sine-wave pattern.
    *   **Kamikaze**: Special variant that charges directly at the player.
        *   *Visual Exception*: Uses a unique **Kamikaze** sprite (not Enemie13).
    *   **Coward**: Approaches then retreats.
    *   **Chaser**: Actively follows the player's Y-coordinate.

### Tank Class
Heavily armored enemies that act as damage sponges. They act as moving walls.

*   **Sprite**: Uniformly uses **Enemie11** (Bulky Red ship).
*   **Variations**:
    *   **Standard Tank**: Moves very slowly in a straight line.
    *   **Patrol**: Moves vertically or loops in a specific area.
    *   **Spiral**: Moves in a circular spiral pattern.

### Boss Class
End-of-level guardians with massive health pools and unique scripts.

*   **Mars Boss**: The standard boss for Level 1.
*   **Jupiter Boss**: The boss for Level 4.
*   **Uranus Boss**: The boss for Level 3.
*   **Health**: Starts at 500 HP, scales significantly.

## Configuration

Enemy definitions are loaded from `assets/config/enemies.json`.
Lua scripts in `assets/scripts/` define the movement patterns (AI).

```json
"basic": {
    "script": "basic.lua",
    "description": "Standard enemy - moves left steadily"
}
```
