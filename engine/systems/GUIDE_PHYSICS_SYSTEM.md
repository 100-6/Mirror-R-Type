# ⚙️ PhysicsSystem - Documentation Complète

## 📋 Vue d'ensemble

Le **PhysicsSystem** est responsable d'**appliquer la vélocité à la position** et de gérer les **limites du monde** (world bounds).

### Rôle dans le pipeline

```
VelocityComponent → PhysicsSystem → TransformComponent
   (Vitesse)         (Applique)       (Position)
```

**En une phrase** : "Il déplace les entités en appliquant leur vitesse et les empêche de sortir des limites."

---

## 🏗️ Déclaration

### Fichier
`engine/systems/PhysicsSystem.hpp`

### Signature de la classe

```cpp
class PhysicsSystem {
public:
    PhysicsSystem(TempRegistry& registry, core::EventBus& event_bus);
    
    void update(float delta_time);
    
    void set_world_bounds(float min_x, float min_y, 
                          float max_x, float max_y);

private:
    TempRegistry& registry_;
    core::EventBus& event_bus_;
    float world_min_x_;
    float world_min_y_;
    float world_max_x_;
    float world_max_y_;
};
```

---

## 🔧 Utilisation

### 1. Création du système

```cpp
#include "PhysicsSystem.hpp"
#include "temp/TempRegistry.hpp"
#include "../include/core/event/EventBus.hpp"

// Setup
TempRegistry registry;
core::EventBus event_bus;

// Créer le système
PhysicsSystem physics_system(registry, event_bus);

// Configurer les limites du monde (optionnel)
physics_system.set_world_bounds(0.0f, 0.0f, 1920.0f, 1080.0f);
```

### 2. Créer une entité avec les composants requis

```cpp
// Créer une entité
EntityId entity = registry.create_entity();

// Ajouter TransformComponent (position)
registry.add_component(entity, TransformComponent{100.0f, 100.0f});

// Ajouter VelocityComponent (vitesse)
registry.add_component(entity, VelocityComponent{});

// Le système peut maintenant traiter cette entité
```

### 3. Mettre à jour le système chaque frame

```cpp
float delta_time = 1.0f / 60.0f;  // 60 FPS

// Dans la boucle de jeu
while (running) {
    input_system.update(delta_time);
    movement_system.update(delta_time);
    
    // PhysicsSystem doit être appelé APRÈS MovementSystem
    physics_system.update(delta_time);
    
    render_system.update(delta_time);
}
```

**⚠️ IMPORTANT** : PhysicsSystem doit être appelé **APRÈS** MovementSystem !

---

## 🔄 Fonctionnement détaillé

### Méthode `update(float delta_time)`

```cpp
void update(float delta_time) {
    // 1️⃣ Récupérer toutes les entités avec Transform ET Velocity
    auto entities = registry_.get_entities_with<TransformComponent, VelocityComponent>();
    
    // 2️⃣ Pour chaque entité
    for (EntityId entity : entities) {
        auto transform_opt = registry_.get_component<TransformComponent>(entity);
        auto velocity_opt = registry_.get_component<VelocityComponent>(entity);
        
        if (!transform_opt.has_value() || !velocity_opt.has_value()) {
            continue;
        }
        
        auto* transform = transform_opt.value();
        auto* velocity = velocity_opt.value();
        
        // 3️⃣ Appliquer la vitesse à la position
        transform->position.x += velocity->velocity.x * delta_time;
        transform->position.y += velocity->velocity.y * delta_time;
        
        // 4️⃣ Gérer les limites du monde
        // Limite gauche
        if (transform->position.x < world_min_x_) {
            transform->position.x = world_min_x_;
            velocity->velocity.x = 0.0f;  // Stop horizontal
        }
        
        // Limite droite
        if (transform->position.x > world_max_x_) {
            transform->position.x = world_max_x_;
            velocity->velocity.x = 0.0f;  // Stop horizontal
        }
        
        // Limite haut
        if (transform->position.y < world_min_y_) {
            transform->position.y = world_min_y_;
            velocity->velocity.y = 0.0f;  // Stop vertical
        }
        
        // Limite bas
        if (transform->position.y > world_max_y_) {
            transform->position.y = world_max_y_;
            velocity->velocity.y = 0.0f;  // Stop vertical
        }
    }
}
```

