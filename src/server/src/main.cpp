#include <iostream>
#include <cmath> // Pour std::abs (comparaison de float)
#include "ecs/Registry.hpp"
#include "ecs/systems/CollisionSystem.hpp"
#include "ecs/Components.hpp"

// Petite fonction utilitaire pour comparer des floats sans erreurs d'arrondi
bool is_approx(float a, float b) {
    return std::abs(a - b) < 0.001f;
}

void print_test_result(const std::string& testName, bool success, float got, float expected) {
    if (success) {
        std::cout << "✅ " << testName << " [OK]" << std::endl;
    } else {
        std::cout << "❌ " << testName << " [ECHEC]" << std::endl;
        std::cout << "   -> Attendu: " << expected << " | Obtenu: " << got << std::endl;
    }
}

int main() {
    Registry registry;
    CollisionSystem collisionSystem;

    // 1. Enregistrement
    registry.register_component<Position>();
    registry.register_component<Velocity>(); // Pas utilisé ici mais bon
    registry.register_component<Collider>();
    registry.register_component<Controllable>();
    registry.register_component<Wall>();
    
    // Pour éviter les crashs si tu as laissé les autres checks
    registry.register_component<Projectile>(); 
    registry.register_component<Enemy>();

    std::cout << "=== TEST PHYSIQUE : JOUEUR vs MUR (4 DIRECTIONS) ===" << std::endl;

    // CONFIGURATION DU MUR CENTRAL
    // Position: 100, 100
    // Taille:   50, 50
    // Bounding Box: X[100 à 150], Y[100 à 150]
    Entity wall = registry.spawn_entity();
    registry.add_component(wall, Position{100.0f, 100.0f});
    registry.add_component(wall, Collider{50.0f, 50.0f});
    registry.add_component(wall, Wall{});

    // Taille du Joueur pour tous les tests : 10x10
    float pSize = 10.0f;

    // ==========================================================
    // ⬅️ TEST 1 : GAUCHE (Le joueur rentre par la gauche)
    // ==========================================================
    // On place le joueur à X=92. 
    // Son bord droit est à 92 + 10 = 102.
    // Le mur commence à 100.
    // -> Chevauchement de 2 pixels.
    // -> Résultat attendu : Repoussé à X=90.
    {
        Entity pLeft = registry.spawn_entity();
        registry.add_component(pLeft, Position{92.0f, 120.0f}); // Y=120 (au milieu du mur en hauteur)
        registry.add_component(pLeft, Collider{pSize, pSize});
        registry.add_component(pLeft, Controllable{});

        collisionSystem.update(registry);

        auto& pos = registry.get_components<Position>();
        print_test_result("Collision GAUCHE", is_approx(pos[pLeft].x, 90.0f), pos[pLeft].x, 90.0f);
        
        registry.kill_entity(pLeft); // Nettoyage
    }

    // ==========================================================
    // ➡️ TEST 2 : DROITE (Le joueur rentre par la droite)
    // ==========================================================
    // Le mur finit à X=150.
    // On place le joueur à X=148.
    // Son bord gauche est à 148 (avant la fin du mur).
    // -> Chevauchement de 2 pixels.
    // -> Résultat attendu : Repoussé à X=150.
    {
        Entity pRight = registry.spawn_entity();
        registry.add_component(pRight, Position{148.0f, 120.0f});
        registry.add_component(pRight, Collider{pSize, pSize});
        registry.add_component(pRight, Controllable{});

        collisionSystem.update(registry);

        auto& pos = registry.get_components<Position>();
        print_test_result("Collision DROITE", is_approx(pos[pRight].x, 150.0f), pos[pRight].x, 150.0f);
        
        registry.kill_entity(pRight);
    }

    // ==========================================================
    // ⬆️ TEST 3 : HAUT (Le joueur rentre par le haut)
    // ==========================================================
    // Le mur commence à Y=100.
    // On place le joueur à Y=92.
    // Son bord bas est à 92 + 10 = 102.
    // -> Chevauchement de 2 pixels.
    // -> Résultat attendu : Repoussé à Y=90.
    {
        Entity pTop = registry.spawn_entity();
        registry.add_component(pTop, Position{120.0f, 92.0f}); // X=120 (milieu largeur)
        registry.add_component(pTop, Collider{pSize, pSize});
        registry.add_component(pTop, Controllable{});

        collisionSystem.update(registry);

        auto& pos = registry.get_components<Position>();
        print_test_result("Collision HAUT  ", is_approx(pos[pTop].y, 90.0f), pos[pTop].y, 90.0f);
        
        registry.kill_entity(pTop);
    }

    // ==========================================================
    // ⬇️ TEST 4 : BAS (Le joueur rentre par le bas)
    // ==========================================================
    // Le mur finit à Y=150.
    // On place le joueur à Y=148.
    // Son bord haut est à 148.
    // -> Chevauchement de 2 pixels.
    // -> Résultat attendu : Repoussé à Y=150.
    {
        Entity pBot = registry.spawn_entity();
        registry.add_component(pBot, Position{120.0f, 148.0f});
        registry.add_component(pBot, Collider{pSize, pSize});
        registry.add_component(pBot, Controllable{});

        collisionSystem.update(registry);

        auto& pos = registry.get_components<Position>();
        print_test_result("Collision BAS   ", is_approx(pos[pBot].y, 150.0f), pos[pBot].y, 150.0f);
        
        registry.kill_entity(pBot);
    }

    return 0;
}