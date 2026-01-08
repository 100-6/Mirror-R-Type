# CreateRoom Module - Architecture Documentation

## 📁 Structure Modulaire

```
createroom/
├── CreateRoomConfig.hpp              # Configuration & constantes
├── CreateRoomInput.hpp/cpp           # Gestion des inputs (clics circulaires)
├── CreateRoomRenderer.hpp/cpp        # Rendu bas niveau (textures, shapes)
├── CreateRoomInitializer.hpp/cpp     # Initialisation des UI elements
├── CreateRoomUpdater.hpp/cpp         # Logique de mise à jour
└── CreateRoomDrawer.hpp/cpp          # Logique de dessin haut niveau
```

## 🎯 Responsabilités de chaque module

### **CreateRoomConfig.hpp**
- **Rôle**: Définit toutes les constantes de layout et styling
- **Contenu**:
  - Dimensions (tailles, espacements)
  - Couleurs (background, bordures, glow)
  - Chemins des assets (textures)
- **Utilisation**: Importé par tous les autres modules

### **CreateRoomInput.hpp/cpp**
- **Rôle**: Gère toute la logique d'input utilisateur
- **Fonctions principales**:
  - `handle_difficulty_click()` - Détection de clic circulaire pour difficultés
  - `handle_gamemode_click()` - Détection de clic circulaire pour modes de jeu
  - `is_point_in_circle()` - Utilitaire de collision circulaire
- **Dépendances**: IInputPlugin, protocol::Payloads

### **CreateRoomRenderer.hpp/cpp**
- **Rôle**: Rendu bas niveau des éléments graphiques
- **Classe TexturePack**:
  - Charge et stocke toutes les textures
  - `load()` - Chargement paresseux des textures
- **Classe Renderer**:
  - `draw_background()` - Arrière-plan avec gradients
  - `draw_stepper()` - Indicateur de progression des steps
  - `draw_map_selection()` - Rendu des previews de maps (rectangulaires)
  - `draw_difficulty_selection()` - Rendu des icônes de difficulté (circulaires)
  - `draw_gamemode_selection()` - Rendu des icônes de mode de jeu (circulaires)
  - `draw_circular_image()` - Utilitaire pour dessiner image + effets circulaires
- **Dépendances**: IGraphicsPlugin, protocol::Payloads

### **CreateRoomInitializer.hpp/cpp**
- **Rôle**: Initialise tous les éléments UI des différents steps
- **Fonctions principales**:
  - `init_room_info_step()` - Crée labels et text fields (nom de room, password)
  - `init_map_selection_step()` - Crée les boutons de sélection de map
  - `init_difficulty_step()` - Pas de boutons (images circulaires cliquables)
  - `init_game_mode_step()` - Pas de boutons (images circulaires cliquables)
  - `init_navigation_buttons()` - Crée Previous/Next/Create buttons
- **Dépendances**: UIButton, UILabel, UITextField

### **CreateRoomUpdater.hpp/cpp**
- **Rôle**: Gère la logique de mise à jour de chaque step
- **Fonctions principales**:
  - `update_room_info_step()` - Met à jour les text fields
  - `update_map_selection_step()` - Met à jour les boutons de map (sélection)
  - `update_difficulty_step()` - Délègue à InputHandler pour clics circulaires
  - `update_game_mode_step()` - Délègue à InputHandler pour clics circulaires
  - `update_navigation_buttons()` - Met à jour les boutons de navigation
  - `is_any_field_focused()` - Utilitaire pour vérifier le focus des text fields
- **Dépendances**: IGraphicsPlugin, IInputPlugin, CreateRoomInput

### **CreateRoomDrawer.hpp/cpp**
- **Rôle**: Orchestre le rendu de chaque step (haut niveau)
- **Fonctions principales**:
  - `draw_room_info_step()` - Dessine labels + text fields
  - `draw_map_selection_step()` - Dessine images de maps + boutons (via Renderer)
  - `draw_difficulty_step()` - Dessine images circulaires de difficulté (via Renderer)
  - `draw_game_mode_step()` - Dessine images circulaires de mode de jeu (via Renderer)
  - `draw_navigation_buttons()` - Dessine boutons avec texte dynamique
- **Dépendances**: Renderer, UIButton, UILabel, UITextField

## 🔄 Flux de données

### Initialisation
```
CreateRoomScreen::initialize()
  └─> Initializer::init_room_info_step()
  └─> Initializer::init_map_selection_step()
  └─> Initializer::init_difficulty_step()
  └─> Initializer::init_game_mode_step()
  └─> Initializer::init_navigation_buttons()
```

### Update
```
CreateRoomScreen::update()
  └─> Updater::is_any_field_focused()
  └─> Updater::update_*_step()
      └─> InputHandler::handle_*_click() (pour difficulty & gamemode)
  └─> Updater::update_navigation_buttons()
```

### Draw
```
CreateRoomScreen::draw()
  └─> TexturePack::load() (lazy loading)
  └─> Renderer::draw_background()
  └─> Renderer::draw_stepper()
  └─> Drawer::draw_*_step()
      └─> Renderer::draw_*_selection() (pour maps, difficulty, gamemode)
  └─> Drawer::draw_navigation_buttons()
```

## ✨ Avantages de cette architecture

1. **Séparation des responsabilités**: Chaque module a un rôle unique et bien défini
2. **Réutilisabilité**: Les renderers peuvent être réutilisés dans d'autres écrans
3. **Testabilité**: Chaque module peut être testé indépendamment
4. **Maintenabilité**: Modifications localisées dans des fichiers spécifiques
5. **Lisibilité**: Code court et focalisé (< 150 lignes par fichier)
6. **Extensibilité**: Facile d'ajouter de nouveaux steps ou fonctionnalités

## 📊 Comparaison

### Avant (monolithique)
- **1 fichier**: CreateRoomScreen.cpp (620+ lignes)
- Tout mélangé: init, update, draw, rendering
- Difficile à naviguer et maintenir

### Après (modulaire)
- **7 fichiers** bien organisés
- CreateRoomScreen.cpp: **275 lignes** (56% de réduction!)
- Chaque module: **50-150 lignes**
- Code clair, focalisé, facile à maintenir

## 🚀 Comment étendre

### Ajouter un nouveau step
1. Ajouter enum dans `CreateRoomScreen.hpp`
2. Créer `init_new_step()` dans `Initializer`
3. Créer `update_new_step()` dans `Updater`
4. Créer `draw_new_step()` dans `Drawer`
5. Mettre à jour les switch cases dans `CreateRoomScreen.cpp`

### Ajouter un nouvel élément UI
1. Ajouter constantes dans `Config.hpp`
2. Modifier `Initializer` pour créer l'élément
3. Modifier `Updater` pour la logique d'update
4. Modifier `Drawer` pour le rendu
