# Architecture du Serveur R-Type

## Vue d'ensemble

Le serveur R-Type est une application réseau hybride TCP/UDP qui gère les connexions des clients, les lobbies de jeu et les sessions de jeu multijoueurs.

## Architecture globale

```
┌─────────────────────────────────────────────────────────┐
│                       Server                            │
│  (Orchestration et gestion des joueurs)                │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  ┌─────────────┐  ┌──────────────┐  ┌────────────────┐│
│  │  Network    │  │  Packet      │  │  GameSession   ││
│  │  Handler    │  │  Sender      │  │  Manager       ││
│  └─────────────┘  └──────────────┘  └────────────────┘│
│         │                 │                  │         │
└─────────┼─────────────────┼──────────────────┼─────────┘
          │                 │                  │
          ▼                 ▼                  ▼
  ┌───────────────┐ ┌──────────────┐ ┌─────────────────┐
  │ INetwork      │ │ INetwork     │ │ GameSession     │
  │ Plugin        │ │ Plugin       │ │ (instances)     │
  └───────────────┘ └──────────────┘ └─────────────────┘
```

## Composants principaux

### 1. Server (Classe principale)

**Fichier**: `src/r-type/server/include/Server.hpp`

**Responsabilités**:
- Orchestration de tous les composants
- Gestion du cycle de vie du serveur (start/stop/run)
- Gestion de la liste des joueurs connectés
- Coordination entre lobbies et sessions de jeu

**Membres clés**:
```cpp
class Server {
private:
    // Composants
    std::unique_ptr<NetworkHandler> network_handler_;
    std::unique_ptr<PacketSender> packet_sender_;
    std::unique_ptr<GameSessionManager> session_manager_;

    // État
    std::unordered_map<uint32_t, PlayerInfo> connected_clients_;
    LobbyManager lobby_manager_;

    // Network
    engine::INetworkPlugin* network_plugin_;
};
```

**Méthodes principales**:
- `bool start()` - Initialise et démarre le serveur
- `void run()` - Boucle principale du serveur (60 TPS)
- `void stop()` - Arrêt gracieux du serveur

### 2. NetworkHandler

**Fichier**: `src/r-type/server/include/NetworkHandler.hpp`

**Responsabilités**:
- Réception et décodage des paquets réseau
- Validation de la version du protocole
- Routage des paquets vers les handlers appropriés

**Architecture**:
```cpp
NetworkHandler::process_packets()
    ↓
receive() → decode_header() → validate_version()
    ↓
route_packet()
    ├─→ handle_tcp_packet() → callback (CLIENT_CONNECT, etc.)
    └─→ handle_udp_packet() → callback (CLIENT_INPUT, etc.)
```

**Callbacks disponibles**:
- `on_client_connect` - Connexion d'un nouveau client
- `on_client_disconnect` - Déconnexion d'un client
- `on_client_ping` - Requête de ping
- `on_client_join_lobby` - Demande de rejoindre un lobby
- `on_client_leave_lobby` - Demande de quitter un lobby
- `on_udp_handshake` - Handshake UDP
- `on_client_input` - Input du joueur

### 3. PacketSender

**Fichier**: `src/r-type/server/include/PacketSender.hpp`

**Responsabilités**:
- Envoi de paquets TCP (fiable, ordonné)
- Envoi de paquets UDP (rapide, non fiable)
- Broadcasting vers lobbies et sessions

**Méthodes principales**:

**TCP**:
- `send_tcp_packet(client_id, type, payload)` - Envoi unicast
- `broadcast_tcp_packet(type, payload)` - Envoi à tous les clients
- `broadcast_tcp_to_lobby(lobby_id, type, payload)` - Envoi au lobby

**UDP**:
- `send_udp_packet(client_id, type, payload)` - Envoi unicast
- `broadcast_udp_to_session(session_id, type, payload)` - Envoi à la session

