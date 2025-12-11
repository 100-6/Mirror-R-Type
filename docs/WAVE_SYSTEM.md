# Wave System Documentation

## Overview

Le système de vagues (Wave System) permet de spawn automatiquement des entités (ennemis, murs, obstacles, bonus/powerups) basé sur la progression du scrolling et configuré via des fichiers JSON.

## Architecture

### Components

Les composants suivants ont été ajoutés dans [`GameComponents.hpp`](src/r-type/game-logic/include/components/GameComponents.hpp):

- `WaveSpawnData`: Données pour spawner une entité
- `WaveTrigger`: Conditions de déclenchement d'une vague
- `WaveController`: Contrôleur global du système de vagues
- `SpawnPattern`: Enum pour les patterns de spawn
- `EntitySpawnType`: Enum pour les types d'entités

### Configuration

Les constantes de configuration sont définies dans [`WaveConfig.hpp`](src/r-type/game-logic/include/components/WaveConfig.hpp):

```cpp
#define WAVE_DEFAULT_SPAWN_INTERVAL     2.0f    // Intervalle par défaut entre spawns
#define WAVE_DEFAULT_BETWEEN_WAVES      5.0f    // Temps entre vagues
#define WAVE_SPAWN_OFFSET_X             50.0f   // Offset de spawn à droite
#define WAVE_SPAWN_MIN_Y                50.0f   // Position Y minimale
#define WAVE_SPAWN_MAX_Y                1030.0f // Position Y maximale
// ... et plus
```

### Systems

- **WaveSpawnerSystem**: Système principal qui gère le spawn des vagues
- **WaveConfigLoader**: Utilitaire pour charger et valider les configurations JSON

## JSON Configuration Format

### Structure de base

```json
{
  "defaultSpawnInterval": 2.0,
  "loopWaves": false,
  "waves": [...]
}
```

### Format d'une vague

```json
{
  "trigger": {
    "scrollDistance": 500,    // Distance de scroll pour trigger (en pixels)
    "timeDelay": 2.0          // Délai après trigger (en secondes)
  },
  "spawns": [...]
}
```

### Format d'un spawn

```json
{
  "type": "enemy",           // Type: "enemy", "wall", "obstacle", "powerup"
  "enemyType": "basic",      // Pour enemies: "basic", "fast", "tank", "boss"
  "bonusType": "health",     // Pour powerups: "health", "shield", "speed"
  "positionX": 1920,         // Position X absolue
  "positionY": 300,          // Position Y absolue
  "count": 3,                // Nombre d'entités à spawner
  "pattern": "line",         // Pattern: "single", "line", "grid", "random", "formation"
  "spacing": 150             // Espacement entre entités (pour patterns)
}
```

## Spawn Patterns

### SINGLE
Spawn une seule entité à la position spécifiée.

```json
{
  "pattern": "single",
  "count": 1
}
```

### LINE
Spawn des entités en ligne verticale.

```json
{
  "pattern": "line",
  "count": 5,
  "spacing": 100  // Espacement vertical
}
```

### GRID
Spawn des entités en grille.

```json
{
  "pattern": "grid",
  "count": 9,
  "spacing": 80  // Espacement vertical
}
```

### RANDOM
Spawn des entités à des positions Y aléatoires.

```json
{
  "pattern": "random",
  "count": 4
}
```

### FORMATION
Spawn des entités en formation V.

```json
{
  "pattern": "formation",
  "count": 6,
  "spacing": 80
}
```

## Usage Example

### 1. Intégration dans le code

```cpp
#include "systems/WaveSpawnerSystem.hpp"

// Dans votre fonction d'initialisation
auto waveSystem = std::make_unique<WaveSpawnerSystem>(graphics);
registry.register_system(std::move(waveSystem));

// Charger la configuration
auto* waveSpawner = registry.get_system<WaveSpawnerSystem>();
if (waveSpawner) {
    waveSpawner->loadWaveConfiguration("assets/waves_example.json");
}
```

### 2. Exemple de configuration simple

Voir [`waves_simple.json`](src/r-type/assets/waves_simple.json):

```json
{
  "defaultSpawnInterval": 2.0,
  "loopWaves": false,
  "waves": [
    {
      "trigger": {
        "scrollDistance": 0,
        "timeDelay": 0
      },
      "spawns": [
        {
          "type": "enemy",
          "enemyType": "basic",
          "positionX": 1920,
          "positionY": 300,
          "count": 1,
          "pattern": "single",
          "spacing": 0
        }
      ]
    }
  ]
}
```

### 3. Exemple de configuration avancée

Voir [`waves_example.json`](src/r-type/assets/waves_example.json) pour un exemple complet avec:
- 7 vagues différentes
- Multiples patterns de spawn
- Combinaisons d'ennemis et de murs
- Boss final

## Configuration des Entités

### Enemies

