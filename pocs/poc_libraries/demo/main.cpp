#include "../static_lib/CoreEngine.hpp"
#include "../dynamic_lib/IEnemyPlugin.hpp"

#include <iostream>
#include <dlfcn.h>
#include <vector>
#include <memory>

class PluginManager {
public:
    ~PluginManager() {
        unloadAll();
    }

    bool loadPlugin(const std::string& path) {
        void* handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "Failed to load plugin: " << dlerror() << "\n";
            return false;
        }

        auto createFunc = (RType::CreatePluginFunc)dlsym(handle, "createPlugin");
        auto destroyFunc = (RType::DestroyPluginFunc)dlsym(handle, "destroyPlugin");

        if (!createFunc || !destroyFunc) {
            std::cerr << "Failed to find plugin functions: " << dlerror() << "\n";
            dlclose(handle);
            return false;
        }

        RType::IEnemyPlugin* plugin = createFunc();
        plugins_.push_back({handle, plugin, destroyFunc});

        std::cout << "✓ Loaded plugin: " << plugin->getName() << " (" << path << ")\n";
        return true;
    }

    const std::vector<RType::IEnemyPlugin*> getPlugins() const {
        std::vector<RType::IEnemyPlugin*> result;
        for (const auto& p : plugins_) {
            result.push_back(p.plugin);
        }
        return result;
    }

    void unloadAll() {
        for (auto& p : plugins_) {
            p.destroyFunc(p.plugin);
            dlclose(p.handle);
        }
        plugins_.clear();
    }

private:
    struct LoadedPlugin {
        void* handle;
        RType::IEnemyPlugin* plugin;
        RType::DestroyPluginFunc destroyFunc;
    };

    std::vector<LoadedPlugin> plugins_;
};

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║        R-Type: Static vs Dynamic Libraries POC            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    printSeparator("PARTIE 1: Static Library (.a) - Core Engine");

    std::cout << "→ Le CoreEngine est linké statiquement\n";
    std::cout << "→ Code intégré dans l'exécutable au moment de la compilation\n\n";

    RType::CoreEngine engine;
    engine.initialize();

    std::cout << "\n[Demo] Creating game entities...\n";
    auto player = engine.createEntity("Player Ship");
    auto bullet1 = engine.createEntity("Bullet 1");
    auto bullet2 = engine.createEntity("Bullet 2");

    std::cout << "\n[Demo] Current entities:\n";
    for (const auto& entity : engine.getEntities()) {
        std::cout << "  - #" << entity->getId() << ": " << entity->getName() << "\n";
    }

    std::cout << "\n✓ AVANTAGES .a (static):\n";
    std::cout << "  • Pas de dépendances externes à runtime\n";
    std::cout << "  • Performance optimale (pas d'indirection)\n";
    std::cout << "  • Distribution simple (un seul fichier)\n";
    std::cout << "  • Parfait pour le code CORE utilisé par client ET serveur\n";

    printSeparator("PARTIE 2: Dynamic Libraries (.so) - Plugin System");

    std::cout << "→ Les plugins d'ennemis sont chargés dynamiquement\n";
    std::cout << "→ Fichiers .so chargés avec dlopen() à runtime\n\n";

    PluginManager pluginMgr;

    std::cout << "[Demo] Loading enemy plugins...\n";
    bool basicLoaded = pluginMgr.loadPlugin("./libbasic_enemy.so");
    bool bossLoaded = pluginMgr.loadPlugin("./libboss_enemy.so");

    if (!basicLoaded || !bossLoaded) {
        std::cerr << "\n⚠ Warning: Some plugins failed to load\n";
        std::cerr << "Make sure to build the project first: cmake --build build\n";
    } else {
        std::cout << "\n[Demo] Spawning enemies from plugins...\n";
        auto plugins = pluginMgr.getPlugins();

        for (auto* plugin : plugins) {
            std::cout << "\n--- " << plugin->getName() << " ---\n";
            plugin->spawn(800, 300);
            std::cout << "Damage: " << plugin->getDamage() << "\n";
        }

        std::cout << "\n✓ AVANTAGES .so (dynamic):\n";
        std::cout << "  • Modification sans recompiler le programme principal\n";
        std::cout << "  • Système de mods/plugins extensible\n";
        std::cout << "  • Chargement à la demande (économie mémoire)\n";
        std::cout << "  • Parfait pour du contenu modulaire (ennemis, armes, niveaux)\n";
    }

    printSeparator("COMPARAISON & RECOMMANDATIONS");

    std::cout << "┌────────────────────┬─────────────────┬─────────────────┐\n";
    std::cout << "│ Critère            │ Static (.a)     │ Dynamic (.so)   │\n";
    std::cout << "├────────────────────┼─────────────────┼─────────────────┤\n";
    std::cout << "│ Linking Time       │ Compile-time    │ Runtime         │\n";
    std::cout << "│ Performance        │ Excellent       │ Très bon        │\n";
    std::cout << "│ Distribution       │ Facile          │ Multiple files  │\n";
    std::cout << "│ Updates            │ Recompile all   │ Replace .so     │\n";
    std::cout << "│ Modularity         │ Faible          │ Excellente      │\n";
    std::cout << "│ Taille exe         │ Plus gros       │ Plus petit      │\n";
    std::cout << "└────────────────────┴─────────────────┴─────────────────┘\n";

    std::cout << "\n📋 ARCHITECTURE RECOMMANDÉE pour R-Type:\n\n";
    std::cout << "STATIC (.a):\n";
    std::cout << "  • rtype_engine    → ECS, physique, réseau, rendering core\n";
    std::cout << "  • rtype_protocol  → Protocole UDP/TCP partagé\n";
    std::cout << "  • rtype_common    → Types et utils communs\n";

    std::cout << "\nDYNAMIC (.so) - OPTIONNEL:\n";
    std::cout << "  • enemy_*.so      → Différents types d'ennemis\n";
    std::cout << "  • weapon_*.so     → Système d'armes modulaire\n";
    std::cout << "  • level_*.so      → Niveaux/stages du jeu\n";
    std::cout << "  • mod_*.so        → Support de mods communautaires\n";

    std::cout << "\n💡 Pour ce projet, commencez avec tout en STATIC,\n";
    std::cout << "   puis ajoutez des plugins .so si besoin de modularité.\n";

    engine.shutdown();

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                       POC terminé                          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    return 0;
}