### Étapes d'exécution

1. **Récupérer** toutes les entités avec TransformComponent ET VelocityComponent
2. **Pour chaque entité** :
   - Obtenir les deux composants
   - **Appliquer la vitesse** : `position += velocity * delta_time`
   - **Vérifier les limites** : Si hors limites, ramener à la limite et arrêter
3. **Répéter** pour la frame suivante

---

## 🎯 Exemple complet

### Code minimal

```cpp
#include "PhysicsSystem.hpp"
#include "temp/TempRegistry.hpp"
#include "temp/TempComponents.hpp"
#include "../include/core/event/EventBus.hpp"
#include <iostream>

int main() {
    // 1. Setup
    TempRegistry registry;
    core::EventBus event_bus;
    PhysicsSystem physics_system(registry, event_bus);
    
    // 2. Configurer les limites (0-800 en X, 0-600 en Y)
    physics_system.set_world_bounds(0.0f, 0.0f, 800.0f, 600.0f);
    
    // 3. Créer une entité
    EntityId entity = registry.create_entity();
    registry.add_component(entity, TransformComponent{400.0f, 300.0f});
    registry.add_component(entity, VelocityComponent{});
    
    // 4. Donner une vitesse
    auto velocity = registry.get_component<VelocityComponent>(entity).value();
    velocity->velocity.x = 200.0f;  // 200 pixels/seconde vers la droite
    velocity->velocity.y = 0.0f;
    
    // 5. Simuler 60 frames (1 seconde)
    float delta_time = 1.0f / 60.0f;
    for (int i = 0; i < 60; i++) {
        physics_system.update(delta_time);
    }
    
    // 6. Vérifier la position finale
    auto transform = registry.get_component<TransformComponent>(entity).value();
    std::cout << "Position finale: (" 
              << transform->position.x << ", " 
              << transform->position.y << ")\n";
    
    // Résultat attendu: (600.0, 300.0)
    // Car: 400 + (200 * 1.0) = 600
    
    return 0;
}
```

### Résultat

```
Position finale: (600.0, 300.0)
```

---

## 🌍 Limites du monde (World Bounds)

### Méthode `set_world_bounds()`

```cpp
void set_world_bounds(float min_x, float min_y, float max_x, float max_y) {
    world_min_x_ = min_x;
    world_min_y_ = min_y;
    world_max_x_ = max_x;
    world_max_y_ = max_y;
}
```

### Valeurs par défaut

```cpp
// Dans le constructeur
PhysicsSystem::PhysicsSystem(TempRegistry& registry, core::EventBus& event_bus)
    : registry_(registry)
    , event_bus_(event_bus)
    , world_min_x_(0.0f)
    , world_min_y_(0.0f)
    , world_max_x_(1920.0f)  // Full HD par défaut
    , world_max_y_(1080.0f)
{}
```

### Exemples de configuration

```cpp
// Écran Full HD
physics_system.set_world_bounds(0.0f, 0.0f, 1920.0f, 1080.0f);

// Écran 720p
physics_system.set_world_bounds(0.0f, 0.0f, 1280.0f, 720.0f);

// Grille 4x4 (100px par case)
physics_system.set_world_bounds(0.0f, 0.0f, 300.0f, 300.0f);

// Monde infini en X, limité en Y
physics_system.set_world_bounds(-999999.0f, 0.0f, 999999.0f, 600.0f);

// Monde centré sur l'origine
physics_system.set_world_bounds(-400.0f, -300.0f, 400.0f, 300.0f);
```

---

## 📐 Calcul du mouvement

### Formule de base

```cpp
position_new = position_old + (velocity * delta_time)
```

### Exemples de calcul

#### Exemple 1 : Mouvement horizontal

```cpp
// État initial
position.x = 100.0f
velocity.x = 300.0f  // pixels/seconde
delta_time = 1.0f / 60.0f  // 16.67 ms

// Calcul
position.x += velocity.x * delta_time
position.x += 300.0f * 0.01667f
position.x += 5.0f

// Résultat
position.x = 105.0f
```

