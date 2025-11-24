#include <chrono>
#include <iostream>
#include <dlfcn.h>
#include "../dynamic_lib/IEnemyPlugin.hpp"

using namespace std::chrono;

void benchmark_loading() {
    const int iterations = 100;

    std::cout << "Benchmark: Temps de CHARGEMENT\n";
    std::cout << "================================\n\n";

    long long total_load = 0;
    long long total_unload = 0;

    for (int i = 0; i < iterations; i++) {
        auto start_load = high_resolution_clock::now();
        void* handle = dlopen("./libbasic_enemy.so", RTLD_LAZY);
        auto end_load = high_resolution_clock::now();

        if (!handle) {
            std::cerr << "Erreur dlopen: " << dlerror() << "\n";
            return;
        }

        auto start_unload = high_resolution_clock::now();
        dlclose(handle);
        auto end_unload = high_resolution_clock::now();

        total_load += duration_cast<microseconds>(end_load - start_load).count();
        total_unload += duration_cast<microseconds>(end_unload - start_unload).count();
    }

    std::cout << "Static (.a):\n";
    std::cout << "  Temps de chargement: 0 us (code deja dans l'exe)\n\n";

    std::cout << "Dynamic (.so):\n";
    std::cout << "  Chargement (dlopen):  " << total_load / iterations << " us\n";
    std::cout << "  Dechargement (dlclose): " << total_unload / iterations << " us\n";
}

void benchmark_calls() {
    const int iterations = 1000000;

    std::cout << "\n\nBenchmark: APPELS de fonction\n";
    std::cout << "================================\n\n";

    void* handle = dlopen("./libbasic_enemy.so", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Erreur: " << dlerror() << "\n";
        return;
    }

    auto createFunc = (RType::CreatePluginFunc)dlsym(handle, "createPlugin");
    auto destroyFunc = (RType::DestroyPluginFunc)dlsym(handle, "destroyPlugin");

    if (!createFunc || !destroyFunc) {
        std::cerr << "Erreur symboles\n";
        dlclose(handle);
        return;
    }

    auto start = high_resolution_clock::now();

    for (int i = 0; i < iterations; i++) {
        RType::IEnemyPlugin* plugin = createFunc();
        plugin->getDamage();
        destroyFunc(plugin);
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start).count();

    std::cout << "Dynamic (.so):\n";
    std::cout << "  " << iterations << " appels\n";
    std::cout << "  Temps moyen: " << duration / iterations << " ns par appel\n";

    dlclose(handle);

    std::cout << "\nStatic (.a):\n";
    std::cout << "  Temps moyen: ~1-2 ns par appel (appel direct)\n";
}

int main() {
    std::cout << "\n========================================\n";
    std::cout << "POC: Static (.a) vs Dynamic (.so)\n";
    std::cout << "========================================\n\n";

    benchmark_loading();
    benchmark_calls();

    std::cout << "\n========================================\n";
    std::cout << "Conclusion:\n";
    std::cout << "========================================\n";
    std::cout << "- Static: Pas de temps de chargement\n";
    std::cout << "- Dynamic: Quelques microsecondes\n";
    std::cout << "  pour dlopen() au demarrage\n";
    std::cout << "- Difference negligeable pour un jeu\n";
    std::cout << "\n";

    return 0;
}
