# Guide : Créer un Plugin

Ce guide explique comment créer un plugin pour le système de plugin manager de R-Type.

## Structure d'un Plugin

Un plugin est une bibliothèque partagée (`.so` sur Linux, `.dll` sur Windows) qui :

1. **Implémente** une interface héritant de `IPlugin`
2. **Exporte** des fonctions factory C pour créer/détruire le plugin
3. **Compile** en bibliothèque partagée (SHARED)

## Étape 1 : Choisir l'interface

Selon le type de plugin que vous créez :

- **Graphics** → `IGraphicsPlugin`
- **Network** → `INetworkPlugin`
- **Audio** → `IAudioPlugin`
- **Input** → `IInputPlugin`
- **Custom** → Hériter directement de `IPlugin`

## Étape 2 : Implémenter le Plugin

### Exemple : Plugin Graphique SFML

```cpp
// SFMLGraphicsPlugin.hpp
#pragma once
#include "IGraphicsPlugin.hpp"
#include <SFML/Graphics.hpp>
#include <unordered_map>

namespace rtype {

class SFMLGraphicsPlugin : public IGraphicsPlugin {
private:
    sf::RenderWindow window_;
    std::unordered_map<TextureHandle, sf::Texture> textures_;
    std::unordered_map<FontHandle, sf::Font> fonts_;
    bool initialized_ = false;
    TextureHandle next_texture_handle_ = 1;
    FontHandle next_font_handle_ = 1;

public:
    // === IPlugin Methods ===
    
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
        if (window_.isOpen()) {
            window_.close();
        }
        textures_.clear();
        fonts_.clear();
        initialized_ = false;
    }
    
    bool is_initialized() const override {
        return initialized_;
    }
    
    // === IGraphicsPlugin Methods ===
    
    bool create_window(int width, int height, const char* title) override {
        window_.create(sf::VideoMode(width, height), title);
        return window_.isOpen();
    }
    
    void close_window() override {
        window_.close();
    }
    
    bool is_window_open() const override {
        return window_.isOpen();
    }
    
    void set_fullscreen(bool fullscreen) override {
        if (fullscreen) {
            window_.create(sf::VideoMode::getDesktopMode(), "R-Type", 
                          sf::Style::Fullscreen);
        } else {
            window_.create(sf::VideoMode(800, 600), "R-Type", 
                          sf::Style::Default);
        }
    }
    
    void set_vsync(bool enabled) override {
        window_.setVerticalSyncEnabled(enabled);
    }
    
    void clear(Color color) override {
        window_.clear(sf::Color(color.r, color.g, color.b, color.a));
    }
    
    void display() override {
        window_.display();
    }
    
    void draw_sprite(const Sprite& sprite, Vector2f position) override {
        auto it = textures_.find(sprite.texture_handle);
        if (it == textures_.end()) return;
        
        sf::Sprite sf_sprite(it->second);
        sf_sprite.setPosition(position.x, position.y);
        sf_sprite.setRotation(sprite.rotation);
        sf_sprite.setOrigin(sprite.origin.x, sprite.origin.y);
        sf_sprite.setScale(sprite.size.x / it->second.getSize().x,
                          sprite.size.y / it->second.getSize().y);
        sf_sprite.setColor(sf::Color(sprite.tint.r, sprite.tint.g, 
                                     sprite.tint.b, sprite.tint.a));
        
        window_.draw(sf_sprite);
    }
    
    void draw_text(const std::string& text, Vector2f position, Color color,
                   FontHandle font_handle, int font_size) override {
        sf::Text sf_text;
        sf_text.setString(text);
        sf_text.setPosition(position.x, position.y);
        sf_text.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
        sf_text.setCharacterSize(font_size);
        
        if (font_handle != INVALID_HANDLE) {
            auto it = fonts_.find(font_handle);
            if (it != fonts_.end()) {
                sf_text.setFont(it->second);
            }
        }
        
        window_.draw(sf_text);
    }
    
    void draw_rectangle(const Rectangle& rect, Color color) override {
        sf::RectangleShape shape(sf::Vector2f(rect.width, rect.height));
        shape.setPosition(rect.x, rect.y);
        shape.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
        window_.draw(shape);
    }
    
    void draw_rectangle_outline(const Rectangle& rect, Color color, 
                                float thickness) override {
        sf::RectangleShape shape(sf::Vector2f(rect.width, rect.height));
        shape.setPosition(rect.x, rect.y);
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(sf::Color(color.r, color.g, color.b, color.a));
        shape.setOutlineThickness(thickness);
        window_.draw(shape);
    }
    
    void draw_circle(Vector2f center, float radius, Color color) override {
        sf::CircleShape shape(radius);
        shape.setPosition(center.x - radius, center.y - radius);
        shape.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
        window_.draw(shape);
    }
    
    void draw_line(Vector2f start, Vector2f end, Color color, 
                   float thickness) override {
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(start.x, start.y), 
                      sf::Color(color.r, color.g, color.b, color.a)),
            sf::Vertex(sf::Vector2f(end.x, end.y), 
                      sf::Color(color.r, color.g, color.b, color.a))
        };
        window_.draw(line, 2, sf::Lines);
    }
    
    TextureHandle load_texture(const std::string& path) override {
        sf::Texture texture;
        if (!texture.loadFromFile(path)) {
            return INVALID_HANDLE;
        }
        
        TextureHandle handle = next_texture_handle_++;
        textures_[handle] = std::move(texture);
        return handle;
    }
    
    void unload_texture(TextureHandle handle) override {
        textures_.erase(handle);
    }
    
    Vector2f get_texture_size(TextureHandle handle) const override {
        auto it = textures_.find(handle);
        if (it == textures_.end()) {
            return Vector2f{0.0f, 0.0f};
        }
        auto size = it->second.getSize();
        return Vector2f{static_cast<float>(size.x), 
                       static_cast<float>(size.y)};
    }
    
    FontHandle load_font(const std::string& path) override {
        sf::Font font;
        if (!font.loadFromFile(path)) {
            return INVALID_HANDLE;
        }
        
        FontHandle handle = next_font_handle_++;
        fonts_[handle] = std::move(font);
        return handle;
    }
    
    void unload_font(FontHandle handle) override {
        fonts_.erase(handle);
    }
    
    void set_view(Vector2f center, Vector2f size) override {
        sf::View view(sf::FloatRect(center.x - size.x / 2, 
                                    center.y - size.y / 2,
                                    size.x, size.y));
        window_.setView(view);
    }
    
    void reset_view() override {
        window_.setView(window_.getDefaultView());
    }
};

} // namespace rtype
```