**Interprétation** : À 300 px/s et 60 FPS, l'entité avance de 5 pixels par frame.

#### Exemple 2 : Mouvement diagonal

```cpp
// État initial
position.x = 0.0f
position.y = 0.0f
velocity.x = 200.0f
velocity.y = 200.0f
delta_time = 1.0f / 60.0f

// Après 60 frames (1 seconde)
for (int i = 0; i < 60; i++) {
    position.x += velocity.x * delta_time;
    position.y += velocity.y * delta_time;
}

// Résultat
position.x = 200.0f
position.y = 200.0f
```

**Distance parcourue** : `√(200² + 200²) ≈ 283 pixels`

#### Exemple 3 : Collision avec limite

```cpp
// État initial
position.x = 750.0f
velocity.x = 300.0f
world_max_x_ = 800.0f
delta_time = 1.0f / 60.0f

// Frame 1
position.x += 300.0f * 0.01667f = 755.0f  ✅ OK

// Frame 2
position.x += 300.0f * 0.01667f = 760.0f  ✅ OK

// ...

// Frame 11
position.x += 300.0f * 0.01667f = 805.0f  ❌ Dépasse !

// Correction
if (position.x > world_max_x_) {
    position.x = world_max_x_;  // Ramener à 800.0f
    velocity.x = 0.0f;           // Arrêter
}

// Résultat
position.x = 800.0f
velocity.x = 0.0f
```

---

## 🚧 Gestion des collisions

### Comportement actuel

Quand une entité touche une limite :

1. **Position** : Ramenée exactement à la limite
2. **Vitesse** : Mise à 0 dans la direction de la collision

```cpp
// Collision mur droit
if (position.x > world_max_x_) {
    position.x = world_max_x_;    // Ramener
    velocity.x = 0.0f;             // Stop
    // velocity.y reste inchangé !
}
```

### Exemple visuel

```
Avant collision:
position = (805, 300)
velocity = (300, 0)

Après collision:
position = (800, 300)  ← Ramené à la limite
velocity = (0, 0)      ← Arrêté
```

### Problème potentiel

Si l'entité est dans un coin et appuie contre deux murs :

```cpp
// Coin bas-droit
position = (800, 600)
velocity = (100, 100)  // Essaie d'aller en bas-droite

// Après PhysicsSystem
position = (800, 600)  // Bloqué
velocity = (0, 0)      // Complètement arrêté

// L'entité ne peut plus bouger même si le joueur appuie sur une autre touche !
```

**Solution** : Ne mettre à 0 que la composante qui touche le mur (voir section Améliorations).

---

## 🔗 Intégration avec les autres systèmes

### Flux de données complet

```
Frame N:

InputSystem:
    input->move_right = true

MovementSystem:
    velocity.x = 300.0f  (calcule)

PhysicsSystem:
    position.x += 300 * dt  (applique)

RenderSystem:
    draw(position.x, position.y)  (affiche)
```

### Ordre d'exécution CRITIQUE

```cpp
// ✅ BON ORDRE
input_system.update(dt);      // 1. Lit
movement_system.update(dt);   // 2. Calcule
physics_system.update(dt);    // 3. Applique ← ICI
render_system.update(dt);     // 4. Affiche

// ❌ MAUVAIS ORDRE
physics_system.update(dt);    // Applique l'ancienne vitesse !
movement_system.update(dt);   // Calcule la nouvelle vitesse trop tard
```

---

## 🧪 Tests

### Test 1 : Mouvement simple

```cpp
TEST(PhysicsSystem, AppliesVelocity) {
    TempRegistry registry;
    core::EventBus event_bus;
    PhysicsSystem system(registry, event_bus);
    
    EntityId entity = registry.create_entity();
    registry.add_component(entity, TransformComponent{0.0f, 0.0f});
    registry.add_component(entity, VelocityComponent{});
    
    auto velocity = registry.get_component<VelocityComponent>(entity).value();
    velocity->velocity.x = 100.0f;
    
    // 1 seconde à 60 FPS
    for (int i = 0; i < 60; i++) {
        system.update(1.0f / 60.0f);
    }
    
    auto transform = registry.get_component<TransformComponent>(entity).value();
    
    ASSERT_NEAR(transform->position.x, 100.0f, 0.1f);
}
```

