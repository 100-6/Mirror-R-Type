# 🎨 RenderSystem - Documentation Complète

## 📋 Vue d'ensemble

Le **RenderSystem** est responsable de **dessiner toutes les entités visibles** à l'écran en utilisant un plugin graphique (IGraphicsPlugin).

### Rôle dans le pipeline

```
TransformComponent + SpriteComponent → RenderSystem → IGraphicsPlugin → Écran
     (Position)         (Visuel)         (Dessine)       (Affiche)
```

**En une phrase** : "Il dessine tous les sprites aux bonnes positions en respectant le Z-order."

---

## 🏗️ Déclaration

### Fichier
`engine/systems/RenderSystem.hpp`

### Signature de la classe

```cpp
class RenderSystem {
public:
    RenderSystem(TempRegistry& registry,
                 core::EventBus& event_bus,
                 IGraphicsPlugin* graphics_plugin);
    
    void update(float delta_time = 0.0f);

private:
    TempRegistry& registry_;
    core::EventBus& event_bus_;
    IGraphicsPlugin* graphics_plugin_;
};
```

---

## 🔧 Utilisation

### 1. Création du système

```cpp
#include "RenderSystem.hpp"
#include "temp/TempRegistry.hpp"
#include "../include/core/event/EventBus.hpp"
#include "../../plugin_manager/include/IGraphicsPlugin.hpp"

// Setup
TempRegistry registry;
core::EventBus event_bus;
IGraphicsPlugin* graphics_plugin = /* votre plugin */;

// Créer le système
RenderSystem render_system(registry, event_bus, graphics_plugin);
```

### 2. Créer une entité visible

```cpp
// Créer une entité
EntityId player = registry.create_entity();

// Ajouter TransformComponent (position obligatoire)
registry.add_component(player, TransformComponent{100.0f, 100.0f});

// Ajouter SpriteComponent (visuel obligatoire)
TextureHandle player_texture = graphics_plugin->load_texture("assets/player.png");
registry.add_component(player, SpriteComponent{player_texture, {32.0f, 32.0f}});

// Le système peut maintenant dessiner cette entité
```

### 3. Mettre à jour le système chaque frame

```cpp
float delta_time = 1.0f / 60.0f;  // 60 FPS

// Dans la boucle de jeu
while (running) {
    input_system.update(delta_time);
    movement_system.update(delta_time);
    physics_system.update(delta_time);
    
    // Effacer l'écran
    graphics_plugin->clear(Color::Black);
    
    // Dessiner tout
    render_system.update(delta_time);
    
    // Afficher
    graphics_plugin->display();
}
```

**⚠️ IMPORTANT** : RenderSystem doit être le **DERNIER** système appelé !

---

## 🔄 Fonctionnement détaillé

### Méthode `update(float delta_time)`

```cpp
void update(float delta_time = 0.0f) {
    (void)delta_time;  // Non utilisé
    
    if (!graphics_plugin_) {
        return;  // Pas de plugin, on sort
    }
    
    // 1️⃣ Récupérer toutes les entités avec Transform ET Sprite
    auto entities = registry_.get_entities_with<TransformComponent, SpriteComponent>();
    
    // 2️⃣ Trier par Z-order (profondeur)
    std::vector<EntityId> sorted_entities(entities.begin(), entities.end());
    std::sort(sorted_entities.begin(), sorted_entities.end(),
        [this](EntityId a, EntityId b) {
            auto sprite_a = registry_.get_component<SpriteComponent>(a).value();
            auto sprite_b = registry_.get_component<SpriteComponent>(b).value();
            return sprite_a->z_order < sprite_b->z_order;
        });
    
    // 3️⃣ Dessiner chaque entité dans l'ordre
    for (EntityId entity : sorted_entities) {
        auto transform_opt = registry_.get_component<TransformComponent>(entity);
        auto sprite_opt = registry_.get_component<SpriteComponent>(entity);
        
        if (!transform_opt.has_value() || !sprite_opt.has_value()) {
            continue;
        }
        
        auto* transform = transform_opt.value();
        auto* sprite = sprite_opt.value();
        
        // 4️⃣ Préparer le sprite
        Sprite render_sprite;
        render_sprite.texture = sprite->texture;
        render_sprite.position = transform->position;
        render_sprite.size = sprite->size;
        render_sprite.rotation = transform->rotation;
        render_sprite.scale = transform->scale;
        render_sprite.tint = sprite->tint;
        
        // 5️⃣ Dessiner
        graphics_plugin_->draw_sprite(render_sprite, transform->position);
    }
}
```