## Étape 3 : Exporter les Fonctions Factory

```cpp
// À la fin du fichier .cpp
extern "C" {
    rtype::IGraphicsPlugin* create_graphics_plugin() {
        return new rtype::SFMLGraphicsPlugin();
    }
    
    void destroy_graphics_plugin(rtype::IGraphicsPlugin* plugin) {
        delete plugin;
    }
}
```

**Important** : Le nom des fonctions doit correspondre au pattern :
- `create_<type>_plugin()` → Crée une instance
- `destroy_<type>_plugin()` → Détruit l'instance (optionnel)

## Étape 4 : CMakeLists.txt

```cmake
# plugins/graphics/sfml/CMakeLists.txt
cmake_minimum_required(VERSION 3.20)

# Trouver SFML
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Créer la bibliothèque partagée
add_library(sfml_graphics SHARED
    SFMLGraphicsPlugin.cpp
)

# Inclure les headers du plugin manager
target_include_directories(sfml_graphics PRIVATE
    ${CMAKE_SOURCE_DIR}/plugin_manager/include
)

# Lier avec SFML
target_link_libraries(sfml_graphics PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
)

# Compiler en C++20
target_compile_features(sfml_graphics PRIVATE cxx_std_20)

# Retirer le préfixe 'lib' si nécessaire (optionnel)
set_target_properties(sfml_graphics PROPERTIES PREFIX "")
```

## Étape 5 : Compiler

```bash
mkdir build && cd build
cmake ..
cmake --build . --target sfml_graphics
```

Résultat : `plugins/graphics/sfml/sfml_graphics.so`

## Étape 6 : Utiliser le Plugin

```cpp
#include "PluginManager.hpp"
#include "IGraphicsPlugin.hpp"

int main() {
    rtype::PluginManager manager;
    
    auto* graphics = manager.load_plugin<rtype::IGraphicsPlugin>(
        "./plugins/graphics/sfml/sfml_graphics.so",
        "create_graphics_plugin"
    );
    
    graphics->create_window(800, 600, "R-Type");
    
    while (graphics->is_window_open()) {
        graphics->clear(rtype::Color::Black);
        // ... votre logique de jeu ...
        graphics->display();
    }
    
    return 0;
}
```

## Bonnes Pratiques

### 1. Gestion des Erreurs

```cpp
bool initialize() override {
    try {
        // Votre initialisation
        initialized_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Init failed: " << e.what() << std::endl;
        return false;
    }
}
```

### 2. État d'Initialisation

```cpp
void some_method() override {
    if (!initialized_) {
        throw std::runtime_error("Plugin not initialized");
    }
    // ... votre code ...
}
```

### 3. Cleanup Complet

```cpp
void shutdown() override {
    // Libérer TOUTES les ressources
    window_.close();
    textures_.clear();
    fonts_.clear();
    sounds_.clear();
    initialized_ = false;
}
```

### 4. Thread-Safety (si nécessaire)

```cpp
class ThreadSafePlugin : public INetworkPlugin {
private:
    mutable std::mutex mutex_;
    
public:
    bool send(const NetworkPacket& packet) override {
        std::lock_guard<std::mutex> lock(mutex_);
        // ... votre code thread-safe ...
    }
};
```

## Checklist de Création de Plugin

- [ ] Hérite de l'interface appropriée (`IGraphicsPlugin`, etc.)
- [ ] Implémente toutes les méthodes virtuelles pures
- [ ] Exporte les fonctions factory C (`extern "C"`)
- [ ] Gère correctement `initialize()` et `shutdown()`
- [ ] Retourne des valeurs appropriées (handles, booléens, etc.)
- [ ] Compile en bibliothèque SHARED (`.so`)
- [ ] Teste le chargement avec `PluginManager`
- [ ] Vérifie les fuites mémoire (valgrind)

## Templates Disponibles

Voir dans `plugin_manager/tests/test_plugin.cpp` pour un exemple minimal.

## Dépannage

### Plugin ne se charge pas
- Vérifier que le fichier `.so` existe
- Vérifier les permissions (chmod +x)
- Vérifier les dépendances (ldd plugin.so)
- Vérifier les symboles exportés (nm -D plugin.so)

### Fonction factory introuvable
- Vérifier `extern "C"` autour des fonctions
- Vérifier le nom exact (underscore, casse)
- Utiliser `nm -D plugin.so | grep create` pour lister

### Crash au runtime
- Vérifier la compatibilité ABI (même compilateur)
- Vérifier que `initialize()` retourne true
- Vérifier les pointeurs nuls
- Utiliser gdb pour débugger

---

**Vous êtes maintenant prêt à créer vos propres plugins !** 🚀
