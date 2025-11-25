# 🎮 InputSystem - Documentation Complète

## 📋 Vue d'ensemble

Le **InputSystem** est responsable de la **lecture des entrées utilisateur** (clavier, souris, gamepad) et de la mise à jour du composant `InputComponent`.

### Rôle dans le pipeline

```
IInputPlugin → InputSystem → InputComponent → MovementSystem
  (Clavier)      (Lit)        (Stocke)         (Utilise)
```

**En une phrase** : "Il lit le clavier via un plugin et met à jour les booléens dans InputComponent."

---

## 🏗️ Déclaration

### Fichier
`engine/systems/InputSystem.hpp`

### Signature de la classe

```cpp
class InputSystem {
public:
    InputSystem(TempRegistry& registry, 
                core::EventBus& event_bus,
                IInputPlugin* input_plugin);
    
    void update(float delta_time = 0.0f);

private:
    TempRegistry& registry_;
    core::EventBus& event_bus_;
    IInputPlugin* input_plugin_;
};
```

---

## 🔧 Utilisation

### 1. Création du système

```cpp
#include "InputSystem.hpp"
#include "temp/TempRegistry.hpp"
#include "../include/core/event/EventBus.hpp"
#include "../../plugin_manager/include/IInputPlugin.hpp"

// Setup
TempRegistry registry;
core::EventBus event_bus;
IInputPlugin* input_plugin = /* votre plugin */;

// Créer le système
InputSystem input_system(registry, event_bus, input_plugin);
```

### 2. Créer une entité avec InputComponent

```cpp
// Créer le joueur
EntityId player = registry.create_entity();

// Ajouter InputComponent
registry.add_component(player, InputComponent{});

// InputComponent initial :
// {
//     move_up = false,
//     move_down = false,
//     move_left = false,
//     move_right = false,
//     shoot = false
// }
```

### 3. Mettre à jour le système chaque frame

```cpp
float delta_time = 1.0f / 60.0f;  // 60 FPS

// Dans la boucle de jeu
while (running) {
    // Mettre à jour l'input AVANT les autres systèmes
    input_system.update(delta_time);
    
    // Puis les autres systèmes
    movement_system.update(delta_time);
    physics_system.update(delta_time);
}
```

**⚠️ IMPORTANT** : InputSystem doit être le **PREMIER** système appelé !

---

## 🔄 Fonctionnement détaillé

### Méthode `update(float delta_time)`

```cpp
void update(float delta_time = 0.0f) {
    (void)delta_time;  // Non utilisé
    
    if (!input_plugin_) {
        return;  // Pas de plugin, on sort
    }
    
    // 1️⃣ Récupérer toutes les entités avec InputComponent
    auto entities = registry_.get_entities_with<InputComponent>();
    
    // 2️⃣ Pour chaque entité
    for (EntityId entity : entities) {
        auto input_opt = registry_.get_component<InputComponent>(entity);
        
        if (!input_opt.has_value()) {
            continue;
        }
        
        auto* input = input_opt.value();
        
        // 3️⃣ Lire l'état du clavier via le plugin
        input->move_up    = input_plugin_->is_key_pressed(Key::W) ||
                            input_plugin_->is_key_pressed(Key::Up);
        input->move_down  = input_plugin_->is_key_pressed(Key::S) ||
                            input_plugin_->is_key_pressed(Key::Down);
        input->move_left  = input_plugin_->is_key_pressed(Key::A) ||
                            input_plugin_->is_key_pressed(Key::Left);
        input->move_right = input_plugin_->is_key_pressed(Key::D) ||
                            input_plugin_->is_key_pressed(Key::Right);
        input->shoot      = input_plugin_->is_key_pressed(Key::Space);
    }
}
```

### Étapes d'exécution

1. **Vérifier** qu'un plugin est présent
2. **Récupérer** toutes les entités avec `InputComponent`
3. **Pour chaque entité** :
   - Obtenir le composant InputComponent
   - Lire l'état des touches via `input_plugin_->is_key_pressed()`
   - Mettre à jour les booléens du composant

---

## 🎯 Exemple complet

### Code minimal

```cpp
#include "InputSystem.hpp"
#include "MockPlugins.hpp"  // Pour les tests

int main() {
    // 1. Setup de base
    TempRegistry registry;
    core::EventBus event_bus;
    MockInputPlugin input_plugin;
    input_plugin.initialize();
    
    // 2. Créer le système
    InputSystem input_system(registry, event_bus, &input_plugin);
    
    // 3. Créer une entité joueur
    EntityId player = registry.create_entity();
    registry.add_component(player, InputComponent{});
    
    // 4. Simuler un input (pour les tests)
    input_plugin.simulate_key_press(Key::D, true);  // Appuyer sur D
    
    // 5. Mettre à jour le système
    input_system.update(1.0f / 60.0f);
    
    // 6. Vérifier le résultat
    auto input = registry.get_component<InputComponent>(player).value();
    
    std::cout << "move_right: " << input->move_right << "\n";  // true
    std::cout << "move_left: " << input->move_left << "\n";    // false
    
    return 0;
}
```

