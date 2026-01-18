# Wave Configuration Guide - Pattern 2-Chunks

Ce guide explique comment configurer les waves (vagues d'ennemis) pour les nouvelles maps du jeu R-Type.

## Table des Matières

1. [Vue d'Ensemble](#vue-densemble)
2. [Système de Chunks](#système-de-chunks)
3. [Pattern Standard: 2-Chunks](#pattern-standard-2-chunks)
4. [Structure JSON des Waves](#structure-json-des-waves)
5. [Composition des Ennemis par Difficulté](#composition-des-ennemis-par-difficulté)
6. [Configuration du Boss](#configuration-du-boss)
7. [Exemples Complets](#exemples-complets)
8. [Validation](#validation)

---

## Vue d'Ensemble

Le système de waves de R-Type est basé sur un déclenchement par **chunks** (morceaux de carte) et non plus par distance de scroll absolue.

### Concepts Clés

- **Chunk**: Unité de base de la carte (1 chunk = 480 pixels = 30 tuiles × 16px)
- **ChunkId**: Position du chunk sur la carte (0-indexed)
- **Offset**: Position relative dans le chunk (0.0 = début, 1.0 = fin)
- **Wave**: Vague d'ennemis déclenchée à une position de chunk précise
- **Phase**: Groupe de waves avec une difficulté cohérente (easy/medium/hard)

---

## Système de Chunks

### Dimensions

```
1 chunk = 480 pixels
1 tuile = 16 pixels
1 chunk = 30 tuiles
```

### Calcul de Position

Pour calculer la distance de scroll d'un chunk:

```
scroll_distance = chunkId × 480
```

**Exemples:**
- Chunk 0: 0 pixels
- Chunk 2: 960 pixels
- Chunk 10: 4800 pixels
- Chunk 20: 9600 pixels (fin de niveau standard)

### ChunkId + Offset

Le trigger d'une wave utilise `chunkId` (entier) + `offset` (décimal):

```json
{
  "chunkId": 4,
  "offset": 0.0
}
```

- `offset: 0.0` → Début du chunk (position exacte = 4 × 480 = 1920px)
- `offset: 0.5` → Milieu du chunk (position exacte = 1920 + 240 = 2160px)
- `offset: 1.0` → Fin du chunk (position exacte = 1920 + 480 = 2400px)

---

## Pattern Standard: 2-Chunks

### Règle Officielle

**Chaque level doit avoir des waves espacées de 2 chunks.**

### Configuration Standard (20 chunks total)

Pour un niveau avec 20 chunks:

| Wave | ChunkId | Scroll Distance | Position |
|------|---------|-----------------|----------|
| 1    | 0       | 0 px            | Début    |
| 2    | 2       | 960 px          |          |
| 3    | 4       | 1920 px         |          |
| 4    | 6       | 2880 px         |          |
| 5    | 8       | 3840 px         |          |
| 6    | 10      | 4800 px         |          |
| 7    | 12      | 5760 px         |          |
| 8    | 14      | 6720 px         |          |
| 9    | 16      | 7680 px         |          |
| 10   | 18      | 8640 px         | Dernière |
| Boss | N/A     | 9600 px         | Après chunk 20 |

### Avantages du Pattern 2-Chunks

✓ Rythme régulier et prévisible
✓ 10 waves par niveau = bon équilibre gameplay
✓ Temps de préparation constant entre waves
✓ Facile à tester et débugger

---

## Structure JSON des Waves

### Template de Wave

```json
{
  "wave_number": 1,
  "trigger": {
    "chunkId": 0,
    "offset": 0.0,
    "timeDelay": 0
  },
  "spawns": [
    {
      "type": "enemy",
      "enemyType": "basic",
      "positionX": 2100,
      "positionY": 300,
      "count": 3,
      "pattern": "line",
      "spacing": 120
    },
    {
      "type": "powerup",
      "bonusType": "health",
      "positionX": 2300,
      "positionY": 540,
      "count": 1,
      "pattern": "single"
    }
  ]
}
```

### Champs Obligatoires

#### wave_number (integer)
- Numéro séquentiel de la wave (1, 2, 3...)
- **IMPORTANT**: Doit être unique et séquentiel
- Utilisé pour le tracking côté serveur/client

#### trigger (object)
- **chunkId** (integer): Position du chunk (0-19 pour 20 chunks)
- **offset** (float): Position dans le chunk (0.0-1.0)
- **timeDelay** (integer): Délai supplémentaire en secondes (généralement 0)

#### spawns (array)
- Liste des entités à faire apparaître dans cette wave
- Peut contenir des ennemis et des powerups
- Au moins 1 spawn requis

### Types de Spawns

#### Enemy Spawn
```json
{
  "type": "enemy",
  "enemyType": "basic|zigzag|bouncer|tank|kamikaze",
  "positionX": 2100,
  "positionY": 300,
  "count": 3,
  "pattern": "line|grid|formation|random|single",
  "spacing": 120
}
```

**Enemy Types Disponibles:**
- `basic`: Ennemi standard, déplacement linéaire
- `zigzag`: Déplacement en zigzag vertical
- `bouncer`: Rebondit sur les bords de l'écran
- `tank`: Ennemi lent mais résistant
- `kamikaze`: Fonce sur le joueur

**Patterns de Spawn:**
- `single`: 1 seul ennemi (ignore count/spacing)
- `line`: Ligne verticale d'ennemis
- `grid`: Grille 2D (nécessite spacing horizontal/vertical)
- `formation`: Formation prédéfinie
- `random`: Positions aléatoires dans une zone

#### Powerup Spawn
```json
{
  "type": "powerup",
  "bonusType": "health|shield|speed|firepower",
  "positionX": 2300,
  "positionY": 540,
  "count": 1,
  "pattern": "single"
}
```

**Bonus Types:**
- `health`: Restaure la santé
- `shield`: Bouclier temporaire
- `speed`: Augmente la vitesse
- `firepower`: Améliore les tirs

---

## Composition des Ennemis par Difficulté

### Phase 1: Easy (Waves 1-3, Chunks 0-4)

**Objectif**: Introduction progressive, peu de menace

**Recommandations:**
- Majoritairement `basic` (60-80%)
- Quelques `zigzag` pour variété (20-30%)
- 1 powerup `health` dans wave 3

**Exemple Wave 1:**
```json
{
  "wave_number": 1,
  "trigger": {"chunkId": 0, "offset": 0.0, "timeDelay": 0},
  "spawns": [
    {
      "type": "enemy",
      "enemyType": "basic",
      "positionX": 2100,
      "positionY": 300,
      "count": 3,
      "pattern": "line",
      "spacing": 150
    }
  ]
}
```

### Phase 2: Medium (Waves 4-7, Chunks 6-12)

**Objectif**: Montée en difficulté, introduction ennemis avancés

**Recommandations:**
- Mix `zigzag`, `bouncer`, `tank`
- Premières apparitions de `kamikaze`
- Powerups stratégiques (`shield`, `speed`)
- Spawns multiples dans une wave (2-3 groupes)

**Exemple Wave 5:**
```json
{
  "wave_number": 5,
  "trigger": {"chunkId": 8, "offset": 0.0, "timeDelay": 0},
  "spawns": [
    {
      "type": "enemy",
      "enemyType": "tank",
      "positionX": 2100,
      "positionY": 250,
      "count": 2,
      "pattern": "line",
      "spacing": 280
    },
    {
      "type": "enemy",
      "enemyType": "bouncer",
      "positionX": 2200,
      "positionY": 600,
      "count": 3,
      "pattern": "line",
      "spacing": 130
    },
    {
      "type": "powerup",
      "bonusType": "shield",
      "positionX": 2300,
      "positionY": 450,
      "count": 1,
      "pattern": "single"
    }
  ]
}
```

### Phase 3: Hard (Waves 8-10, Chunks 14-18)

**Objectif**: Difficulté maximale, préparation au boss

**Recommandations:**
- Beaucoup de `tank`, `kamikaze`, `bouncer`
- Spawns multiples et simultanés (3-4 groupes)
- Powerups rares mais puissants
- Dernier powerup `health` dans wave 10 (avant boss)

**Exemple Wave 10:**
```json
{
  "wave_number": 10,
  "trigger": {"chunkId": 18, "offset": 0.0, "timeDelay": 0},
  "spawns": [
    {
      "type": "enemy",
      "enemyType": "tank",
      "positionX": 2100,
      "positionY": 300,
      "count": 3,
      "pattern": "line",
      "spacing": 180
    },
    {
      "type": "enemy",
      "enemyType": "bouncer",
      "positionX": 2200,
      "positionY": 150,
      "count": 4,
      "pattern": "line",
      "spacing": 140
    },
    {
      "type": "enemy",
      "enemyType": "kamikaze",
      "positionX": 2300,
      "positionY": 750,
      "count": 3,
      "pattern": "line",
      "spacing": 120
    },
    {
      "type": "powerup",
      "bonusType": "health",
      "positionX": 2400,
      "positionY": 540,
      "count": 1,
      "pattern": "single"
    }
  ]
}
```

---

## Configuration du Boss

### Spawn Distance Standard

**Le boss doit toujours spawn après le chunk 20:**

```json
"boss": {
  "boss_name": "Nom du Boss",
  "spawn_scroll_distance": 9600.0,
  "spawn_position_x": 1600.0,
  "spawn_position_y": 540.0,
  "enemy_type": "boss",
  "total_phases": 3,
  "script_path": "boss/boss_script.lua",
  "phases": [...]
}
```

### Calcul de spawn_scroll_distance

```
spawn_scroll_distance = (total_chunks) × 480
                      = 20 × 480
                      = 9600.0 pixels
```

### Position de Spawn Recommandée

- **spawn_position_x**: 1600.0 (centre-droit de l'écran)
- **spawn_position_y**: 540.0 (centre vertical, écran 1920×1080)

### Phases du Boss

Chaque boss a généralement **3 phases** basées sur le seuil de santé:

```json
"phases": [
  {
    "phase_number": 1,
    "health_threshold": 1.0,     // 100% → 66% HP
    "movement_pattern": "vertical_sine",
    "movement_speed_multiplier": 1.0,
    "attack_patterns": [...]
  },
  {
    "phase_number": 2,
    "health_threshold": 0.66,    // 66% → 33% HP
    "movement_pattern": "figure_eight",
    "movement_speed_multiplier": 1.3,
    "attack_patterns": [...]
  },
  {
    "phase_number": 3,
    "health_threshold": 0.33,    // 33% → 0% HP
    "movement_pattern": "chase_player",
    "movement_speed_multiplier": 1.6,
    "attack_patterns": [...]
  }
]
```

---

## Exemples Complets

### Exemple: Level Structure Minimale

```json
{
  "level_id": 3,
  "level_name": "Nom du Niveau",
  "level_description": "Description courte",
  "map_id": 3,
  "base_scroll_speed": 70.0,
  "total_chunks": 20,
  "phases": [
    {
      "phase_number": 1,
      "phase_name": "Phase Easy",
      "scroll_start": 0.0,
      "scroll_end": 2400.0,
      "difficulty": "easy",
      "waves": [
        {
          "wave_number": 1,
          "trigger": {"chunkId": 0, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 2,
          "trigger": {"chunkId": 2, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 3,
          "trigger": {"chunkId": 4, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        }
      ]
    },
    {
      "phase_number": 2,
      "phase_name": "Phase Medium",
      "scroll_start": 2400.0,
      "scroll_end": 6240.0,
      "difficulty": "medium",
      "waves": [
        {
          "wave_number": 4,
          "trigger": {"chunkId": 6, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 5,
          "trigger": {"chunkId": 8, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 6,
          "trigger": {"chunkId": 10, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 7,
          "trigger": {"chunkId": 12, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        }
      ]
    },
    {
      "phase_number": 3,
      "phase_name": "Phase Hard",
      "scroll_start": 6240.0,
      "scroll_end": 9600.0,
      "difficulty": "hard",
      "waves": [
        {
          "wave_number": 8,
          "trigger": {"chunkId": 14, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 9,
          "trigger": {"chunkId": 16, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        },
        {
          "wave_number": 10,
          "trigger": {"chunkId": 18, "offset": 0.0, "timeDelay": 0},
          "spawns": [...]
        }
      ]
    }
  ],
  "boss": {
    "boss_name": "Boss Name",
    "spawn_scroll_distance": 9600.0,
    "spawn_position_x": 1600.0,
    "spawn_position_y": 540.0,
    "enemy_type": "boss",
    "total_phases": 3,
    "script_path": "boss/boss_script.lua",
    "phases": [...]
  }
}
```

### Checklist pour Nouveau Niveau

- [ ] `level_id` unique
- [ ] `total_chunks: 20`
- [ ] `base_scroll_speed` adapté (50-80)
- [ ] 3 phases (easy/medium/hard)
- [ ] 10 waves exactement (chunkIds: 0,2,4,6,8,10,12,14,16,18)
- [ ] `wave_number` séquentiel (1-10)
- [ ] Powerups répartis stratégiquement (1 par phase minimum)
- [ ] Boss à `spawn_scroll_distance: 9600.0`
- [ ] Boss script Lua existant dans `assets/scripts/boss/`

---

## Validation

### Validation Syntaxe JSON

Avant de tester dans le jeu, valider la syntaxe JSON:

```bash
python3 -m json.tool src/r-type/assets/levels/level_X.json > /dev/null
```

Si aucune erreur, le fichier est valide.

### Validation Logique

**Vérifier:**
1. ChunkIds en ordre croissant (0,2,4,6...)
2. Tous les offset à 0.0 (pattern strict)
3. Wave numbers séquentiels (1,2,3...)
4. EnemyTypes existent (basic, zigzag, bouncer, tank, kamikaze)
5. Powerup dans au moins 1 wave par phase
6. Boss spawn après dernière wave

### Testing In-Game

```bash
# Compiler le jeu
cmake --build build

# Lancer serveur
./build/bin/r-type_server

# Lancer client
./build/bin/r-type_client

# Sélectionner le niveau à tester
```

**Points de test:**
- ✓ HUD affiche "WAVE 1 / 10" immédiatement
- ✓ Wave 1 se déclenche à chunk 0 (~0px)
- ✓ Wave 2 se déclenche à chunk 2 (~960px)
- ✓ Waves continuent tous les 2 chunks
- ✓ Boss spawn après wave 10 complétée
- ✓ Aucune erreur dans les logs serveur/client

---

## Résumé des Règles

### Pattern 2-Chunks (Règle Absolue)
```
Waves aux chunks: 0, 2, 4, 6, 8, 10, 12, 14, 16, 18
Total waves: 10
Boss spawn: 9600.0 (chunk 20)
```

### Distribution par Phase
```
Phase 1 (Easy):   Waves 1-3  (chunks 0-4)
Phase 2 (Medium): Waves 4-7  (chunks 6-12)
Phase 3 (Hard):   Waves 8-10 (chunks 14-18)
```

### Ennemis par Difficulté
```
Easy:   basic (70%) + zigzag (30%)
Medium: zigzag, bouncer, tank, kamikaze (mix)
Hard:   tank, kamikaze, bouncer (priorité)
```

### Powerups
```
Phase 1: health (wave 3)
Phase 2: shield ou speed (wave 5 ou 7)
Phase 3: health (wave 10, avant boss)
```

---

## Références

- [LEVEL_SYSTEM.md](../../../docs/LEVEL_SYSTEM.md): Documentation complète du système de niveaux
- [WAVE_SYSTEM.md](../../../docs/WAVE_SYSTEM.md): Documentation du système de waves
- [LUA_SCRIPTING.md](../../../docs/LUA_SCRIPTING.md): Guide des scripts Lua pour ennemis/boss

---

**Dernière mise à jour**: 2025-01-17
**Version du pattern**: 2-Chunks (v2.0)
