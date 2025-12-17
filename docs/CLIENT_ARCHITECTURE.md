# Architecture du Client R-Type

## Vue d'ensemble

Le client R-Type est une application graphique multijoueur qui se connecte au serveur de jeu, affiche le rendu du jeu et envoie les inputs du joueur.

## Architecture globale

```
┌─────────────────────────────────────────────────────────────┐
│                       main.cpp                              │
│                    (Point d'entrée)                         │
└────────────────────────┬────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│                     ClientGame                              │
│              (Orchestration du jeu)                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────┐ ┌─────────┐ ┌──────────┐ ┌───────────────┐ │
│  │ Texture  │ │ Input   │ │ Screen   │ │    Entity     │ │
│  │ Manager  │ │ Handler │ │ Manager  │ │   Manager     │ │
│  └──────────┘ └─────────┘ └──────────┘ └───────────────┘ │
│                                                             │
│  ┌──────────────┐      ┌─────────────────────────────────┐│
│  │    Status    │      │       NetworkClient             ││
│  │   Overlay    │      │   (Gestion réseau)              ││
│  └──────────────┘      └─────────────────────────────────┘│
│                                                             │
└─────────────────────────────────────────────────────────────┘
          │                        │                    │
          ▼                        ▼                    ▼
  ┌───────────────┐       ┌──────────────┐    ┌───────────────┐
  │   Registry    │       │   Plugins    │    │  INetwork     │
  │     (ECS)     │       │ (Graphics,   │    │   Plugin      │
  │               │       │  Input, etc) │    │               │
  └───────────────┘       └──────────────┘    └───────────────┘
```

## Composants principaux

### 1. ClientGame (Classe principale)

**Fichier**: `src/r-type/client/include/ClientGame.hpp`

**Responsabilités**:
- Orchestration de tous les composants du jeu
- Initialisation des plugins (graphics, input, audio, network)
- Configuration du registre ECS
- Setup des systèmes de jeu
- Boucle de jeu principale
- Gestion des callbacks réseau

**Membres clés**:
```cpp
class ClientGame {
private:
    // Plugins
    engine::IGraphicsPlugin* graphics_plugin_;
    engine::IInputPlugin* input_plugin_;
    engine::IAudioPlugin* audio_plugin_;
    engine::INetworkPlugin* network_plugin_;

    // ECS
    std::unique_ptr<Registry> registry_;

    // Composants de jeu
    std::unique_ptr<TextureManager> texture_manager_;
    std::unique_ptr<ScreenManager> screen_manager_;
    std::unique_ptr<EntityManager> entity_manager_;
    std::unique_ptr<StatusOverlay> status_overlay_;
    std::unique_ptr<InputHandler> input_handler_;

    // Réseau
    std::unique_ptr<NetworkClient> network_client_;
};
```

**Cycle de vie**:
```
initialize()
    ├─► load_plugins()
    ├─► setup_registry()
    ├─► setup_systems()
    ├─► load_textures()
    ├─► setup_background()
    ├─► setup_network_callbacks()
    └─► connect()
        ↓
run()  [Boucle principale]
    ├─► update(dt)
    ├─► handle_input()
    ├─► update_projectiles()
    ├─► run_systems()
    └─► display()
        ↓
shutdown()
    ├─► disconnect()
    └─► cleanup_plugins()
```

### 2. TextureManager

**Fichier**: `src/r-type/client/include/TextureManager.hpp`

**Responsabilités**:
- Chargement de toutes les textures au démarrage
- Gestion centralisée des handles de texture
- Fallbacks pour les textures manquantes

**Textures gérées**:
- `background` - Fond d'écran scrollant
- `menu_background` - Fond des menus
- `player_frames[4]` - Frames d'animation du joueur
- `enemy` - Sprite des ennemis
- `projectile` - Sprite des projectiles
- `wall` - Sprite des obstacles

**Méthodes principales**:
```cpp
bool load_all();  // Charge toutes les textures
engine::TextureHandle get_background() const;
engine::TextureHandle get_player_frame(size_t index) const;
// ... etc
```

### 3. InputHandler

**Fichier**: `src/r-type/client/include/InputHandler.hpp`

**Responsabilités**:
- Lecture des inputs du joueur
- Conversion en flags réseau (protocole)
- Détection de touches spéciales

**Mapping des touches**:
```
W / Up Arrow    → INPUT_UP
S / Down Arrow  → INPUT_DOWN
A / Left Arrow  → INPUT_LEFT
D / Right Arrow → INPUT_RIGHT
Space           → INPUT_SHOOT
Shift           → INPUT_CHARGE
Ctrl            → INPUT_SPECIAL
E               → INPUT_SWITCH_WEAPON
Escape          → Quitter le jeu
```