**Format de paquet**:
```
┌────────────┬────────┬─────────────┬──────────────┬─────────┐
│  Version   │  Type  │  Payload    │   Sequence   │ Payload │
│  (1 byte)  │(1 byte)│  Length     │   Number     │  Data   │
│            │        │  (2 bytes)  │  (4 bytes)   │         │
└────────────┴────────┴─────────────┴──────────────┴─────────┘
```

### 4. GameSessionManager

**Fichier**: `src/r-type/server/include/GameSessionManager.hpp`

**Responsabilités**:
- Création et destruction des sessions de jeu
- Mise à jour de toutes les sessions actives
- Nettoyage automatique des sessions inactives
- Configuration des callbacks de session

**Cycle de vie d'une session**:
```
create_session()
    ↓
setup_callbacks()
    ↓
update_all(delta_time)  [Appelé chaque tick]
    ↓
cleanup_inactive_sessions()
    ↓
remove_session() / on_game_over()
```

**Callbacks de session**:
- `on_state_snapshot` - Snapshot d'état des entités
- `on_entity_spawn` - Apparition d'une nouvelle entité
- `on_entity_destroy` - Destruction d'une entité
- `on_projectile_spawn` - Apparition d'un projectile
- `on_wave_start` - Début d'une vague
- `on_wave_complete` - Fin d'une vague
- `on_game_over` - Fin de partie

### 5. LobbyManager

**Fichier**: `src/r-type/server/include/LobbyManager.hpp`

**Responsabilités**:
- Gestion des lobbies de jeu (création/suppression)
- Matchmaking des joueurs
- Countdown avant le début de la partie
- Notification de l'état des lobbies

**États d'un lobby**:
```
WAITING (< min_players)
    ↓
READY (>= min_players)
    ↓
COUNTDOWN (10 secondes)
    ↓
STARTING
    ↓
[Game Session créée]
```

## Flux réseau

### Connexion d'un client

```
Client                  Server
  │                       │
  ├──► CLIENT_CONNECT ───►│
  │                       ├─► Valider le paquet
  │                       ├─► Créer PlayerInfo
  │                       ├─► Assigner player_id
  │                       │
  │◄─── SERVER_ACCEPT ────┤
  │                       │
```

### Création et démarrage d'une partie

```
Client 1, 2, 3          Server                    GameSession
  │                       │                           │
  ├──► JOIN_LOBBY ───────►│                           │
  │                       ├─► lobby_manager.join()    │
  │◄─── LOBBY_STATE ──────┤                           │
  │                       │                           │
  │     [Min players atteint]                         │
  │                       │                           │
  │◄─── COUNTDOWN (10) ───┤                           │
  │◄─── COUNTDOWN (9) ────┤                           │
  │        ...            │                           │
  │◄─── COUNTDOWN (1) ────┤                           │
  │                       │                           │
  │◄─── GAME_START ───────┤──► create_session() ────►│
  │                       │                           │
  ├──► UDP_HANDSHAKE ────►│──► associate_udp()       │
  │                       │                           │
  │◄═══ ENTITY_SPAWN ═════╬═══════════════════════════│
  │◄═══ WAVE_START ═══════╬═══════════════════════════│
  │                       │                           │
```

**Légende**: `───►` TCP, `═══►` UDP

### Boucle de jeu

```
Client                  Server                    GameSession
  │                       │                           │
  ├══► CLIENT_INPUT ═════►│──► handle_input() ───────►│
  │     (30 Hz)           │                           │
  │                       │                           │
  │                       │      update(dt)           │
  │                       │          ├─► MovementSystem
  │                       │          ├─► CollisionSystem
  │                       │          ├─► AISystem
  │                       │          └─► etc.
  │                       │                           │
  │◄═══ DELTA_SNAPSHOT ═══╬═══════════════════════════│
  │     (60 Hz)           │                           │
  │                       │                           │
```

## Protocole réseau

### Séparation TCP/UDP

**TCP** (Port 4242 par défaut):
- Connexion/Déconnexion
- Gestion des lobbies
- Messages de contrôle
- Notifications importantes

