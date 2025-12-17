#pragma once

namespace rtype::shared::dimensions {

// Vaisseaux
constexpr float PLAYER_WIDTH = 128.0f;
constexpr float PLAYER_HEIGHT = 128.0f;

constexpr float PROJECTILE_WIDTH = 28.0f;
constexpr float PROJECTILE_HEIGHT = 12.0f;

constexpr float ENEMY_BASIC_WIDTH = 120.0f;
constexpr float ENEMY_BASIC_HEIGHT = 120.0f;
constexpr float ENEMY_FAST_WIDTH = 100.0f;
constexpr float ENEMY_FAST_HEIGHT = 100.0f;
constexpr float ENEMY_TANK_WIDTH = 160.0f;
constexpr float ENEMY_TANK_HEIGHT = 160.0f;
constexpr float ENEMY_BOSS_WIDTH = 280.0f;
constexpr float ENEMY_BOSS_HEIGHT = 280.0f;

// Murs et bonus
constexpr float WALL_WIDTH = 100.0f;
constexpr float WALL_HEIGHT = 80.0f;

constexpr float BONUS_SIZE = 12.0f;

} // namespace rtype::shared::dimensions

/* ensure header guard is not necessary due to #pragma once */