**Utilisation**:
```cpp
uint16_t flags = input_handler_->gather_input();
network_client_->send_input(flags, client_tick_);
```

### 4. ScreenManager

**Fichier**: `src/r-type/client/include/ScreenManager.hpp`

**Responsabilités**:
- Gestion des états d'écran (State Pattern)
- Transitions entre écrans
- Affichage/masquage des overlays
- Gestion des écrans de résultat

**États disponibles**:
```cpp
enum class GameScreen {
    WAITING,   // En attente de joueurs
    PLAYING,   // En jeu
    VICTORY,   // Victoire
    DEFEAT     // Défaite
};
```

**Transitions**:
```
WAITING (Lobby)
    ↓
PLAYING (Game start)
    ↓
VICTORY / DEFEAT (Game over)
    ↓
(Retour possible au lobby)
```

**Écrans gérés**:
- Écran d'attente (fond + texte "En attente de joueurs...")
- Écran de résultat (fond + texte "VICTOIRE !" ou "DEFAITE...")
- Overlay de statut (connexion, lobby, ping)

### 5. StatusOverlay

**Fichier**: `src/r-type/client/include/StatusOverlay.hpp`

**Responsabilités**:
- Affichage du statut de connexion
- Affichage de l'état du lobby
- Affichage de la session en cours
- Affichage du ping

**Format d'affichage**:
```
Connected (Player 1234) | Lobby 1: 3/4 | In game (session 5678) | Ping: 25ms
```

**Méthodes**:
```cpp
void set_connection(const std::string& status);
void set_lobby(const std::string& status);
void set_session(const std::string& status);
void set_ping(int ping_ms);
void refresh();  // Met à jour l'affichage
```

### 6. EntityManager

**Fichier**: `src/r-type/client/include/EntityManager.hpp`

**Responsabilités**:
- Gestion complète des entités réseau
- Synchronisation avec le serveur
- Spawn/Update/Destroy d'entités
- Prédiction côté client (projectiles)
- Gestion des entités périmées (stale entities)
- Name tags des joueurs

**Tracking des entités**:
```cpp
std::unordered_map<uint32_t, Entity> server_to_local_;      // server_id → entity
std::unordered_map<uint32_t, EntityType> server_types_;     // Type de l'entité
std::unordered_map<uint32_t, uint8_t> stale_counters_;      // Compteur de vieillissement
std::unordered_set<uint32_t> locally_integrated_;           // Projectiles (prédiction)
```

**Méthodes principales**:
```cpp
Entity spawn_or_update_entity(server_id, type, x, y, health, subtype);
void remove_entity(server_id);
void clear_all();  // Nettoyage complet
void process_snapshot_update(updated_ids);  // Détection des entités périmées
void update_projectiles(delta_time);  // Prédiction locale
void update_name_tags();  // Mise à jour des positions
```

**Construction de sprites**:

Chaque type d'entité a un sprite spécifique:
```cpp
switch (type) {
    case PLAYER:        → Sprite animé cyan/blanc
    case ENEMY_BASIC:   → Sprite ennemi standard
    case ENEMY_FAST:    → Sprite orange
    case ENEMY_TANK:    → Sprite rouge
    case ENEMY_BOSS:    → Sprite violet
    case PROJECTILE:    → Sprite cyan/rouge
    case POWERUP:       → Sprite vert
    // ...
}
```

### 7. NetworkClient

**Fichier**: `src/r-type/client/include/NetworkClient.hpp`

**Responsabilités**:
- Gestion de la connexion TCP/UDP avec le serveur
- Envoi de paquets
- Réception et décodage de paquets
- Callbacks pour les événements réseau

**Méthodes principales**:
```cpp
bool connect(host, tcp_port);
void disconnect();
void update();  // Process packets

// Envoi
void send_connect(player_name);
void send_join_lobby(mode, difficulty);
void send_input(flags, tick);
void send_ping();

// Callbacks
void set_on_accepted(callback);
void set_on_lobby_state(callback);
void set_on_game_start(callback);
void set_on_entity_spawn(callback);
void set_on_snapshot(callback);
// ... etc
```

## Architecture ECS (Entity Component System)

### Registry

Le registre ECS gère tous les composants et entités du jeu.

**Composants enregistrés**:
```cpp
registry->register_component<Position>();
registry->register_component<Velocity>();
registry->register_component<Sprite>();
registry->register_component<SpriteAnimation>();
registry->register_component<Collider>();
registry->register_component<Health>();
registry->register_component<Score>();
registry->register_component<Controllable>();
registry->register_component<NetworkId>();
registry->register_component<UIText>();
// ... etc
```

### Systèmes

