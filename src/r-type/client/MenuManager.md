# MenuManager Documentation


## Flux de navigation

```
MAIN_MENU
  ├→ CREATE_ROOM → (création) → ROOM_LOBBY (host)
  └→ BROWSE_ROOMS → (join) → ROOM_LOBBY (guest)
      └→ Password Dialog (si room privée)

ROOM_LOBBY
  ├→ Leave → MAIN_MENU
  └→ Start Game (host only) → Countdown → Game
```

## Fonctionnalités clés

### 1. Création de Room
- Nom personnalisé
- Password optionnel
- Nombre de joueurs configurable (2-4)
- Mode/difficulté (actuellement SQUAD/NORMAL)

### 2. Navigation des Rooms
- Liste actualisée du serveur
- Boutons "Join" dynamiques
- Indication des rooms privées
- Dialog de password pour rooms protégées
- Bouton Refresh manuel

### 3. Lobby de Room
- **Host**: Voit les contrôles (min players, Start Game)
- **Guest**: Voit "Waiting for host..."
- Affichage en temps réel des joueurs
- Slots colorés (vert=connecté, gris=vide)
- Countdown visible par tous
- Refresh automatique (1s) du nombre de joueurs

### 4. Countdown de Start
- Démarre à 5 secondes
- Visible par tous les joueurs
- Bloque les contrôles pendant le countdown
- Envoi de SERVER_GAME_START à la fin

## Callbacks Réseau

### Reçus du NetworkClient
- `on_room_created` - Room créée avec succès
- `on_room_joined` - Rejoint une room avec succès
- `on_room_list` - Liste des rooms disponibles
- `on_room_error` - Erreur (room pleine, password invalide, etc.)
- `on_countdown` - Mise à jour du countdown

### Envoyés au serveur
- `send_create_room()` - Demande de création
- `send_join_room()` - Demande de join
- `send_leave_room()` - Quitter la room
- `send_start_game()` - Démarrer le jeu (host)
- `send_request_room_list()` - Refresh de la liste

## Amélirations futures possibles

1. **Refactoring**
   - Extraire chaque écran dans sa propre classe
   - Créer une classe BaseScreen
   - Pattern State Machine pour les transitions

2. **Fonctionnalités**
   - Sélection de mode de jeu (DUO, TRIO, SQUAD)
   - Sélection de difficulté
   - Sélection de map
   - Chat dans le lobby
   - Indicateur de ping

3. **UX**
   - Animations de transition
   - Sons UI
   - Meilleure indication de qui est l'hôte
   - Avatars des joueurs

## Notes techniques

- Résolution basée sur `screen_width_` et `screen_height_`
- Utilise ~60 FPS (0.016f delta time assumé)
- Rafraîchissement automatique toutes les 1 seconde en lobby
- Messages d'erreur avec timer de 3 secondes
- Countdown de 5 secondes avant start

## Dépendances

- `NetworkClient` - Communication serveur
- `IGraphicsPlugin` - Rendu
- `IInputPlugin` - Input souris/clavier
- UI Components (UIButton, UITextField, UILabel)
- Protocol (RoomInfo, payloads, etc.)
