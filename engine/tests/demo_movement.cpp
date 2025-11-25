/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** TEST MOUVEMENT - Simple et clair
*/

#include "../systems/temp/TempRegistry.hpp"
#include "../include/core/event/EventBus.hpp"
#include "../systems/temp/TempComponents.hpp"
#include "../systems/temp/TempEvents.hpp"
#include "../systems/InputSystem.hpp"
#include "../systems/MovementSystem.hpp"
#include "../systems/PhysicsSystem.hpp"
#include "MockPlugins.hpp"
#include <iostream>

using namespace rtype;

int main() {
    std::cout << "\n╔═══════════════════════════════════════╗\n";
    std::cout << "║    TEST DU SYSTÈME DE MOUVEMENT      ║\n";
    std::cout << "╚═══════════════════════════════════════╝\n\n";

    // ═══════════════════════════════════════════════════════════════
    // 1. INITIALISATION
    // ═══════════════════════════════════════════════════════════════
    TempRegistry registry;
    core::EventBus event_bus;
    MockInputPlugin input;
    input.initialize();

    // TEST EVENTBUS : Abonnement à un événement de collision
    int collision_count = 0;
    event_bus.subscribe<CollisionEvent>([&collision_count](const CollisionEvent& e) {
        collision_count++;
        std::cout << "🔔 EventBus : Collision détectée entre entités " 
                  << e.entity_a << " et " << e.entity_b << "\n";
    });

    std::cout << "✅ EventBus initialisé avec 1 subscriber CollisionEvent\n";
    std::cout << "   Nombre de subscribers : " << event_bus.getSubscriberCount<CollisionEvent>() << "\n\n";

    InputSystem input_system(registry, event_bus, &input);
    MovementSystem movement_system(registry, event_bus);
    PhysicsSystem physics_system(registry, event_bus);
    physics_system.set_world_bounds(0.0f, 0.0f, 800.0f, 600.0f);

    EntityId player = registry.create_entity();
    registry.add_component(player, TransformComponent{100.0f, 100.0f});
    registry.add_component(player, VelocityComponent{});
    registry.add_component(player, InputComponent{});

    std::cout << "Position initiale : (100, 100)\n\n";

    // ═══════════════════════════════════════════════════════════════
    // 2. TEST MOUVEMENT DROITE
    // ═══════════════════════════════════════════════════════════════
    std::cout << "Test : Appui sur D (droite) pendant 1 seconde\n\n";
    input.simulate_key_press(Key::D, true);

    for (int frame = 0; frame < 60; ++frame) {
        input_system.update(1.0f / 60.0f);
        movement_system.update(1.0f / 60.0f);
        physics_system.update(1.0f / 60.0f);
    }

    input.simulate_key_press(Key::D, false);
    
    auto transform = registry.get_component<TransformComponent>(player);
    std::cout << "Position finale : (" 
              << transform.value()->position.x << ", "
              << transform.value()->position.y << ")\n\n";

    // ═══════════════════════════════════════════════════════════════
    // 3. TEST EVENTBUS - Publication immédiate
    // ═══════════════════════════════════════════════════════════════
    std::cout << "Test EventBus : Publication d'événements\n";
    
    // Publier un événement immédiat
    event_bus.publish(CollisionEvent{player, 999, {200.0f, 150.0f}, {0.0f, 1.0f}});
    
    // Publier un événement différé
    event_bus.publish_deferred(CollisionEvent{player, 888, {250.0f, 200.0f}, {1.0f, 0.0f}});
    std::cout << "   Événements différés en attente : " << event_bus.getDeferredEventCount() << "\n";
    
    // Traiter les événements différés
    event_bus.process_deferred();
    std::cout << "   Événements différés traités : " << event_bus.getDeferredEventCount() << " restant\n\n";

    // ═══════════════════════════════════════════════════════════════
    // 4. RÉSULTATS
    // ═══════════════════════════════════════════════════════════════
    bool moved = (transform.value()->position.x != 100.0f);
    bool eventbus_works = (collision_count == 2);
    
    std::cout << "╔═══════════════════════════════════════╗\n";
    std::cout << "║            RÉSULTATS                  ║\n";
    std::cout << "╚═══════════════════════════════════════╝\n\n";
    std::cout << "✓ Mouvement : Le joueur s'est déplacé de " 
              << (transform.value()->position.x - 100.0f) << " pixels\n";
    std::cout << "✓ EventBus  : " << collision_count << " événements reçus (attendu: 2)\n\n";
    
    if (moved && eventbus_works) {
        std::cout << "🎉 TOUS LES TESTS PASSENT !\n\n";
        return 0;
    } else {
        if (!moved) std::cout << "❌ Échec : Le joueur n'a pas bougé\n";
        if (!eventbus_works) std::cout << "❌ Échec : EventBus n'a pas fonctionné correctement\n";
        std::cout << "\n";
        return 1;
    }
}