### Résultat attendu

```
move_right: 1  (true)
move_left: 0   (false)
```

---

## 🔌 Interface IInputPlugin

### Méthodes utilisées par InputSystem

```cpp
class IInputPlugin {
public:
    // Vérifier si une touche est pressée
    virtual bool is_key_pressed(Key key) const = 0;
    
    // Autres méthodes disponibles mais non utilisées par InputSystem
    virtual bool is_key_just_pressed(Key key) const = 0;
    virtual bool is_key_just_released(Key key) const = 0;
    virtual Vector2f get_mouse_position() const = 0;
    // ...
};
```

### Touches supportées

```cpp
enum class Key {
    // Lettres WASD pour mouvement
    W, A, S, D,
    
    // Flèches directionnelles
    Up, Down, Left, Right,
    
    // Action
    Space,  // Tirer
    
    // Autres
    Escape, Enter, Q, // ...
};
```

---

## 🎮 Mappage des touches

### Configuration par défaut

| Touche | Action | Booléen dans InputComponent |
|--------|--------|---------------------------|
| W ou ↑ | Haut | `move_up` |
| S ou ↓ | Bas | `move_down` |
| A ou ← | Gauche | `move_left` |
| D ou → | Droite | `move_right` |
| Space | Tirer | `shoot` |

### Pourquoi deux touches par action ?

```cpp
input->move_up = input_plugin_->is_key_pressed(Key::W) ||
                 input_plugin_->is_key_pressed(Key::Up);
```

**Flexibilité** : Le joueur peut utiliser WASD OU les flèches directionnelles.

---

## 📊 Composant InputComponent

### Structure

```cpp
struct InputComponent {
    bool move_up = false;
    bool move_down = false;
    bool move_left = false;
    bool move_right = false;
    bool shoot = false;
    
    InputComponent() = default;
};
```

### États possibles

```cpp
// Aucune touche
InputComponent{ false, false, false, false, false }

// Mouvement droite
InputComponent{ false, false, false, true, false }

// Mouvement diagonal haut-droite
InputComponent{ true, false, false, true, false }

// Tir + mouvement
InputComponent{ true, false, false, false, true }
```

---

## 🔗 Intégration avec MovementSystem

### Flux de données

```cpp
// Frame N
InputSystem.update()
    → input->move_right = true
    
MovementSystem.update()
    → Lit input->move_right
    → Calcule velocity.x = +300
    
PhysicsSystem.update()
    → Applique velocity à position
```

### Ordre d'exécution CRITIQUE

```cpp
// ✅ BON ORDRE
input_system.update(dt);      // 1. Lit le clavier
movement_system.update(dt);   // 2. Utilise l'input
physics_system.update(dt);    // 3. Applique le mouvement

// ❌ MAUVAIS ORDRE
movement_system.update(dt);   // Utilise l'input de la frame précédente !
input_system.update(dt);      // Lit trop tard
physics_system.update(dt);
```

**Résultat du mauvais ordre** : Lag d'une frame (input en retard)

---

## 🧪 Tests

### Test 1 : Détection d'une touche

```cpp
TEST(InputSystem, DetectsSingleKey) {
    TempRegistry registry;
    core::EventBus event_bus;
    MockInputPlugin input;
    input.initialize();
    
    InputSystem system(registry, event_bus, &input);
    
    EntityId player = registry.create_entity();
    registry.add_component(player, InputComponent{});
    
    // Simuler D appuyé
    input.simulate_key_press(Key::D, true);
    system.update(0.016f);
    
    auto input_comp = registry.get_component<InputComponent>(player).value();
    
    ASSERT_TRUE(input_comp->move_right);
    ASSERT_FALSE(input_comp->move_left);
}
```

### Test 2 : Touches multiples

```cpp
TEST(InputSystem, DetectsMultipleKeys) {
    // ... setup ...
    
    // Simuler W + D (diagonal)
    input.simulate_key_press(Key::W, true);
    input.simulate_key_press(Key::D, true);
    system.update(0.016f);
    
    auto input_comp = registry.get_component<InputComponent>(player).value();
    
    ASSERT_TRUE(input_comp->move_up);
    ASSERT_TRUE(input_comp->move_right);
}
```

### Test 3 : Sans plugin

```cpp
TEST(InputSystem, HandlesNullPlugin) {
    TempRegistry registry;
    core::EventBus event_bus;
    
    // Plugin NULL
    InputSystem system(registry, event_bus, nullptr);
    
    EntityId player = registry.create_entity();
    registry.add_component(player, InputComponent{});
    
    // Ne devrait pas crash
    ASSERT_NO_THROW(system.update(0.016f));
}
```

---

## 💡 Cas d'usage avancés

### 1. Plusieurs joueurs