### Test 2 : Collision mur droit

```cpp
TEST(PhysicsSystem, ClampsToRightBound) {
    TempRegistry registry;
    core::EventBus event_bus;
    PhysicsSystem system(registry, event_bus);
    system.set_world_bounds(0.0f, 0.0f, 100.0f, 100.0f);
    
    EntityId entity = registry.create_entity();
    registry.add_component(entity, TransformComponent{50.0f, 50.0f});
    registry.add_component(entity, VelocityComponent{});
    
    auto velocity = registry.get_component<VelocityComponent>(entity).value();
    velocity->velocity.x = 1000.0f;  // Très rapide
    
    // Beaucoup de frames
    for (int i = 0; i < 100; i++) {
        system.update(1.0f / 60.0f);
    }
    
    auto transform = registry.get_component<TransformComponent>(entity).value();
    
    // Devrait être bloqué à 100.0f
    ASSERT_EQ(transform->position.x, 100.0f);
    
    // Vitesse devrait être 0
    ASSERT_EQ(velocity->velocity.x, 0.0f);
}
```

### Test 3 : Sans vitesse

```cpp
TEST(PhysicsSystem, HandlesZeroVelocity) {
    TempRegistry registry;
    core::EventBus event_bus;
    PhysicsSystem system(registry, event_bus);
    
    EntityId entity = registry.create_entity();
    registry.add_component(entity, TransformComponent{50.0f, 50.0f});
    registry.add_component(entity, VelocityComponent{});  // velocity = (0, 0)
    
    system.update(1.0f / 60.0f);
    
    auto transform = registry.get_component<TransformComponent>(entity).value();
    
    // Position ne devrait pas changer
    ASSERT_EQ(transform->position.x, 50.0f);
    ASSERT_EQ(transform->position.y, 50.0f);
}
```

---

## 💡 Cas d'usage avancés

### 1. Projectiles

```cpp
// Créer un projectile
EntityId bullet = registry.create_entity();
registry.add_component(bullet, TransformComponent{player_x, player_y});
registry.add_component(bullet, VelocityComponent{});

// Donner une vitesse rapide
auto velocity = registry.get_component<VelocityComponent>(bullet).value();
velocity->velocity.x = 500.0f;  // Très rapide
velocity->velocity.y = 0.0f;

// PhysicsSystem le déplace automatiquement
physics_system.update(dt);

// Détruire quand hors limites
auto transform = registry.get_component<TransformComponent>(bullet).value();
if (transform->position.x > world_max_x_) {
    registry.destroy_entity(bullet);
}
```

### 2. Gravité

```cpp
// Dans une boucle de jeu
for (EntityId entity : entities_with_gravity) {
    auto velocity = registry.get_component<VelocityComponent>(entity).value();
    
    // Appliquer la gravité (9.8 m/s² = 980 pixels/s²)
    velocity->velocity.y += 980.0f * delta_time;
}

// PhysicsSystem applique ensuite la vitesse
physics_system.update(delta_time);
```

### 3. Rebond sur les murs

Modifier PhysicsSystem pour inverser la vitesse au lieu de l'arrêter :

```cpp
// Dans PhysicsSystem::update()
if (transform->position.x < world_min_x_) {
    transform->position.x = world_min_x_;
    velocity->velocity.x = -velocity->velocity.x * 0.8f;  // Rebond avec perte
}
```

---

## 🔧 Améliorations possibles

### 1. Collision intelligente (ne bloquer qu'une direction)

```cpp
// Au lieu de tout arrêter
velocity->velocity.x = 0.0f;
velocity->velocity.y = 0.0f;

// Bloquer seulement la direction qui touche
if (transform->position.x > world_max_x_) {
    transform->position.x = world_max_x_;
    velocity->velocity.x = std::min(0.0f, velocity->velocity.x);  // Garde vitesse négative
}
```