**Ordre d'exécution** (chaque frame):
1. `ScrollingSystem` - Défilement du background
2. `SpriteAnimationSystem` - Animation des sprites
3. `RenderSystem` - Rendu graphique
4. `HUDSystem` - Affichage de l'interface

## Flux réseau

### Connexion au serveur

```
Client                                  Server
  │                                       │
  ├──► connect(host, tcp_port) ─────────►│
  │                                       │
  ├──► CLIENT_CONNECT ──────────────────►│
  │     (player_name)                     │
  │                                       │
  │◄──── SERVER_ACCEPT ───────────────────┤
  │      (assigned_player_id)             │
  │                                       │
  │  [Callback: on_accepted()]            │
  │                                       │
  ├──► CLIENT_JOIN_LOBBY ────────────────►│
  │                                       │
  │◄──── SERVER_LOBBY_STATE ──────────────┤
  │                                       │
```

### Début de partie

```
Client                                  Server
  │                                       │
  │◄──── SERVER_GAME_START_COUNTDOWN ────┤
  │      (10, 9, 8, ...)                  │
  │                                       │
  │  [Callback: on_countdown()]           │
  │                                       │
  │◄──── SERVER_GAME_START ───────────────┤
  │      (session_id, udp_port)           │
  │                                       │
  │  [Callback: on_game_start()]          │
  │  - Clear entities                     │
  │  - Switch to PLAYING screen           │
  │                                       │
  ├══► CLIENT_UDP_HANDSHAKE ═════════════►│
  │                                       │
  │◄═══ SERVER_ENTITY_SPAWN ══════════════┤
  │     (players, enemies, walls...)      │
  │                                       │
```

**Légende**: `───►` TCP, `═══►` UDP

### Boucle de jeu

```
Client                                  Server
  │                                       │
  │  [Every 30ms]                         │
  ├══► CLIENT_INPUT ═════════════════════►│
  │     (input_flags, client_tick)        │
  │                                       │
  │  [Every ~16ms]                        │
  │◄═══ SERVER_DELTA_SNAPSHOT ════════════┤
  │     (entity states)                   │
  │                                       │
  │  [EntityManager processes]            │
  │  - Update positions                   │
  │  - Update velocities                  │
  │  - Update health                      │
  │  - Remove stale entities              │
  │                                       │
  │◄═══ SERVER_ENTITY_SPAWN ══════════════┤
  │◄═══ SERVER_ENTITY_DESTROY ════════════┤
  │◄═══ SERVER_PROJECTILE_SPAWN ══════════┤
  │                                       │
```

### Prédiction côté client (Projectiles)

Pour réduire la latence perçue, les projectiles sont mis à jour localement:

```cpp
// Marquage lors du spawn
locally_integrated_.insert(projectile_id);

// Mise à jour locale chaque frame
void EntityManager::update_projectiles(float dt) {
    for (auto projectile_id : locally_integrated_) {
        if (!in_latest_snapshot) {
            // Mise à jour prédictive
            pos.x += vel.x * dt;
            pos.y += vel.y * dt;

            // Despawn si hors écran
            if (out_of_bounds) {
                remove_entity(projectile_id);
            }
        }
    }
}
```

### Fin de partie

```
Client                                  Server
  │                                       │
  │◄═══ SERVER_GAME_OVER ═════════════════┤
  │     (result: VICTORY/DEFEAT)          │
  │                                       │
  │  [Callback: on_game_over()]           │
  │  - Show result screen                 │
  │  - Update status overlay              │
  │                                       │
```

## Gestion des entités périmées (Stale Entities)

Pour détecter les entités qui n'existent plus côté serveur mais n'ont pas reçu de paquet DESTROY:

```cpp
void EntityManager::process_snapshot_update(updated_ids) {
    for (auto& [server_id, entity] : server_to_local_) {
        if (!updated_ids.contains(server_id)) {
            // Entité non mise à jour dans ce snapshot
            stale_counters_[server_id]++;

            if (stale_counters_[server_id] > THRESHOLD) {
                // Suppression après ~6 frames sans update
                remove_entity(server_id);
            }
        } else {
            // Reset du compteur
            stale_counters_[server_id] = 0;
        }
    }
}
```

## Synchronisation Audio-Visuelle

### Callbacks réseau → Effets visuels/sonores

```cpp
network_client_->set_on_projectile_spawn([this](...) {
    // Spawn visuel
    entity_manager_->spawn_or_update_entity(...);

    // Son de tir (si audio_plugin disponible)
    if (audio_plugin_) {
        audio_plugin_->play_sound("laser.wav");
    }
});

network_client_->set_on_entity_destroy([this](...) {
    // Effet d'explosion
    create_particle_effect(...);

    // Son d'explosion
    if (audio_plugin_) {
        audio_plugin_->play_sound("explosion.wav");
    }
});
```

