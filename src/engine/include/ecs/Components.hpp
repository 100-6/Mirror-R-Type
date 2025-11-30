/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Component
*/

#ifndef COMPONENT_HPP_
#define COMPONENT_HPP_
#include <iostream>

// Physique et Mouvement 

struct Position {
    float x = 0.0f;
    float y = 0.0f;
};

struct Velocity {
    float x = 0.0f;
    float y = 0.0f;
};

// Collision

struct Collider {
    float width = 0;
    float height = 0;
};

struct Input {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

// Tags

struct Controllable {};
struct Enemy {};
struct Projectile {};
struct Wall {};

// Logique de jeu

struct Health
{
    int max = 100;
    int current = 100;
};

struct Damage
{
    int value = 10;
};


#endif /* !COMPONENT_HPP_ */