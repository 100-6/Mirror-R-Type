/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** KeyConfig - Configuration des touches à surveiller par l'InputSystem
*/

#pragma once

#include "IInputPlugin.hpp"
#include <array>

namespace engine {

/**
 * @brief Configuration globale des touches surveillées par l'InputSystem
 *
 * Cette liste définit toutes les touches que l'InputSystem doit surveiller.
 * Peut être modifiée pour ajouter/retirer des touches selon les besoins.
 */
namespace KeyConfig {

    /**
     * @brief Toutes les touches surveillées par défaut
     */
    constexpr std::array ALL_KEYS = {
        // Lettres A-Z (26)
        Key::A, Key::B, Key::C, Key::D, Key::E, Key::F, Key::G, Key::H,
        Key::I, Key::J, Key::K, Key::L, Key::M, Key::N, Key::O, Key::P,
        Key::Q, Key::R, Key::S, Key::T, Key::U, Key::V, Key::W, Key::X,
        Key::Y, Key::Z,

        // Chiffres 0-9 (10)
        Key::Num0, Key::Num1, Key::Num2, Key::Num3, Key::Num4,
        Key::Num5, Key::Num6, Key::Num7, Key::Num8, Key::Num9,

        // Modificateurs (6)
        Key::LShift, Key::RShift, Key::LControl, Key::RControl,
        Key::LAlt, Key::RAlt,

        // Navigation (4)
        Key::Up, Key::Down, Key::Left, Key::Right,

        // Touches spéciales (5)
        Key::Space, Key::Enter, Key::Escape, Key::Backspace, Key::Tab,

        // Touches fonction (12)
        Key::F1, Key::F2, Key::F3, Key::F4, Key::F5, Key::F6,
        Key::F7, Key::F8, Key::F9, Key::F10, Key::F11, Key::F12
    };

    /**
     * @brief Tous les boutons de souris surveillés par défaut
     */
    constexpr std::array ALL_MOUSE_BUTTONS = {
        MouseButton::Left,
        MouseButton::Right,
        MouseButton::Middle,
        MouseButton::XButton1,
        MouseButton::XButton2
    };

} // namespace KeyConfig

} // namespace engine