**UDP** (Port 4243 par défaut):
- Inputs des joueurs
- State snapshots
- Spawn/Destroy des entités
- Projectiles

### Types de paquets

**TCP - Client → Server**:
- `CLIENT_CONNECT` - Demande de connexion
- `CLIENT_DISCONNECT` - Déconnexion volontaire
- `CLIENT_PING` - Mesure de latence
- `CLIENT_JOIN_LOBBY` - Rejoindre un lobby
- `CLIENT_LEAVE_LOBBY` - Quitter un lobby

**TCP - Server → Client**:
- `SERVER_ACCEPT` - Acceptation de connexion
- `SERVER_REJECT` - Rejet de connexion
- `SERVER_PONG` - Réponse au ping
- `SERVER_LOBBY_STATE` - État du lobby
- `SERVER_GAME_START_COUNTDOWN` - Countdown
- `SERVER_GAME_START` - Début de partie

**UDP - Client → Server**:
- `CLIENT_UDP_HANDSHAKE` - Association UDP/TCP
- `CLIENT_INPUT` - Input du joueur

**UDP - Server → Client**:
- `SERVER_DELTA_SNAPSHOT` - État des entités
- `SERVER_ENTITY_SPAWN` - Apparition d'entité
- `SERVER_ENTITY_DESTROY` - Destruction d'entité
- `SERVER_PROJECTILE_SPAWN` - Apparition de projectile
- `SERVER_WAVE_START` - Début de vague
- `SERVER_WAVE_COMPLETE` - Fin de vague
- `SERVER_GAME_OVER` - Fin de partie

## Configuration

**Fichier**: `src/r-type/server/include/ServerConfig.hpp`

**Constantes importantes**:
```cpp
namespace rtype::server::config {
    constexpr uint16_t DEFAULT_TCP_PORT = 4242;
    constexpr uint16_t DEFAULT_UDP_PORT = 4243;
    constexpr uint32_t SERVER_TICK_RATE = 60;      // 60 TPS
    constexpr uint32_t TICK_INTERVAL_MS = 16;      // ~16ms
    constexpr uint32_t MAX_PLAYERS_PER_LOBBY = 4;
}
```

## Gestion des joueurs

### Structure PlayerInfo

```cpp
struct PlayerInfo {
    uint32_t client_id;      // ID de connexion TCP
    uint32_t player_id;      // ID unique du joueur
    std::string player_name;
    uint32_t udp_client_id;  // ID de connexion UDP
    bool in_lobby;
    uint32_t lobby_id;
    bool in_game;
    uint32_t session_id;
};
```

### Cycle de vie d'un joueur

```
1. Connexion TCP
   ↓
2. Attribution player_id
   ↓
3. Acceptation (SERVER_ACCEPT)
   ↓
4. Rejoindre un lobby
   ↓
5. Handshake UDP
   ↓
6. Début de partie
   ↓
7. En jeu (réception d'inputs, envoi de snapshots)
   ↓
8. Fin de partie
   ↓
9. Retour au lobby ou déconnexion
```

## Gestion d'erreurs

### Déconnexion inattendue

Le serveur détecte les déconnexions via:
- Timeout TCP (pas de keepalive)
- Callback `on_client_disconnected` du plugin réseau

Action:
- Retrait du lobby si applicable
- Notification aux autres joueurs du lobby
- Nettoyage des ressources

### Paquets invalides

- Validation de la version du protocole
- Validation de la taille du payload
- Logs d'erreur sans crash

### Session orpheline

Le `GameSessionManager` nettoie automatiquement les sessions inactives à chaque tick.

## Performance

### Tick Rate

- **Serveur**: 60 TPS (Ticks Per Second)
- **Client Input**: ~30 Hz
- **State Snapshot**: 60 Hz

### Optimisations

