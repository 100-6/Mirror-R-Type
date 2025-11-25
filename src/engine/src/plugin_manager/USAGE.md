# Plugin Manager - Guide d'Utilisation

## Vue d'ensemble

Le **Plugin Manager** est une bibliothèque statique (`.a`) qui permet de charger dynamiquement des plugins (fichiers `.so`) dans votre jeu R-Type. Il offre une architecture modulaire où les composants graphiques, réseau, audio, et input peuvent être remplacés sans recompiler le jeu.

## Structure du projet

```
plugin_manager/
├── include/                      # Headers publics
│   ├── IPlugin.hpp              # Interface de base pour tous les plugins
│   ├── PluginManager.hpp        # Gestionnaire de plugins
│   ├── CommonTypes.hpp          # Types communs (Vector2f, Color, etc.)
│   ├── IGraphicsPlugin.hpp      # Interface pour plugins graphiques
│   ├── INetworkPlugin.hpp       # Interface pour plugins réseau
│   ├── IAudioPlugin.hpp         # Interface pour plugins audio
│   └── IInputPlugin.hpp         # Interface pour plugins input
├── src/
│   └── PluginManager.cpp        # Implémentation du gestionnaire
├── examples/
│   └── example_usage.cpp        # Exemple d'utilisation
├── tests/
│   ├── test_plugin.cpp          # Plugin de test
│   └── test_plugin_manager.cpp  # Tests unitaires
└── CMakeLists.txt
```

## Compilation

### Compiler la bibliothèque

```bash
cd build
cmake .. -DBUILD_PLUGIN_MANAGER_TESTS=ON
cmake --build . --target plugin_manager
```

Cela génère : `build/plugin_manager/libplugin_manager.a`

### Compiler les tests

```bash
cmake --build . --target test_plugin
cmake --build . --target test_plugin_manager
./test_plugin_manager
```

## Utilisation de base

### 1. Inclure les headers

```cpp
#include "PluginManager.hpp"
#include "IGraphicsPlugin.hpp"  // Pour les plugins graphiques
#include "INetworkPlugin.hpp"   // Pour les plugins réseau
// etc.

using namespace rtype;
```

### 2. Créer un gestionnaire

```cpp
PluginManager manager;
```

### 3. Charger un plugin

```cpp
try {
    auto* graphics = manager.load_plugin<IGraphicsPlugin>(
        "./plugins/libsfml_graphics.so",
        "create_graphics_plugin"  // Nom de la fonction factory
    );
    
    // Utiliser le plugin
    graphics->create_window(800, 600, "R-Type");
    
} catch (const PluginException& e) {
    std::cerr << "Erreur: " << e.what() << std::endl;
}
```

### 4. Utiliser le plugin

```cpp
// Le plugin est initialisé et prêt à l'emploi
while (graphics->is_window_open()) {
    graphics->clear(Color::Black);
    graphics->draw_text("Hello World", Vector2f{100, 100}, Color::White);
    graphics->display();
}
```

### 5. Décharger les plugins

```cpp
// Décharger un plugin spécifique
manager.unload_plugin("./plugins/libsfml_graphics.so");

// Ou décharger tous les plugins
manager.unload_all();

// Les plugins sont aussi automatiquement déchargés
// quand le PluginManager est détruit
```

## Créer un plugin

### Structure d'un plugin

Chaque plugin doit :
1. Hériter d'une interface (IPlugin, IGraphicsPlugin, etc.)
2. Implémenter toutes les méthodes virtuelles pures
3. Exporter une fonction factory `create_*_plugin()`

### Exemple : Plugin minimal

```cpp
#include "IGraphicsPlugin.hpp"
#include <SFML/Graphics.hpp>

class SFMLGraphicsPlugin : public IGraphicsPlugin {
private:
    sf::RenderWindow window_;
    bool initialized_ = false;

public:
    // Méthodes IPlugin
    const char* get_name() const override {
        return "SFML Graphics Plugin";
    }
    
    const char* get_version() const override {
        return "1.0.0";
    }
    
    bool initialize() override {
        initialized_ = true;
        return true;
    }
    
    void shutdown() override {
        window_.close();
        initialized_ = false;
    }
    
    bool is_initialized() const override {
        return initialized_;
    }
    
    // Méthodes IGraphicsPlugin
    bool create_window(int width, int height, const char* title) override {
        window_.create(sf::VideoMode(width, height), title);
        return window_.isOpen();
    }
    
    void clear(Color color) override {
        window_.clear(sf::Color(color.r, color.g, color.b, color.a));
    }
    
    void display() override {
        window_.display();
    }
    
    // ... autres méthodes ...
};

// Fonction factory exportée
extern "C" {
    IGraphicsPlugin* create_graphics_plugin() {
        return new SFMLGraphicsPlugin();
    }
    
    void destroy_graphics_plugin(IGraphicsPlugin* plugin) {
        delete plugin;
    }
}
```

