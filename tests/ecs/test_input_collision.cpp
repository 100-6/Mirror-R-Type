/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Test Shooting + Collision - Projectiles vs Ennemis + Murs
*/

#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "systems/CollisionSystem.hpp"
#include "ecs/systems/InputSystem.hpp"
#include "plugins/input/raylib/RaylibInputPlugin.hpp"
#include <raylib.h>
#include <iostream>
#include <cmath>

int main() {
    // Initialisation de Raylib
    const int screenWidth = 800;
    const int screenHeight = 600;
    
    InitWindow(screenWidth, screenHeight, "Test Collision R-Type - Projectiles vs Ennemis");
    SetTargetFPS(60);
    
    std::cout << "=== Test Collision R-Type - Projectiles vs Ennemis ===" << std::endl;
    std::cout << std::endl;
    
    // Créer le Registry
    Registry registry;
    registry.register_component<Position>();
    registry.register_component<Velocity>();
    registry.register_component<Input>();
    registry.register_component<Collider>();
    registry.register_component<Controllable>();
    registry.register_component<Projectile>();
    registry.register_component<Enemy>();
    registry.register_component<Wall>();
    
    // Enregistrer le système de collision
    registry.register_system<CollisionSystem>();
    
    // Créer le plugin d'input
    RaylibInputPlugin inputPlugin;
    if (!inputPlugin.initialize()) {
        std::cerr << "❌ Erreur lors de l'initialisation du plugin" << std::endl;
        CloseWindow();
        return 1;
    }
    
    // Enregistrer l'InputSystem avec le plugin
    registry.register_system<InputSystem>(&inputPlugin);
    
    std::cout << "✓ Input Plugin, InputSystem et CollisionSystem enregistrés" << std::endl;
    std::cout << std::endl;
    
    // === CRÉER LE JOUEUR ===
    Entity player = registry.spawn_entity();
    registry.add_component<Position>(player, Position{100.0f, 300.0f});
    registry.add_component<Velocity>(player, Velocity{0.0f, 0.0f});
    registry.add_component<Input>(player, Input{});
    registry.add_component<Collider>(player, Collider{30.0f, 30.0f});
    registry.add_component<Controllable>(player, Controllable{});
    
    std::cout << "✓ Joueur créé (Bleu)" << std::endl;
    
    // === CRÉER LES ENNEMIS ===
    Entity enemy1 = registry.spawn_entity();
    registry.add_component<Position>(enemy1, Position{400.0f, 150.0f});
    registry.add_component<Collider>(enemy1, Collider{35.0f, 35.0f});
    registry.add_component<Enemy>(enemy1, Enemy{});
    
    Entity enemy2 = registry.spawn_entity();
    registry.add_component<Position>(enemy2, Position{600.0f, 300.0f});
    registry.add_component<Collider>(enemy2, Collider{35.0f, 35.0f});
    registry.add_component<Enemy>(enemy2, Enemy{});
    
    Entity enemy3 = registry.spawn_entity();
    registry.add_component<Position>(enemy3, Position{400.0f, 450.0f});
    registry.add_component<Collider>(enemy3, Collider{35.0f, 35.0f});
    registry.add_component<Enemy>(enemy3, Enemy{});
    
    std::cout << "✓ 3 Ennemis créés (Rouges)" << std::endl;
    
    // === CRÉER LES MURS ===
    Entity wall1 = registry.spawn_entity();
    registry.add_component<Position>(wall1, Position{250.0f, 100.0f});
    registry.add_component<Collider>(wall1, Collider{20.0f, 400.0f});
    registry.add_component<Wall>(wall1, Wall{});
    
    Entity wall2 = registry.spawn_entity();
    registry.add_component<Position>(wall2, Position{550.0f, 100.0f});
    registry.add_component<Collider>(wall2, Collider{20.0f, 400.0f});
    registry.add_component<Wall>(wall2, Wall{});
    
    std::cout << "✓ 2 Murs créés (Gris)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== Contrôles ===" << std::endl;
    std::cout << "  WASD ou Flèches : Déplacer le joueur" << std::endl;
    std::cout << "  ESPACE          : Tirer un projectile" << std::endl;
    std::cout << "  ESC             : Quitter" << std::endl;
    std::cout << std::endl;
    std::cout << "🎯 Tire sur les ennemis rouges ! Les murs te bloquent." << std::endl;
    std::cout << std::endl;
    
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& inputs = registry.get_components<Input>();
    auto& colliders = registry.get_components<Collider>();
    
    float speed = 3.0f;
    float shootCooldown = 0.0f;
    int enemiesKilled = 0;
    
    // Boucle de jeu
    while (!WindowShouldClose()) {
        // === UPDATE ===
        
        // 1. Exécuter tous les systèmes (InputSystem met à jour les inputs, CollisionSystem gère les collisions)
        registry.run_systems(GetFrameTime());
        
        // 2. Appliquer le mouvement au joueur (basé sur le composant Input mis à jour par InputSystem)
        auto& playerInput = inputs[player];
        auto& playerPos = positions[player];
        if (playerInput.up) playerPos.y -= speed;
        if (playerInput.down) playerPos.y += speed;
        if (playerInput.left) playerPos.x -= speed;
        if (playerInput.right) playerPos.x += speed;
        
        // Garder le joueur dans l'écran
        float playerRadius = 15.0f;
        if (playerPos.x < playerRadius) playerPos.x = playerRadius;
        if (playerPos.x > screenWidth - playerRadius) playerPos.x = screenWidth - playerRadius;
        if (playerPos.y < playerRadius) playerPos.y = playerRadius;
        if (playerPos.y > screenHeight - playerRadius) playerPos.y = screenHeight - playerRadius;
        
        // 3. Tirer des projectiles
        shootCooldown -= GetFrameTime();
        if (playerInput.fire && shootCooldown <= 0.0f) {
            Entity projectile = registry.spawn_entity();
            registry.add_component<Position>(projectile, Position{playerPos.x + 20.0f, playerPos.y});
            registry.add_component<Velocity>(projectile, Velocity{8.0f, 0.0f});
            registry.add_component<Collider>(projectile, Collider{10.0f, 5.0f});
            registry.add_component<Projectile>(projectile, Projectile{});
            shootCooldown = 0.3f; // 300ms de cooldown
        }
        
        // 4. Déplacer les projectiles
        {
            auto& projectiles = registry.get_components<Projectile>();
            for (size_t i = 0; i < projectiles.size(); i++) {
                Entity proj = projectiles.get_entity_at(i);
                if (positions.has_entity(proj) && velocities.has_entity(proj)) {
                    auto& projPos = positions.get_data_by_entity_id(proj);
                    auto& projVel = velocities.get_data_by_entity_id(proj);
                    projPos.x += projVel.x;
                    projPos.y += projVel.y;
                    
                    // Détruire les projectiles hors écran
                    if (projPos.x > screenWidth + 50) {
                        registry.kill_entity(proj);
                    }
                }
            }
        }
        
        // === RENDER ===
        
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Dessiner le titre
        DrawText("Test Collision R-Type", 10, 10, 20, DARKGRAY);
        
        // Instructions
        DrawText("WASD/Fleches: Deplacer | ESPACE: Tirer | ESC: Quitter", 10, 35, 14, DARKGRAY);
        
        // Stats
        auto& enemies = registry.get_components<Enemy>();
        int remainingEnemies = 0;
        for (size_t i = 0; i < enemies.size(); i++) {
            if (enemies.has_entity(enemies.get_entity_at(i))) remainingEnemies++;
        }
        DrawText(TextFormat("Ennemis restants: %d/3", remainingEnemies), 10, 55, 16, RED);
        
        // Dessiner les murs (rectangles gris)
        auto& walls = registry.get_components<Wall>();
        for (size_t i = 0; i < walls.size(); i++) {
            Entity wall = walls.get_entity_at(i);
            if (positions.has_entity(wall) && colliders.has_entity(wall)) {
                const Position& wallPos = positions.get_data_by_entity_id(wall);
                const Collider& wallCol = colliders.get_data_by_entity_id(wall);
                DrawRectangle(
                    (int)wallPos.x, 
                    (int)wallPos.y,
                    (int)wallCol.width, 
                    (int)wallCol.height, 
                    GRAY
                );
            }
        }
        
        // Dessiner les ennemis (cercles rouges)
        for (size_t i = 0; i < enemies.size(); i++) {
            Entity enemy = enemies.get_entity_at(i);
            if (positions.has_entity(enemy)) {
                const Position& enemyPos = positions.get_data_by_entity_id(enemy);
                DrawCircle((int)enemyPos.x, (int)enemyPos.y, 17.5f, RED);
                DrawCircleLines((int)enemyPos.x, (int)enemyPos.y, 17.5f, MAROON);
                DrawText("E", (int)enemyPos.x - 5, (int)enemyPos.y - 6, 15, WHITE);
            }
        }
        
        // Dessiner les projectiles (petits cercles jaunes)
        auto& projectiles = registry.get_components<Projectile>();
        for (size_t i = 0; i < projectiles.size(); i++) {
            Entity proj = projectiles.get_entity_at(i);
            if (positions.has_entity(proj)) {
                const Position& projPos = positions.get_data_by_entity_id(proj);
                DrawCircle((int)projPos.x, (int)projPos.y, 5.0f, YELLOW);
            }
        }
        
        // Dessiner le joueur (cercle bleu)
        DrawCircle((int)playerPos.x, (int)playerPos.y, playerRadius, BLUE);
        DrawCircleLines((int)playerPos.x, (int)playerPos.y, playerRadius, DARKBLUE);
        DrawText("P", (int)playerPos.x - 5, (int)playerPos.y - 6, 15, WHITE);
        
        // Message de victoire
        if (remainingEnemies == 0) {
            DrawRectangle(screenWidth / 2 - 150, screenHeight / 2 - 40, 300, 80, Fade(GREEN, 0.9f));
            DrawText("VICTOIRE !", screenWidth / 2 - 80, screenHeight / 2 - 30, 30, WHITE);
            DrawText("Tous les ennemis elimines!", screenWidth / 2 - 120, screenHeight / 2 + 10, 16, WHITE);
        }
        
        EndDrawing();
    }
    
    // Nettoyage
    inputPlugin.shutdown();
    CloseWindow();
    
    std::cout << "=== Fin du test ===" << std::endl;
    
    return 0;
}
