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

struct PositionComponent {
    float x = 0.0f;
    float y = 0.0f;
};

struct VelocityComponent {
    float x = 0.0f;
    float y = 0.0f;
};

// Collision

struct ColliderComponent {
    float width = 0;
    float height = 0;
};

struct InputComponent {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
};

// Tags

struct ControllableComponent {};
struct EnemyComponent {};
struct ProjectileComponent {};

// Logique de jeu

struct HealthComponent
{
    int max = 100;
    int current = 100;
};

struct DamageComponent
{
    int value = 10;
};


#endif /* !COMPONENT_HPP_ */