1. **Delta Snapshots**: Seules les entités qui ont changé sont envoyées
2. **Entity Prioritization**: Les snapshots priorisent les entités critiques:
   - **PLAYER** (priorité maximale) - toujours inclus
   - **PROJECTILE** (haute priorité) - inclus pour un gameplay précis
   - **ENEMY** (priorité moyenne) - inclus pour la synchronisation
   - **WALL** exclus des snapshots (défilement prédictible côté client)
3. **Snapshot Size Limit**: Maximum 55 entités par snapshot (payload 1387 bytes / 25 bytes par EntityState)
4. **UDP pour gameplay**: Réduction de la latence
5. **Pooling d'entités**: Réutilisation des entités détruites
6. **Spatial partitioning**: Pour les collisions (dans GameSession)

## Déploiement

### Compilation

```bash
cmake -B build
cmake --build build --target r-type_server
```

### Exécution

```bash
# Port par défaut (4242 TCP, 4243 UDP)
./r-type_server

# Ports personnalisés
./r-type_server <tcp_port> <udp_port>
```

### Logs

Le serveur affiche:
- État de démarrage
- Connexions/Déconnexions
- Création/Destruction de lobbies
- Début/Fin de parties
- Erreurs réseau

## Dépendances

- **game_engine**: ECS, systèmes, plugins
- **rtype_logic**: Logique de jeu (collision, AI, etc.)
- **Boost.Asio**: Plugin réseau
- **Protocol**: Headers partagés client/serveur

## Diagramme de séquence complet

```
┌──────┐  ┌────────┐  ┌──────────────┐  ┌───────────────┐  ┌─────────────┐
│Client│  │ Server │  │NetworkHandler│  │ LobbyManager  │  │GameSession  │
└──┬───┘  └───┬────┘  └──────┬───────┘  └───────┬───────┘  └──────┬──────┘
   │          │               │                  │                 │
   ├─CONNECT─►│               │                  │                 │
   │          ├─receive()────►│                  │                 │
   │          │               ├─decode()         │                 │
   │          │               ├─route()          │                 │
   │          │◄──callback────┤                  │                 │
   │          ├─create_player()                  │                 │
   │◄─ACCEPT──┤               │                  │                 │
   │          │               │                  │                 │
   ├JOIN_LOBBY│               │                  │                 │
   │          ├──────────────►│──callback───────►│                 │
   │          │               │                  ├─join()          │
   │◄LOBBY────┤◄──broadcast───┼──────────────────┤                 │
   │  STATE   │               │                  │                 │
   │          │               │           [countdown loop]         │
   │◄COUNTDOWN┤◄──broadcast───┼──────────────────┤                 │
   │          │               │                  │                 │
   │          │               │                  ├─on_game_start() │
   │          │               │                  │                 │
   │◄GAME_────┤               │                  ├──create────────►│
   │  START   │               │                  │                 │
   │          │               │                  │                 ├─setup()
   ├UDP_HAND─►│               │                  │                 │
   │  SHAKE   ├──────────────►│──callback────────┼─────────────────►
   │          │               │                  │                 ├associate
   │          │               │                  │                 │
   │          │               │                [game loop 60Hz]    │
   │          │               │                  │                 │
   ├═INPUT═══►│═══════════════╬══════════════════╬═════════════════►
   │          │               │                  │                 ├─update()
   │◄═SNAPSHOT│◄══════════════╬══════════════════╬═════════════════┤
   │          │               │                  │                 │
   │          │               │                [game over]         │
   │          │               │                  │                 │
   │◄GAME_────┤◄══════════════╬══════════════════╬═════════════════┤
   │  OVER    │               │                  │                 │
   │          │               │                  │                 ▼
```

## Conclusion

L'architecture modulaire du serveur permet:
- **Maintenabilité**: Code facile à comprendre et modifier
- **Extensibilité**: Ajout de nouvelles fonctionnalités simplifié
- **Testabilité**: Chaque composant peut être testé indépendamment
- **Performance**: Tick rate élevé avec optimisations réseau
- **Robustesse**: Gestion d'erreurs et cleanup automatique
