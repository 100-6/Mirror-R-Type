# Plan: Remplacement de l'arme chargée par un vrai laser

## Objectif
1. Renommer l'actuel "LASER" (Level 4) en "MACHINE_GUN" (mitraillette) car il tire des projectiles rapides
2. Remplacer "CHARGE" (Level 5) par un vrai LASER beam utilisant `draw_line()` du plugin graphique

## Fichiers critiques à modifier

| Fichier | Modifications |
|---------|---------------|
| `src/r-type/game-logic/include/components/GameComponents.hpp` | Renommer enum `WeaponType`, ajouter composant `LaserBeam` |
| `src/r-type/game-logic/include/components/CombatConfig.hpp` | Renommer `WEAPON_LASER_*` -> `WEAPON_MACHINE_GUN_*`, nouveaux defines pour le laser beam |
| `src/r-type/game-logic/include/components/CombatHelpers.hpp` | Mettre à jour `get_weapon_stats()` et `create_weapon()` |
| `src/r-type/game-logic/include/components/PlayerLevelComponent.hpp` | Mettre à jour `get_weapon_type_for_level()` |
| `src/r-type/game-logic/src/systems/ShootingSystem.cpp` | Logique de tir du laser beam + raycast |
| `src/engine/src/ecs/systems/RenderSystem.cpp` | Rendu du laser avec `draw_line()` |

---

## Phase 1: Renommage LASER -> MACHINE_GUN

### 1.1 GameComponents.hpp
```cpp
enum class WeaponType {
    BASIC,
    SPREAD,
    BURST,
    MACHINE_GUN,  // Anciennement LASER (tir rapide)
    LASER         // Anciennement CHARGE (vrai rayon laser)
};
```

### 1.2 CombatConfig.hpp
- Renommer tous les `WEAPON_LASER_*` en `WEAPON_MACHINE_GUN_*`
- Remplacer `WEAPON_CHARGE_*` par de nouveaux `WEAPON_LASER_*` pour le beam

### 1.3 CombatHelpers.hpp
- Mettre à jour toutes les références `WeaponType::LASER` -> `WeaponType::MACHINE_GUN`
- Mettre à jour toutes les références `WeaponType::CHARGE` -> `WeaponType::LASER`

### 1.4 PlayerLevelComponent.hpp
```cpp
case 4: return WeaponType::MACHINE_GUN;  // Mitraillette
case 5: return WeaponType::LASER;        // Vrai laser beam
```

---

## Phase 2: Nouveau composant LaserBeam

### 2.1 Ajouter dans GameComponents.hpp
```cpp
struct LaserBeam {
    bool active = false;                // Rayon actif?
    float range = 1000.0f;              // Portée max
    float current_length = 0.0f;        // Longueur actuelle (collision)
    float damage_per_tick = 3.0f;       // Dégâts par tick
    float tick_rate = 0.05f;            // 20 ticks/sec
    float time_since_last_tick = 0.0f;
    float width = 8.0f;                 // Épaisseur visuelle
    engine::Color beam_color{255, 50, 50, 255};   // Rouge
    engine::Color core_color{255, 255, 200, 255}; // Blanc/jaune

    // Position de fin du rayon
    float hit_x = 0.0f;
    float hit_y = 0.0f;
    std::vector<Entity> entities_hit_this_tick;
};
```

### 2.2 Configuration dans CombatConfig.hpp
```cpp
// LASER - Vrai rayon continu
#define WEAPON_LASER_DAMAGE_PER_TICK    3       // 60 DPS
#define WEAPON_LASER_TICK_RATE          0.05f
#define WEAPON_LASER_RANGE              1000.0f
#define WEAPON_LASER_WIDTH              8.0f
#define WEAPON_LASER_COLOR_R            255
#define WEAPON_LASER_COLOR_G            50
#define WEAPON_LASER_COLOR_B            50
#define WEAPON_LASER_COLOR_A            255
```

---

## Phase 3: Logique de tir du laser (ShootingSystem.cpp)

### 3.1 Activation du laser
- `PlayerStartFireEvent`: Si arme = LASER, activer `LaserBeam::active = true`
- `PlayerStopFireEvent`: Si arme = LASER, désactiver `LaserBeam::active = false`

### 3.2 Update du laser beam
Nouvelle méthode `updateLaserBeam()`:
1. Calculer position de départ (devant le vaisseau)
2. Raycast horizontal pour trouver les ennemis/murs touchés
3. Appliquer dégâts à intervalles réguliers (tick_rate)
4. Mettre à jour `hit_x`, `hit_y` pour le rendu

### 3.3 Raycast pour collision
- Parcourir tous les ennemis avec Collider
- Vérifier intersection ligne horizontale vs AABB
- Stocker le point d'impact le plus proche

---

## Phase 4: Rendu du laser (RenderSystem.cpp)

Après le rendu des sprites, ajouter:
```cpp
// Rendu des lasers
auto& lasers = registry.get_components<LaserBeam>();
for (Entity entity : lasers) {
    LaserBeam& laser = lasers[entity];
    if (!laser.active) continue;

    // Glow externe (additive blending)
    graphics_plugin.begin_blend_mode(1);
    graphics_plugin.draw_line(start, end, glow_color, width * 2);
    graphics_plugin.end_blend_mode();

    // Rayon principal
    graphics_plugin.draw_line(start, end, beam_color, width);

    // Core brillant
    graphics_plugin.draw_line(start, end, core_color, width * 0.3f);

    // Impact (cercle si collision)
    if (hitting_enemy) {
        graphics_plugin.draw_circle(hit_point, radius, core_color);
    }
}
```

---

## Phase 5: Mettre à jour toutes les références

Fichiers à rechercher et remplacer:
- `WeaponType::LASER` -> `WeaponType::MACHINE_GUN`
- `WeaponType::CHARGE` -> `WeaponType::LASER`
- `WEAPON_LASER_*` -> `WEAPON_MACHINE_GUN_*`
- `WEAPON_CHARGE_*` -> supprimer ou renommer

---

## Stats du laser beam

| Propriété | Valeur | Notes |
|-----------|--------|-------|
| DPS | 60 | 3 dmg x 20 ticks/sec |
| Portée | 1000px | Plein écran |
| Épaisseur | 8px | Visible mais pas énorme |
| Couleur | **Rouge** + core blanc | Choix utilisateur |
| Pénétration | **Non** | S'arrête au premier ennemi/mur touché |

---

## Vérification

1. Compiler et lancer le jeu
2. Atteindre Level 4: vérifier que la mitraillette tire toujours des projectiles rapides
3. Atteindre Level 5: vérifier que le laser beam s'affiche en maintenant le tir
4. Tester collision: le laser doit s'arrêter sur les ennemis/murs
5. Tester dégâts: les ennemis doivent subir des dégâts continus