### Étapes d'exécution

1. **Vérifier** qu'un plugin graphique est présent
2. **Récupérer** toutes les entités avec TransformComponent ET SpriteComponent
3. **Trier** par z_order (les plus petits en premier = arrière-plan)
4. **Pour chaque entité** (dans l'ordre) :
   - Obtenir Transform et Sprite
   - Préparer les données de rendu
   - Appeler `graphics_plugin->draw_sprite()`

---

## 🎯 Exemple complet

### Code minimal

```cpp
#include "RenderSystem.hpp"
#include "MockPlugins.hpp"
#include <iostream>

int main() {
    // 1. Setup
    TempRegistry registry;
    core::EventBus event_bus;
    MockGraphicsPlugin graphics;
    graphics.initialize();
    graphics.create_window(800, 600, "Test");
    
    // 2. Créer le système
    RenderSystem render_system(registry, event_bus, &graphics);
    
    // 3. Créer une entité visible
    EntityId player = registry.create_entity();
    registry.add_component(player, TransformComponent{400.0f, 300.0f});
    
    TextureHandle texture = graphics.load_texture("player.png");
    registry.add_component(player, SpriteComponent{texture, {32.0f, 32.0f}});
    
    // 4. Boucle de rendu
    for (int frame = 0; frame < 60; frame++) {
        graphics.clear(Color::Black);
        render_system.update(1.0f / 60.0f);
        graphics.display();
    }
    
    graphics.close_window();
    return 0;
}
```

---

## 🎨 Composant SpriteComponent

### Structure

```cpp
struct SpriteComponent {
    TextureHandle texture = INVALID_HANDLE;
    Vector2f size{32.0f, 32.0f};
    Color tint = Color::White;
    int z_order = 0;
    
    SpriteComponent() = default;
    SpriteComponent(TextureHandle tex, Vector2f sz) 
        : texture(tex), size(sz) {}
};
```

### Champs

- **texture** : Handle de la texture (retourné par `load_texture()`)
- **size** : Taille du sprite en pixels
- **tint** : Couleur de teinte (blanc = pas de teinte)
- **z_order** : Profondeur (ordre de dessin)

### Z-Order (profondeur)

```cpp
// Arrière-plan (dessiné en premier)
SpriteComponent background;
background.z_order = -10;

// Joueur (dessiné au milieu)
SpriteComponent player;
player.z_order = 0;

// UI (dessiné en dernier, au-dessus)
SpriteComponent ui;
ui.z_order = 100;
```

**Ordre de dessin** : -10 → 0 → 100

---

## 🖼️ Gestion des textures

### Charger une texture

```cpp
// Via le plugin graphique
TextureHandle texture = graphics_plugin->load_texture("assets/player.png");

// Vérifier si chargé
if (texture == INVALID_HANDLE) {
    std::cerr << "Erreur : Impossible de charger la texture\n";
}
```

### Utiliser la texture

```cpp
// Créer un sprite avec cette texture
SpriteComponent sprite;
sprite.texture = texture;
sprite.size = {64.0f, 64.0f};

// Ajouter à une entité
registry.add_component(entity, sprite);
```

### Décharger une texture

```cpp
// Quand on n'en a plus besoin
graphics_plugin->unload_texture(texture);
```

---

## 🎭 Exemples d'utilisation

### Exemple 1 : Plusieurs sprites

```cpp
// Créer 3 entités avec des sprites différents

// Arrière-plan
EntityId bg = registry.create_entity();
registry.add_component(bg, TransformComponent{0.0f, 0.0f});
TextureHandle bg_tex = graphics->load_texture("background.png");
SpriteComponent bg_sprite{bg_tex, {800.0f, 600.0f}};
bg_sprite.z_order = -10;  // Derrière tout
registry.add_component(bg, bg_sprite);

// Joueur
EntityId player = registry.create_entity();
registry.add_component(player, TransformComponent{400.0f, 300.0f});
TextureHandle player_tex = graphics->load_texture("player.png");
SpriteComponent player_sprite{player_tex, {32.0f, 32.0f}};
player_sprite.z_order = 0;  // Au milieu
registry.add_component(player, player_sprite);

// UI
EntityId ui = registry.create_entity();
registry.add_component(ui, TransformComponent{10.0f, 10.0f});
TextureHandle ui_tex = graphics->load_texture("ui.png");
SpriteComponent ui_sprite{ui_tex, {200.0f, 50.0f}};
ui_sprite.z_order = 100;  // Devant tout
registry.add_component(ui, ui_sprite);

// Rendu (ordre automatique : bg → player → ui)
render_system.update(dt);
```

### Exemple 2 : Animation de couleur

```cpp
// Faire clignoter un sprite
float time = 0.0f;

while (running) {
    time += delta_time;
    
    // Calculer la teinte
    float intensity = (std::sin(time * 5.0f) + 1.0f) / 2.0f;  // 0-1
    
    auto sprite = registry.get_component<SpriteComponent>(player).value();
    sprite->tint = Color{255, uint8_t(255 * intensity), uint8_t(255 * intensity)};
    
    // Rendu
    graphics->clear(Color::Black);
    render_system.update(delta_time);
    graphics->display();
}
```

### Exemple 3 : Sprites sans texture (rectangles)

```cpp
// Dessiner un rectangle coloré au lieu d'une texture
EntityId box = registry.create_entity();
registry.add_component(box, TransformComponent{100.0f, 100.0f});

SpriteComponent box_sprite;
box_sprite.texture = INVALID_HANDLE;  // Pas de texture
box_sprite.size = {50.0f, 50.0f};
box_sprite.tint = Color::Red;
registry.add_component(box, box_sprite);

// Dans RenderSystem::update(), gérer le cas INVALID_HANDLE
if (sprite->texture == INVALID_HANDLE) {
    // Dessiner un rectangle coloré
    Rectangle rect{transform->position, sprite->size};
    graphics_plugin_->draw_rectangle(rect, sprite->tint);
} else {
    // Dessiner le sprite normalement
    graphics_plugin_->draw_sprite(render_sprite, transform->position);
}
```

---

## 🔗 Intégration avec IGraphicsPlugin

### Méthodes utilisées

```cpp
class IGraphicsPlugin {
public:
    // Dessin de sprites (utilisé par RenderSystem)
    virtual void draw_sprite(const Sprite& sprite, Vector2f position) = 0;
    
    // Autres méthodes disponibles
    virtual void draw_rectangle(const Rectangle& rect, Color color) = 0;
    virtual void draw_text(const std::string& text, Vector2f position, 
                           Color color, FontHandle font, int size) = 0;
    // ...
};
```

### Structure Sprite

```cpp
struct Sprite {
    TextureHandle texture;
    Vector2f position;
    Vector2f size;
    float rotation;
    Vector2f scale;
    Color tint;
};
```

---

## 🎬 Ordre de rendu (Z-Order)

### Concept

Le **z_order** détermine l'ordre dans lequel les sprites sont dessinés.

```
z_order = -10  ← Dessiné en PREMIER (arrière-plan)
z_order = 0    ← Dessiné au milieu
z_order = 100  ← Dessiné en DERNIER (premier plan)
```

### Exemple visuel

```
Écran final :
┌─────────────────────┐
│    UI (z=100)       │  ← Dessiné en dernier
│   ┌───────────┐     │
│   │ Player    │     │  ← Dessiné au milieu (z=0)
│   │ (z=0)     │     │
│   └───────────┘     │
│  Background (z=-10) │  ← Dessiné en premier
└─────────────────────┘
```

### Tri automatique

```cpp
// RenderSystem trie automatiquement
std::sort(entities.begin(), entities.end(), [](EntityId a, EntityId b) {
    auto sprite_a = get_sprite(a);
    auto sprite_b = get_sprite(b);
    return sprite_a->z_order < sprite_b->z_order;  // Croissant
});
```

---

## 🧪 Tests

### Test 1 : Rendu simple

```cpp
TEST(RenderSystem, RendersSprite) {
    TempRegistry registry;
    core::EventBus event_bus;
    MockGraphicsPlugin graphics;
    graphics.initialize();
    
    RenderSystem system(registry, event_bus, &graphics);
    
    EntityId entity = registry.create_entity();
    registry.add_component(entity, TransformComponent{100.0f, 100.0f});
    registry.add_component(entity, SpriteComponent{1, {32.0f, 32.0f}});
    
    // Devrait appeler draw_sprite une fois
    system.update(0.016f);
    
    // Vérifier avec un mock
    ASSERT_EQ(graphics.get_draw_call_count(), 1);
}
```

### Test 2 : Z-Order

```cpp
TEST(RenderSystem, RespectsZOrder) {
    TempRegistry registry;
    core::EventBus event_bus;
    MockGraphicsPlugin graphics;
    
    RenderSystem system(registry, event_bus, &graphics);
    
    // Créer 3 entités avec z-order différents
    EntityId e1 = registry.create_entity();
    SpriteComponent s1{1, {32.0f, 32.0f}};
    s1.z_order = 10;
    registry.add_component(e1, TransformComponent{0, 0});
    registry.add_component(e1, s1);
    
    EntityId e2 = registry.create_entity();
    SpriteComponent s2{2, {32.0f, 32.0f}};
    s2.z_order = -5;
    registry.add_component(e2, TransformComponent{0, 0});
    registry.add_component(e2, s2);
    
    EntityId e3 = registry.create_entity();
    SpriteComponent s3{3, {32.0f, 32.0f}};
    s3.z_order = 0;
    registry.add_component(e3, TransformComponent{0, 0});
    registry.add_component(e3, s3);
    
    system.update(0.016f);
    
    // Vérifier l'ordre de dessin : e2 (-5) → e3 (0) → e1 (10)
    auto draw_order = graphics.get_draw_order();
    ASSERT_EQ(draw_order[0], 2);  // Texture 2 (e2)
    ASSERT_EQ(draw_order[1], 3);  // Texture 3 (e3)
    ASSERT_EQ(draw_order[2], 1);  // Texture 1 (e1)
}
```

---

## 💡 Cas d'usage avancés

### 1. Caméra (offset de position)

```cpp
class RenderSystem {
private:
    Vector2f camera_offset_{0.0f, 0.0f};
    
public:
    void set_camera_offset(Vector2f offset) {
        camera_offset_ = offset;
    }
    
    void update(float dt) override {
        // ...
        for (EntityId entity : entities) {
            // Appliquer l'offset de caméra
            Vector2f world_pos = transform->position;
            Vector2f screen_pos = world_pos - camera_offset_;
            
            render_sprite.position = screen_pos;
            graphics_plugin_->draw_sprite(render_sprite, screen_pos);
        }
    }
};

// Utilisation
render_system.set_camera_offset({player_x - 400, player_y - 300});
```

### 2. Culling (ne dessiner que ce qui est visible)

```cpp
void update(float dt) override {
    Rectangle viewport{camera_offset_, {800.0f, 600.0f}};
    
    for (EntityId entity : entities) {
        auto transform = get_transform(entity);
        auto sprite = get_sprite(entity);
        
        // Vérifier si dans le viewport
        Rectangle entity_rect{transform->position, sprite->size};
        if (!intersects(viewport, entity_rect)) {
            continue;  // Hors écran, ne pas dessiner
        }
        
        // Dessiner
        graphics_plugin_->draw_sprite(/*...*/);
    }
}
```

### 3. Particules avec fade-out

```cpp
// Ajouter un composant de vie
struct ParticleComponent {
    float lifetime = 1.0f;
    float current_time = 0.0f;
};

// Mettre à jour l'alpha dans RenderSystem
void update(float dt) override {
    for (EntityId entity : entities) {
        auto particle = registry_.get_component<ParticleComponent>(entity);
        
        if (particle.has_value()) {
            // Calculer l'alpha
            float alpha = 1.0f - (particle->current_time / particle->lifetime);
            
            // Modifier la teinte
            sprite->tint.a = uint8_t(255 * alpha);
        }
        
        // Dessiner avec la nouvelle transparence
        graphics_plugin_->draw_sprite(/*...*/);
    }
}
```

---

## ❓ Questions fréquentes

### Q1 : Pourquoi delta_time n'est pas utilisé ?

**R** : RenderSystem **dessine juste l'état actuel**. Il ne modifie rien. Le temps n'a pas d'importance pour le dessin.

### Q2 : Faut-il appeler clear() et display() ?

**R** : **OUI** ! RenderSystem ne fait que dessiner. Tu dois :

```cpp
graphics->clear(Color::Black);     // Effacer
render_system.update(dt);          // Dessiner
graphics->display();                // Afficher
```

### Q3 : Que se passe-t-il si graphics_plugin_ est nullptr ?

**R** : Le système **sort immédiatement** sans crash :

```cpp
if (!graphics_plugin_) {
    return;  // Sécurité
}
```

### Q4 : Peut-on changer le z_order dynamiquement ?

**R** : **OUI** :

```cpp
auto sprite = registry.get_component<SpriteComponent>(entity).value();
sprite->z_order = 50;  // Passe devant

// Au prochain update(), sera dessiné dans le nouvel ordre
```

### Q5 : Comment dessiner du texte ?

**R** : RenderSystem ne gère que les sprites. Pour du texte, appelle directement le plugin :

```cpp
// Après render_system.update()
graphics->draw_text("Score: 100", {10, 10}, Color::White, font, 24);
```

---

## 📝 Résumé

### Points clés

1. **Rôle** : Dessine tous les sprites aux bonnes positions
2. **Z-Order** : Tri automatique pour l'ordre de dessin
3. **Ordre** : Doit être appelé EN DERNIER (après physics)
4. **Delta time** : Non utilisé (dessin d'état)
5. **Plugin** : Nécessite un IGraphicsPlugin fonctionnel

### Ordre d'exécution

```
InputSystem → MovementSystem → PhysicsSystem → RenderSystem
   (Lit)        (Calcule)        (Applique)      (Dessine)
```

### Commandes de base

```cpp
// 1. Créer
RenderSystem render_system(registry, event_bus, graphics_plugin);

// 2. Ajouter composants à une entité
registry.add_component(entity, TransformComponent{x, y});
TextureHandle tex = graphics->load_texture("sprite.png");
registry.add_component(entity, SpriteComponent{tex, {32, 32}});

// 3. Boucle de rendu
graphics->clear(Color::Black);
render_system.update(delta_time);
graphics->display();
```

---

**Fichier** : `engine/systems/RenderSystem.hpp`  
**Documentation** : `engine/systems/GUIDE_RENDER_SYSTEM.md`  
**Auteur** : Documentation technique  
**Date** : 25 novembre 2025
