/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** BonusWeaponSystem
*/

#include "systems/BonusWeaponSystem.hpp"
#include "components/CombatConfig.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/events/GameEvents.hpp"
#include <iostream>
#include <cmath>

BonusWeaponSystem::BonusWeaponSystem(engine::IGraphicsPlugin* graphics)
    : graphics_(graphics)
{
}

void BonusWeaponSystem::init(Registry& registry)
{
    std::cout << "BonusWeaponSystem: Initialisation." << std::endl;
    (void)registry;
}

void BonusWeaponSystem::shutdown()
{
    std::cout << "BonusWeaponSystem: Arrêt." << std::endl;
}

void BonusWeaponSystem::update(Registry& registry, float dt)
{
    auto& bonusWeapons = registry.get_components<BonusWeapon>();
    auto& positions = registry.get_components<Position>();
    auto& sprites = registry.get_components<Sprite>();

    // Parcourir tous les joueurs qui ont une arme bonus
    for (size_t i = 0; i < bonusWeapons.size(); i++) {
        Entity playerEntity = bonusWeapons.get_entity_at(i);

        if (!bonusWeapons.has_entity(playerEntity))
            continue;

        auto& bonusWeapon = bonusWeapons[playerEntity];

        // Vérifier si l'arme bonus est active
        if (!bonusWeapon.active)
            continue;

        // Vérifier si l'entité de l'arme bonus existe
        if (bonusWeapon.weaponEntity == (size_t)-1 || !positions.has_entity(bonusWeapon.weaponEntity))
            continue;

        // Mettre à jour le cooldown
        bonusWeapon.timeSinceLastFire += dt;

        // Tirer si le cooldown est écoulé
        if (bonusWeapon.timeSinceLastFire >= WEAPON_BONUS_FIRERATE) {
            const Position& weaponPos = positions[bonusWeapon.weaponEntity];
            fireBonusWeapon(registry, bonusWeapon.weaponEntity, weaponPos);
            bonusWeapon.timeSinceLastFire = 0.0f;
        }
    }
}

void BonusWeaponSystem::fireBonusWeapon(Registry& registry, Entity bonusWeaponEntity, const Position& weaponPos)
{
    auto& sprites = registry.get_components<Sprite>();

    // Obtenir la taille du sprite de l'arme bonus pour calculer le point de tir
    float weaponWidth = WEAPON_BONUS_WIDTH;  // Valeur par défaut
    float weaponHeight = WEAPON_BONUS_HEIGHT;

    if (sprites.has_entity(bonusWeaponEntity)) {
        weaponWidth = sprites[bonusWeaponEntity].width;
        weaponHeight = sprites[bonusWeaponEntity].height;
        std::cout << "[BONUS WEAPON] Found sprite for bonus weapon entity " << bonusWeaponEntity
                  << " with dimensions " << weaponWidth << "x" << weaponHeight << "\n";
    } else {
        std::cout << "[BONUS WEAPON] WARNING: No sprite found for bonus weapon entity " << bonusWeaponEntity
                  << ", using default dimensions\n";
    }

    // Créer le projectile
    Entity projectile = registry.spawn_entity();

    // Position du projectile : au milieu à droite du sprite bonus
    // weaponPos est au centre du sprite (origin centré)
    // Ajuster manuellement pour être au milieu à droite
    float bulletOffsetX = weaponWidth / 2.0f - 120.0f;  // Plus vers la droite
    float bulletOffsetY = -40.0f;  // Légèrement vers le haut

    std::cout << "[BONUS WEAPON] weaponPos=(" << weaponPos.x << "," << weaponPos.y
              << ") weaponWidth=" << weaponWidth << " weaponHeight=" << weaponHeight
              << " bulletOffsetX=" << bulletOffsetX << " final projectile pos=("
              << (weaponPos.x + bulletOffsetX) << "," << (weaponPos.y + bulletOffsetY) << ")\n";

    registry.add_component(projectile, Position{
        weaponPos.x + bulletOffsetX,
        weaponPos.y + bulletOffsetY
    });

    // Vitesse du projectile (vers la droite)
    registry.add_component(projectile, Velocity{WEAPON_BONUS_SPEED, 0.0f});

    // Collider
    registry.add_component(projectile, Collider{WEAPON_BONUS_WIDTH, WEAPON_BONUS_HEIGHT});

    // Sprite du projectile (texture sera définie côté client)
    registry.add_component(projectile, Sprite{
        engine::INVALID_HANDLE,      // Texture (sera défini côté client)
        WEAPON_BONUS_WIDTH,
        WEAPON_BONUS_HEIGHT,
        0.0f,  // rotation
        engine::Color{WEAPON_BONUS_COLOR_R, WEAPON_BONUS_COLOR_G, WEAPON_BONUS_COLOR_B, WEAPON_BONUS_COLOR_A},
        WEAPON_BONUS_WIDTH / 2.0f,   // origin_x (centré)
        WEAPON_BONUS_HEIGHT / 2.0f,  // origin_y (centré)
        20     // layer (même que les autres projectiles)
    });

    // Dégâts
    registry.add_component(projectile, Damage{WEAPON_BONUS_DAMAGE});

    // Composant Projectile
    registry.add_component(projectile, Projectile{0.0f, 5.0f, 0.0f, ProjectileFaction::Player});

    // Pas de friction
    registry.add_component(projectile, NoFriction{});

    // Publier l'événement pour synchroniser via le réseau
    registry.get_event_bus().publish(ecs::ShotFiredEvent{bonusWeaponEntity, projectile});

    std::cout << "[BONUS WEAPON] Fired projectile " << projectile << " from bonus weapon entity " << bonusWeaponEntity << std::endl;
}