### 2. Événements de collision

```cpp
// Dans PhysicsSystem::update()
if (transform->position.x >= world_max_x_) {
    transform->position.x = world_max_x_;
    velocity->velocity.x = 0.0f;
    
    // Publier un événement
    event_bus_.publish(WallCollisionEvent{
        entity,
        CollisionSide::Right,
        {transform->position.x, transform->position.y}
    });
}
```

### 3. Hitbox (AABB)

```cpp
// Ajouter un composant
struct ColliderComponent {
    Vector2f offset{0.0f, 0.0f};
    Vector2f size{32.0f, 32.0f};
};

// Dans PhysicsSystem::update()
auto collider = registry_.get_component<ColliderComponent>(entity);
if (collider.has_value()) {
    float left = transform->position.x + collider->offset.x;
    float right = left + collider->size.x;
    
    if (right > world_max_x_) {
        // Collision avec hitbox
    }
}
```

### 4. Friction

```cpp
// Dans PhysicsSystem::update()
const float FRICTION = 0.98f;  // 98% de la vitesse conservée

velocity->velocity.x *= FRICTION;
velocity->velocity.y *= FRICTION;

// Arrêter si très lent
if (std::abs(velocity->velocity.x) < 0.1f) velocity->velocity.x = 0.0f;
if (std::abs(velocity->velocity.y) < 0.1f) velocity->velocity.y = 0.0f;
```

---

## ❓ Questions fréquentes

### Q1 : Pourquoi delta_time est obligatoire ?

**R** : Pour que le mouvement soit **indépendant du framerate** :

```cpp
// À 60 FPS
position += velocity * (1/60) = velocity * 0.01667

// À 30 FPS
position += velocity * (1/30) = velocity * 0.03333

// Résultat : Même distance parcourue par seconde !
```

### Q2 : Que se passe-t-il si je ne set pas les world_bounds ?

**R** : Les valeurs par défaut sont utilisées (1920x1080). L'entité peut sortir si ton monde est plus grand.

### Q3 : Peut-on avoir un monde sans limites ?

**R** : Oui, utilise des valeurs très grandes :

```cpp
physics_system.set_world_bounds(-1000000.0f, -1000000.0f, 
                                 1000000.0f, 1000000.0f);
```

### Q4 : Comment faire un portail (wraparound) ?

**R** : Modifier PhysicsSystem :

```cpp
// Téléporter au lieu de bloquer
if (transform->position.x > world_max_x_) {
    transform->position.x = world_min_x_;  // Téléporter
    // Ne pas toucher velocity
}
```

### Q5 : Peut-on avoir plusieurs entités avec des limites différentes ?

**R** : Pas avec le système actuel. Tous partagent les mêmes world_bounds. Solution : Ajouter un composant `BoundsComponent` par entité.

---

## 📝 Résumé

### Points clés

1. **Rôle** : Applique velocity à position + gère les limites
2. **Formule** : `position += velocity * delta_time`
3. **Ordre** : Doit être appelé APRÈS MovementSystem
4. **Delta time** : OBLIGATOIRE pour framerate-independence
5. **Limites** : Configurable via `set_world_bounds()`

### Ordre d'exécution

```
InputSystem → MovementSystem → PhysicsSystem → RenderSystem
   (Lit)        (Calcule)        (Applique)      (Affiche)
```

### Commandes de base

```cpp
// 1. Créer
PhysicsSystem physics_system(registry, event_bus);

// 2. Configurer limites
physics_system.set_world_bounds(0.0f, 0.0f, 800.0f, 600.0f);

// 3. Ajouter composants à une entité
registry.add_component(entity, TransformComponent{x, y});
registry.add_component(entity, VelocityComponent{});

// 4. Mettre à jour chaque frame
physics_system.update(delta_time);
```

---

**Fichier** : `engine/systems/PhysicsSystem.hpp`  
**Documentation** : `engine/systems/GUIDE_PHYSICS_SYSTEM.md`  
**Auteur** : Documentation technique  
**Date** : 25 novembre 2025
