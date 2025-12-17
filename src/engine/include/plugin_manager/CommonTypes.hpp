/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Common types used across plugins
*/

#pragma once

#include <cstdint>

namespace engine {

/**
 * @brief 2D Vector with float components
 */
struct Vector2f {
    float x = 0.0f;
    float y = 0.0f;

    Vector2f() = default;
    Vector2f(float x, float y) : x(x), y(y) {}
};

/**
 * @brief Color with RGBA components
 */
struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;

    Color() = default;
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) 
        : r(r), g(g), b(b), a(a) {}

    // Common colors
    static const Color Black;
    static const Color White;
    static const Color Red;
    static const Color Green;
    static const Color Blue;
    static const Color Yellow;
    static const Color Magenta;
    static const Color Cyan;
    static const Color Purple;
    static const Color ShieldViolet;
    static const Color SpeedBlue;
    static const Color Transparent;
};

// Color definitions
inline const Color Color::Black{0, 0, 0, 255};
inline const Color Color::White{255, 255, 255, 255};
inline const Color Color::Red{255, 0, 0, 255};
inline const Color Color::Green{0, 255, 0, 255};
inline const Color Color::Blue{0, 0, 255, 255};
inline const Color Color::Yellow{255, 255, 0, 255};
inline const Color Color::Magenta{255, 0, 255, 255};
inline const Color Color::Cyan{0, 255, 255, 255};
inline const Color Color::Purple{148, 0, 211, 255};
inline const Color Color::ShieldViolet{148, 0, 211, 100};
inline const Color Color::SpeedBlue{0, 150, 255, 255};
inline const Color Color::Transparent{0, 0, 0, 0};

/**
 * @brief Handle type for textures
 */
using TextureHandle = uint32_t;

/**
 * @brief Handle type for fonts
 */
using FontHandle = uint32_t;

/**
 * @brief Handle type for sounds
 */
using SoundHandle = uint32_t;

/**
 * @brief Handle type for music
 */
using MusicHandle = uint32_t;

/**
 * @brief Network client identifier
 */
using ClientId = uint32_t;

/**
 * @brief Invalid handle constant
 */
constexpr uint32_t INVALID_HANDLE = 0;

}