Les statistiques des ennemis sont définies dans [`CombatConfig.hpp`](src/r-type/game-logic/include/components/CombatConfig.hpp):

- **BASIC**: Santé 30, Vitesse 100, Cooldown 2.0s
- **FAST**: Santé 20, Vitesse 200, Cooldown 1.5s
- **TANK**: Santé 100, Vitesse 50, Cooldown 3.0s
- **BOSS**: Santé 500, Vitesse 80, Cooldown 0.8s

### Walls & Obstacles

Configurés dans [`WaveConfig.hpp`](src/r-type/game-logic/include/components/WaveConfig.hpp):

- **Murs**: Santé 100, Taille 50x100
- **Obstacles**: Santé 50, Taille 50x100

### Powerups (Bonus)

Trois types de bonus sont disponibles:

- **health** (Vert): +20 HP au joueur
- **shield** (Violet): Protection d'1 hit (cercle violet autour du joueur)
- **speed** (Bleu): +50% de vitesse pendant 20 secondes

Exemple de spawn de bonus:

```json
{
  "type": "powerup",
  "bonusType": "health",
  "positionX": 1920,
  "positionY": 500,
  "count": 1,
  "pattern": "single"
}
```

## Tips & Best Practices

### 1. Planification des vagues

- Commencer avec des vagues simples (SINGLE, LINE)
- Augmenter progressivement la difficulté
- Utiliser `scrollDistance` pour espacer les vagues
- Utiliser `timeDelay` pour créer du suspense avant les boss

### 2. Patterns de spawn

- **LINE**: Parfait pour des formations défensives
- **GRID**: Bon pour créer des murs d'ennemis
- **RANDOM**: Ajoute de l'imprévisibilité
- **FORMATION**: Idéal pour des escadrons organisés

### 3. Combinaisons d'entités

Combiner différents types dans une même vague:

```json
{
  "spawns": [
    {
      "type": "wall",
      "positionX": 1920,
      "positionY": 200,
      "count": 2,
      "pattern": "line",
      "spacing": 400
    },
    {
      "type": "enemy",
      "enemyType": "tank",
      "positionX": 2000,
      "positionY": 400,
      "count": 1,
      "pattern": "single"
    }
  ]
}
```

### 4. Testing

Utiliser [`waves_simple.json`](src/r-type/assets/waves_simple.json) pour tester:
- Vérifier que les spawns fonctionnent
- Ajuster les positions
- Tester les patterns

## Limites du système

Définies dans [`WaveConfig.hpp`](src/r-type/game-logic/include/components/WaveConfig.hpp):

- `WAVE_MAX_ACTIVE_WAVES`: 10 vagues maximum
- `WAVE_MAX_ENTITIES_PER_WAVE`: 50 entités par vague maximum
- `WAVE_MIN_SPAWN_INTERVAL`: 0.5s minimum entre spawns
- `WAVE_MAX_SPAWN_INTERVAL`: 10.0s maximum entre spawns
- `WAVE_SPAWN_MIN_Y`: 50px position Y minimale
- `WAVE_SPAWN_MAX_Y`: 1030px position Y maximale

## Troubleshooting

### Les vagues ne se déclenchent pas

- Vérifier que le système de scrolling est actif
- Vérifier que `scrollDistance` est correctement configuré
- Regarder les logs console pour les messages de debug

### Les entités spawn au mauvais endroit

- Vérifier `positionX` et `positionY`
- Vérifier que les positions Y sont entre `WAVE_SPAWN_MIN_Y` et `WAVE_SPAWN_MAX_Y`
- Ajuster `spacing` pour les patterns

### Le JSON ne se charge pas

- Vérifier la syntaxe JSON (virgules, guillemets, accolades)
- Vérifier le chemin du fichier
- Regarder les messages d'erreur dans la console

## Files Structure

```
src/r-type/game-logic/
├── include/
│   ├── components/
│   │   ├── GameComponents.hpp     # Wave components definitions
│   │   └── WaveConfig.hpp         # Wave configuration defines
│   └── systems/
│       ├── WaveSpawnerSystem.hpp  # Wave spawner system header
│       └── WaveConfigLoader.hpp   # JSON loader header
└── src/
    └── systems/
        ├── WaveSpawnerSystem.cpp  # Wave spawner implementation
        └── WaveConfigLoader.cpp   # JSON loader implementation

src/r-type/assets/
├── waves_example.json             # Complex example
└── waves_simple.json              # Simple example for testing
```

## Future Improvements

- [x] Support pour les powerups (health, shield, speed)
- [ ] Patterns de spawn plus complexes (cercle, spirale)
- [ ] Événements scriptables (changement de musique, effets visuels)
- [ ] Conditions de trigger multiples (temps ET distance)
- [ ] Support pour les animations de spawn
- [ ] Éditeur visuel de vagues
