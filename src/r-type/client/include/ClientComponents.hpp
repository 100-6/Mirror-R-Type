/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Client-only components
*/

#pragma once

struct LastServerState {
    float x = 0.0f;
    float y = 0.0f;
    float timestamp = 0.0f;  // Seconds since client start for extrapolation
};