```cpp
// Joueur 1 (contrôlé par clavier)
EntityId player1 = registry.create_entity();
registry.add_component(player1, InputComponent{});
registry.add_component(player1, PlayerComponent{1});

// Joueur 2 (contrôlé par IA, pas d'InputComponent)
EntityId player2 = registry.create_entity();
registry.add_component(player2, PlayerComponent{2});
// Pas d'InputComponent !

// InputSystem ne traite que player1
input_system.update(dt);
```

### 2. Désactiver temporairement l'input

```cpp
// Retirer le composant pendant un menu
registry.remove_component<InputComponent>(player);

// L'InputSystem l'ignore maintenant
input_system.update(dt);

// Réactiver
registry.add_component(player, InputComponent{});
```

### 3. Input enregistré (replay)

```cpp
struct InputRecorder {
    struct Frame {
        int frame_number;
        InputComponent input;
    };
    
    std::vector<Frame> recorded_inputs;
    
    void record(int frame, const InputComponent& input) {
        recorded_inputs.push_back({frame, input});
    }
    
    InputComponent playback(int frame) {
        for (const auto& f : recorded_inputs) {
            if (f.frame_number == frame) {
                return f.input;
            }
        }
        return InputComponent{};  // Default
    }
};
```

---

## ❓ Questions fréquentes

### Q1 : Pourquoi delta_time n'est pas utilisé ?

**R** : InputSystem lit juste l'**état actuel** du clavier. Le temps n'a pas d'importance ici. C'est PhysicsSystem qui utilise delta_time pour appliquer le mouvement.

### Q2 : Peut-on changer le mapping des touches ?

**R** : Oui, modifie le code dans `update()` :

```cpp
// Mapping personnalisé
input->move_up = input_plugin_->is_key_pressed(Key::I);  // I au lieu de W
input->move_left = input_plugin_->is_key_pressed(Key::J);
input->move_down = input_plugin_->is_key_pressed(Key::K);
input->move_right = input_plugin_->is_key_pressed(Key::L);
```

### Q3 : Comment gérer la souris ?

**R** : Ajouter des champs dans InputComponent :

```cpp
struct InputComponent {
    bool move_up = false;
    // ...
    Vector2f mouse_position{0.0f, 0.0f};  // Nouveau
    bool mouse_left_click = false;        // Nouveau
};

// Dans InputSystem::update()
input->mouse_position = input_plugin_->get_mouse_position();
input->mouse_left_click = input_plugin_->is_mouse_button_pressed(MouseButton::Left);
```

### Q4 : Que faire si input_plugin_ est nullptr ?

**R** : Le système **sort immédiatement** sans crash :

```cpp
if (!input_plugin_) {
    return;  // Sécurité
}
```

### Q5 : Comment tester sans clavier réel ?

**R** : Utilise `MockInputPlugin` :

```cpp
MockInputPlugin mock;
mock.simulate_key_press(Key::D, true);  // Simule "D" appuyé

InputSystem system(registry, event_bus, &mock);
system.update(dt);
```

---

## 🔧 Modification et extension

### Ajouter une nouvelle action

```cpp
// 1. Ajouter dans InputComponent
struct InputComponent {
    // ... existant ...
    bool dash = false;  // Nouvelle action
};

// 2. Lire dans InputSystem::update()
input->dash = input_plugin_->is_key_pressed(Key::LShift);
```

### Supporter une manette

```cpp
// Dans InputSystem::update()
if (input_plugin_->is_gamepad_connected(0)) {
    // Stick gauche pour mouvement
    float stick_x = input_plugin_->get_gamepad_axis(0, 0);  // Axe X
    float stick_y = input_plugin_->get_gamepad_axis(0, 1);  // Axe Y
    
    input->move_right = (stick_x > 0.5f);
    input->move_left = (stick_x < -0.5f);
    input->move_down = (stick_y > 0.5f);
    input->move_up = (stick_y < -0.5f);
    
    // Bouton A pour tirer
    input->shoot = input_plugin_->is_gamepad_button_pressed(0, 0);
}
```

---

## 📝 Résumé

### Points clés

1. **Rôle** : Lit le clavier et met à jour InputComponent
2. **Dépendances** : TempRegistry, EventBus, IInputPlugin
3. **Ordre** : Doit être appelé EN PREMIER dans la boucle de jeu
4. **Delta time** : Non utilisé (lecture d'état instantané)
5. **Sécurité** : Gère le cas plugin = nullptr

### Ordre d'exécution

```
InputSystem → MovementSystem → PhysicsSystem → RenderSystem
   (Lit)        (Calcule)        (Applique)      (Affiche)
```

### Commande de base

```cpp
// 1. Créer
InputSystem input_system(registry, event_bus, input_plugin);

// 2. Ajouter composant à une entité
registry.add_component(player, InputComponent{});

// 3. Mettre à jour chaque frame
input_system.update(delta_time);
```

---

**Fichier** : `engine/systems/InputSystem.hpp`  
**Documentation** : `engine/systems/GUIDE_INPUT_SYSTEM.md`  
**Auteur** : Documentation technique  
**Date** : 25 novembre 2025