### Compiler le plugin

```cmake
# CMakeLists.txt pour le plugin
add_library(sfml_graphics SHARED
    SFMLGraphicsPlugin.cpp
)

target_include_directories(sfml_graphics PRIVATE
    ${PLUGIN_MANAGER_INCLUDE_DIR}
)

target_link_libraries(sfml_graphics PRIVATE
    sfml-graphics
    sfml-window
)
```

## Interfaces disponibles

### IPlugin (Base)
- `get_name()` : Nom du plugin
- `get_version()` : Version du plugin
- `initialize()` : Initialiser le plugin
- `shutdown()` : Arrêter le plugin
- `is_initialized()` : Vérifier l'état

### IGraphicsPlugin
- Gestion de fenêtre : `create_window()`, `close_window()`, `is_window_open()`
- Rendu : `clear()`, `display()`, `draw_sprite()`, `draw_text()`, `draw_rectangle()`
- Ressources : `load_texture()`, `load_font()`
- Caméra : `set_view()`, `reset_view()`

### INetworkPlugin
- Serveur : `start_server()`, `stop_server()`, `broadcast()`
- Client : `connect()`, `disconnect()`, `send()`
- Callbacks : `set_on_client_connected()`, `set_on_packet_received()`
- Stats : `get_client_count()`, `get_client_ping()`

### IAudioPlugin
- Sons : `load_sound()`, `play_sound()`, `stop_sound()`
- Musique : `load_music()`, `play_music()`, `pause_music()`
- Volume : `set_master_volume()`, `set_music_volume()`, `set_muted()`

### IInputPlugin
- Clavier : `is_key_pressed()`, `is_key_just_pressed()`
- Souris : `is_mouse_button_pressed()`, `get_mouse_position()`
- Gamepad : `is_gamepad_connected()`, `get_gamepad_axis()`

## Exemple complet

Voir `plugin_manager/examples/example_usage.cpp` pour un exemple complet utilisant plusieurs types de plugins.

## Gestion des erreurs

Le PluginManager lance des exceptions `PluginException` en cas d'erreur :

```cpp
try {
    auto* plugin = manager.load_plugin<IGraphicsPlugin>("./plugin.so");
} catch (const PluginException& e) {
    // Erreurs possibles :
    // - Fichier .so introuvable
    // - Fonction factory introuvable
    // - Initialisation échouée
    // - Plugin déjà chargé
    std::cerr << "Erreur: " << e.what() << std::endl;
}
```

## Lier avec votre projet

Dans votre `CMakeLists.txt` :

```cmake
# Ajouter le sous-projet
add_subdirectory(plugin_manager)

# Lier avec votre exécutable
add_executable(rtype_client src/main.cpp)
target_link_libraries(rtype_client PRIVATE plugin_manager)
```

## Notes importantes

1. **Thread-safety** : Le PluginManager n'est PAS thread-safe. Utilisez-le depuis un seul thread ou ajoutez votre propre synchronisation.

2. **Ordre de déchargement** : Les plugins sont déchargés dans l'ordre inverse de leur chargement.

3. **Gestion mémoire** : Le PluginManager gère automatiquement la durée de vie des plugins. Ne pas appeler `delete` sur les pointeurs retournés.

4. **Compatibilité** : Assurez-vous que les plugins sont compilés avec le même compilateur et les mêmes flags que votre application.

5. **Portabilité** : Le code fonctionne sur Linux (dlopen) et Windows (LoadLibrary). Les chemins des plugins doivent être adaptés selon la plateforme (.so vs .dll).

## Prochaines étapes

Pour implémenter vos propres plugins :

1. **Graphics** : Créer un plugin SFML ou Raylib implémentant `IGraphicsPlugin`
2. **Network** : Créer un plugin Asio ou ENet implémentant `INetworkPlugin`
3. **Audio** : Créer un plugin SFML Audio implémentant `IAudioPlugin`
4. **Input** : Créer un plugin SFML ou SDL implémentant `IInputPlugin`

Ces plugins devront être créés dans le dossier `plugins/` avec leur propre `CMakeLists.txt`.
