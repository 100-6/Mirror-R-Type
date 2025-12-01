/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Test Input Plugin with Raylib
*/

#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "ecs/systems/InputSystem.hpp"
#include "plugins/input/raylib/RaylibInputPlugin.hpp"
#include <raylib.h>
#include <iostream>

int main() {
    // Initialisation de Raylib
    const int screenWidth = 800;
    const int screenHeight = 600;
    
    InitWindow(screenWidth, screenHeight, "Test Input Plugin - R-Type");
    SetTargetFPS(60);
    
    std::cout << "=== Test Input Plugin avec Raylib ===" << std::endl;
    std::cout << std::endl;
    
    // Créer le Registry
    Registry registry;
    registry.register_component<Position>();
    registry.register_component<Input>();
    registry.register_component<Controllable>();
    
    // Créer le plugin d'input
    RaylibInputPlugin inputPlugin;
    if (!inputPlugin.initialize()) {
        std::cerr << "❌ Erreur lors de l'initialisation du plugin" << std::endl;
        CloseWindow();
        return 1;
    }
    
    std::cout << "✓ Input Plugin initialisé" << std::endl;
    
    // Enregistrer l'InputSystem
    registry.register_system<InputSystem>(&inputPlugin);
    std::cout << "✓ InputSystem enregistré" << std::endl;
    std::cout << std::endl;
    
    // Créer un joueur
    Entity player = registry.spawn_entity();
    registry.add_component<Position>(player, Position{400.0f, 300.0f});
    registry.add_component<Input>(player, Input{});
    registry.add_component<Controllable>(player, Controllable{});
    
    std::cout << "✓ Joueur créé au centre de l'écran" << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== Contrôles ===" << std::endl;
    std::cout << "  WASD ou Flèches : Déplacement" << std::endl;
    std::cout << "  Espace ou Clic : Tirer" << std::endl;
    std::cout << "  Shift : Action spéciale" << std::endl;
    std::cout << "  ESC : Quitter" << std::endl;
    std::cout << std::endl;
    
    auto& positions = registry.get_components<Position>();
    auto& inputs = registry.get_components<Input>();
    
    float speed = 5.0f;
    
    // Boucle de jeu
    while (!WindowShouldClose()) {
        // === UPDATE ===
        
        // 1. Exécuter tous les systèmes (InputSystem met à jour les composants Input)
        registry.run_systems();
        
        // 2. Appliquer le mouvement basé sur l'input
        auto& input = inputs[player];
        auto& pos = positions[player];
        
        if (input.up) pos.y -= speed;
        if (input.down) pos.y += speed;
        if (input.left) pos.x -= speed;
        if (input.right) pos.x += speed;
        
        // Garder le joueur dans l'écran
        if (pos.x < 0) pos.x = 0;
        if (pos.x > screenWidth) pos.x = screenWidth;
        if (pos.y < 0) pos.y = 0;
        if (pos.y > screenHeight) pos.y = screenHeight;
        
        // === RENDER ===
        
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Dessiner le joueur
        Color playerColor = BLUE;
        if (input.fire) playerColor = RED;       // Rouge si on tire
        if (input.special) playerColor = GOLD;   // Doré si action spéciale
        
        DrawCircle((int)pos.x, (int)pos.y, 20.0f, playerColor);
        
        // Dessiner les instructions
        DrawText("WASD/Arrows: Move", 10, 10, 20, DARKGRAY);
        DrawText("Space/Click: Fire (RED)", 10, 35, 20, DARKGRAY);
        DrawText("Shift: Special (GOLD)", 10, 60, 20, DARKGRAY);
        DrawText("ESC: Quit", 10, 85, 20, DARKGRAY);
        
        // Afficher la position
        DrawText(TextFormat("Position: (%.0f, %.0f)", pos.x, pos.y), 10, screenHeight - 30, 20, DARKGREEN);
        
        // Afficher l'état des inputs
        std::string inputState = "Input: ";
        if (input.up) inputState += "UP ";
        if (input.down) inputState += "DOWN ";
        if (input.left) inputState += "LEFT ";
        if (input.right) inputState += "RIGHT ";
        if (input.fire) inputState += "FIRE ";
        if (input.special) inputState += "SPECIAL ";
        
        DrawText(inputState.c_str(), 10, screenHeight - 60, 20, DARKBLUE);
        
        EndDrawing();
    }
    
    // Nettoyage
    inputPlugin.shutdown();
    CloseWindow();
    
    std::cout << "=== Fin du test ===" << std::endl;
    
    return 0;
}
