# Documentation R-Type

Bienvenue dans la documentation du projet R-Type!

## 📚 Documents disponibles

### Architecture

- **[SERVER_ARCHITECTURE.md](SERVER_ARCHITECTURE.md)** - Documentation complète de l'architecture du serveur
  - Vue d'ensemble des composants
  - Flux réseau détaillés
  - Protocole TCP/UDP
  - Gestion des lobbies et sessions
  - Diagrammes de séquence
  - Configuration et déploiement

- **[CLIENT_ARCHITECTURE.md](CLIENT_ARCHITECTURE.md)** - Documentation complète de l'architecture du client
  - Architecture des composants
  - Gestion des entités réseau
  - Synchronisation avec le serveur
  - Prédiction côté client
  - ECS (Entity Component System)
  - Interface utilisateur
  - Configuration et déploiement

### Game Systems

- **[WAVE_SYSTEM.md](WAVE_SYSTEM.md)** - Système de vagues d'ennemis
  - Configuration JSON des vagues
  - Spawning d'ennemis
  - Patterns de déploiement
  - Gestion de la difficulté

- **[PROCEDURAL_GENERATION.md](PROCEDURAL_GENERATION.md)** - Génération procédurale de maps
  - Architecture du système
  - Algorithmes de génération
  - Synchronisation client-serveur
  - Configuration et paramètres
  - Performance et optimisation

### Refactorisation

- **[../REFACTORING.md](../REFACTORING.md)** - Documentation de la refactorisation complète
  - Méthodologie de refactorisation
  - Avant/Après pour le serveur et le client
  - Métriques de code
  - Patterns utilisés
  - Bénéfices et impact

## 🎯 Par où commencer?

### Pour comprendre le projet

1. Lisez d'abord [REFACTORING.md](../REFACTORING.md) pour comprendre la vision globale
2. Puis [SERVER_ARCHITECTURE.md](SERVER_ARCHITECTURE.md) pour le serveur
3. Enfin [CLIENT_ARCHITECTURE.md](CLIENT_ARCHITECTURE.md) pour le client

### Pour développer

#### Côté Serveur
- Consultez [SERVER_ARCHITECTURE.md](SERVER_ARCHITECTURE.md) section "Composants principaux"
- Regardez les diagrammes de flux réseau
- Explorez le protocole TCP/UDP

#### Côté Client
- Consultez [CLIENT_ARCHITECTURE.md](CLIENT_ARCHITECTURE.md) section "Composants principaux"
- Comprenez l'architecture ECS
- Étudiez la synchronisation réseau

#### Game Systems
- Pour les vagues d'ennemis: [WAVE_SYSTEM.md](WAVE_SYSTEM.md)
- Pour la génération de maps: [PROCEDURAL_GENERATION.md](PROCEDURAL_GENERATION.md)

## 🏗️ Architecture générale

```
┌─────────────────────────────────────────────────────────┐
│                    R-Type System                        │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌──────────────┐         Network          ┌─────────┐ │
│  │              │   TCP: 4242 (lobby)      │         │ │
│  │    Client    │◄═══════════════════════►│ Server  │ │
│  │              │   UDP: 4243 (game)       │         │ │
│  └──────────────┘                          └─────────┘ │
│                                                         │
│  • Graphique (Raylib)                • NetworkHandler  │
│  • Input (Raylib)                    • PacketSender    │
│  • Audio (Miniaudio)                 • GameSession     │
│  • Network (Boost.Asio)              • LobbyManager    │
│  • ECS (Custom)                      • ECS (Custom)    │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

## 📁 Structure du code

```
Mirror-R-Type/
├── src/
│   ├── engine/           # Moteur de jeu (ECS, plugins)
│   ├── r-type/
│   │   ├── client/       # Code client
│   │   │   ├── include/
│   │   │   └── src/
│   │   ├── server/       # Code serveur
│   │   │   ├── include/
│   │   │   └── src/
│   │   ├── shared/       # Code partagé (protocole)
│   │   └── game-logic/   # Logique de jeu partagée
│   └── ...
├── docs/                 # 📚 Vous êtes ici!
│   ├── README.md
│   ├── SERVER_ARCHITECTURE.md
│   └── CLIENT_ARCHITECTURE.md
├── REFACTORING.md
└── README.md
```

## 🔧 Compilation

```bash
# Configuration
cmake -B build

