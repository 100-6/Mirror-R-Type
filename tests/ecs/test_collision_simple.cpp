#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "systems/CollisionSystem.hpp"
#include <iostream>
#include <cmath>

int main() {
    std::cout << "=== Test Simple ECS - Collision entre 2 joueurs ===" << std::endl;
    std::cout << std::endl;

    // Créer le Registry
    Registry registry;

    // Enregistrer les composants
    registry.register_component<Position>();
    registry.register_component<Collider>();
    registry.register_component<Controllable>();  // Tag pour identifier les joueurs

    std::cout << "✓ Registry initialisé" << std::endl;
    std::cout << std::endl;

    // === CRÉATION DES JOUEURS ===
    
    // Player A - commence à (0, 0)
    Entity playerA = registry.spawn_entity();
    registry.add_component<Position>(playerA, Position{0.0f, 0.0f});
    registry.add_component<Collider>(playerA, Collider{1.0f, 1.0f});
    registry.add_component<Controllable>(playerA, Controllable{});

    std::cout << "✓ Player A créé à la position (0, 0)" << std::endl;

    // Player B - statique à (2, 0)
    Entity playerB = registry.spawn_entity();
    registry.add_component<Position>(playerB, Position{2.0f, 0.0f});
    registry.add_component<Collider>(playerB, Collider{1.0f, 1.0f});
    registry.add_component<Controllable>(playerB, Controllable{});

    std::cout << "✓ Player B créé à la position (2, 0)" << std::endl;
    std::cout << std::endl;

    // === SIMULATION ===
    std::cout << "=== Début de la simulation ===" << std::endl;
    std::cout << std::endl;

    // Obtenir les positions
    auto& positions = registry.get_components<Position>();

    // Déplacement 1 : Player A avance d'une case
    std::cout << "--- Déplacement 1 : Player A avance d'une case ---" << std::endl;
    positions[playerA].x += 1.0f;
    
    std::cout << "Player A position: (" << positions[playerA].x << ", " << positions[playerA].y << ")" << std::endl;
    std::cout << "Player B position: (" << positions[playerB].x << ", " << positions[playerB].y << ")" << std::endl;
    std::cout << std::endl;

    // Déplacement 2 : Player A avance encore d'une case - COLLISION !
    std::cout << "--- Déplacement 2 : Player A avance encore d'une case ---" << std::endl;
    positions[playerA].x += 1.0f;
    
    std::cout << "Player A position: (" << positions[playerA].x << ", " << positions[playerA].y << ")" << std::endl;
    std::cout << "Player B position: (" << positions[playerB].x << ", " << positions[playerB].y << ")" << std::endl;
    std::cout << std::endl;

    // Vérification de la collision avec le CollisionSystem
    std::cout << "=== Vérification de collision avec CollisionSystem ===" << std::endl;
    
    CollisionSystem collisionSystem;
    
    int collisionCount = 0;
    collisionSystem.scan_collisions<Controllable, Controllable>(registry, 
        [&](Entity e1, Entity e2) {
            collisionCount++;
            std::cout << "💥 COLLISION DÉTECTÉE par le système !" << std::endl;
            std::cout << "  - Player A (Entity " << e1 << ") position: (" 
                      << positions[e1].x << ", " << positions[e1].y << ")" << std::endl;
            std::cout << "  - Player B (Entity " << e2 << ") position: (" 
                      << positions[e2].x << ", " << positions[e2].y << ")" << std::endl;
        }
    );
    
    if (collisionCount == 0) {
        std::cout << "❌ Pas de collision détectée" << std::endl;
    }
    std::cout << std::endl;

    std::cout << "=== Fin de la simulation ===" << std::endl;

    return 0;
}