## Résolution d'écran et mise à l'échelle

**Résolution par défaut**: 1920x1080

Le jeu utilise des dimensions relatives pour s'adapter:
```cpp
constexpr int SCREEN_WIDTH = 1920;
constexpr int SCREEN_HEIGHT = 1080;

// Positions centrées
float center_x = SCREEN_WIDTH / 2.0f;
float center_y = SCREEN_HEIGHT / 2.0f;
```

## Performance et optimisation

### Fréquences d'update

- **Réseau (envoi inputs)**: 30 Hz (~33ms)
- **Réseau (réception snapshots)**: 60 Hz (~16ms)
- **Rendu**: VSync (60 FPS généralement)
- **Mise à jour overlay**: 2 Hz (500ms)
- **Ping**: Toutes les 5 secondes

### Optimisations

1. **Prédiction locale**: Projectiles mis à jour côté client
2. **Delta snapshots**: Seules les entités qui changent
3. **Stale entity removal**: Nettoyage automatique
4. **Sprite batching**: Par le RenderSystem
5. **Dirty flags**: Mise à jour conditionnelle des name tags

## Configuration

**Ligne de commande**:
```bash
./r-type_client [host] [tcp_port] [player_name]

# Exemples:
./r-type_client                           # Defaults: 127.0.0.1:4242, "Pilot"
./r-type_client 192.168.1.100             # Custom host
./r-type_client 192.168.1.100 5000        # Custom host + port
./r-type_client 192.168.1.100 5000 "Bob"  # + custom name
```

## Gestion d'erreurs

### Échec de connexion

```cpp
if (!network_client_->connect(host, tcp_port)) {
    std::cerr << "Failed to connect to server\n";
    return 1;
}
```

### Déconnexion inattendue

```cpp
network_client_->set_on_disconnected([this]() {
    status_overlay_->set_connection("Disconnected");
    status_overlay_->refresh();
    running_ = false;  // Arrêt de la boucle de jeu
});
```

### Rejet par le serveur

```cpp
network_client_->set_on_rejected([this](reason, message) {
    status_overlay_->set_connection("Rejected: " + message);
    status_overlay_->refresh();
});
```

## Diagramme de flux complet

```
┌─────────┐
│  main() │
└────┬────┘
     │
     ▼
┌─────────────────────────────────────────┐
│  ClientGame::initialize()               │
├─────────────────────────────────────────┤
│  1. Load plugins                        │
│  2. Create window                       │
│  3. Setup Registry + Systems            │
│  4. Load textures (TextureManager)      │
│  5. Create managers                     │
│  6. Setup network callbacks             │
│  7. Connect to server                   │
└────┬────────────────────────────────────┘
     │
     ▼
┌─────────────────────────────────────────┐
│  ClientGame::run()                      │
│  ┌─────────────────────────────────┐   │
│  │  GAME LOOP (60 FPS)             │   │
│  ├─────────────────────────────────┤   │
│  │  1. Network update              │   │
│  │  2. Process packets             │   │
│  │  3. Handle input                │   │
│  │  4. Update projectiles          │   │
│  │  5. Update name tags            │   │
│  │  6. Run ECS systems             │   │
│  │  7. Render                      │   │
│  │  8. Check exit condition        │   │
│  └─────────────────────────────────┘   │
└────┬────────────────────────────────────┘
     │
     ▼
┌─────────────────────────────────────────┐
│  ClientGame::shutdown()                 │
├─────────────────────────────────────────┤
│  1. Disconnect from server              │
│  2. Clear entities                      │
│  3. Shutdown plugins                    │
└─────────────────────────────────────────┘
```

## Déploiement

### Compilation

```bash
cmake -B build
cmake --build build --target r-type_client
```

### Ressources requises

Le client nécessite le dossier `assets/` avec:
- `sprite/symmetry.png` - Background
- `sprite/background_rtype_menu.png` - Menu background
- `sprite/ship1.png`, `ship2.png`, `ship3.png`, `ship4.png` - Player frames
- `sprite/enemy.png` - Enemy sprite
- `sprite/bullet.png` - Projectile sprite
- `sprite/lock.png` - Wall sprite

## Dépendances

- **game_engine**: ECS, plugins, systèmes
- **rtype_logic**: Logique partagée (dimensions, etc.)
- **Raylib**: Graphisme et input via plugins
- **Miniaudio**: Audio via plugin
- **Boost.Asio**: Réseau via plugin

## Conclusion

L'architecture modulaire du client offre:
- **Séparation des responsabilités**: Chaque composant a un rôle clair
- **Maintenabilité**: Code organisé et facile à modifier
- **Extensibilité**: Ajout de features simplifié
- **Performance**: Optimisations réseau et rendu
- **UX**: Feedback visuel clair et réactivité