# Compilation complète
cmake --build build

# Compilation serveur uniquement
cmake --build build --target r-type_server

# Compilation client uniquement
cmake --build build --target r-type_client
```

## 🚀 Exécution

### Serveur
```bash
./build/r-type_server [tcp_port] [udp_port]

# Exemples:
./build/r-type_server                # Ports par défaut (4242, 4243)
./build/r-type_server 5000 5001      # Ports personnalisés
```

### Client
```bash
./build/r-type_client [host] [tcp_port] [player_name]

# Exemples:
./build/r-type_client                           # Défaut: localhost:4242, "Pilot"
./build/r-type_client 192.168.1.100             # Serveur distant
./build/r-type_client 192.168.1.100 5000 "Bob"  # Tout personnalisé
```

## 🎮 Contrôles

| Touche | Action |
|--------|--------|
| **W** / ↑ | Haut |
| **S** / ↓ | Bas |
| **A** / ← | Gauche |
| **D** / → | Droite |
| **Space** | Tirer |
| **Shift** | Charge |
| **Ctrl** | Spécial |
| **E** | Changer d'arme |
| **Escape** | Quitter |

## 🌐 Protocole réseau

### TCP (Port 4242 par défaut)
- Connexion/Déconnexion
- Gestion des lobbies
- Messages de contrôle
- Notifications importantes

### UDP (Port 4243 par défaut)
- Inputs du joueur (30 Hz)
- State snapshots (60 Hz)
- Spawn/Destroy d'entités
- Projectiles

## 📊 Performance

| Métrique | Valeur |
|----------|--------|
| **Server Tick Rate** | 60 TPS |
| **Client FPS** | 60 (VSync) |
| **Input Rate** | 30 Hz |
| **Snapshot Rate** | 60 Hz |
| **Max Players/Lobby** | 4 |

## 🧪 Tests

```bash
# Lancer les tests
cmake --build build --target test
ctest --test-dir build
```

## 🐛 Debugging

### Serveur
Les logs du serveur affichent:
- État de démarrage
- Connexions/Déconnexions
- Lobbies (création/suppression)
- Sessions (début/fin)
- Erreurs réseau

### Client
Les logs du client affichent:
- Connexion au serveur
- État du lobby
- Début de partie
- Spawns d'entités
- Erreurs

## 🤝 Contribution

Pour contribuer au projet:

1. **Lire la documentation** - Comprendre l'architecture
2. **Suivre les patterns** - Respecter la séparation des responsabilités
3. **Tester** - Vérifier que tout compile et fonctionne
4. **Documenter** - Mettre à jour la doc si nécessaire

## 📝 Conventions de code

- **Naming**: `snake_case` pour variables/fonctions, `PascalCase` pour classes
- **Headers**: Guards `#pragma once`
- **Commentaires**: Doxygen style pour les APIs publiques
- **Format**: Indentation 4 espaces, pas de tabs

## 🔗 Liens utiles

- [CMake Documentation](https://cmake.org/documentation/)
- [Raylib](https://www.raylib.com/)
- [Boost.Asio](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [ECS Architecture](https://github.com/SanderMertens/ecs-faq)

## 📧 Support

Pour toute question:
- Consulter la documentation complète
- Vérifier les diagrammes de flux
- Examiner les exemples de code

---

**Dernière mise à jour**: 2025-12-16

**Version**: 1.0 (Post-refactorisation)

**Status**: Production-ready ✅
