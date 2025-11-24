
#include "IEnemyPlugin.hpp"
#include <iostream>

namespace RType {

class BasicEnemy : public IEnemyPlugin {
public:
    BasicEnemy() {
        std::cout << "[BasicEnemy Plugin] Constructor - Loaded from .so at runtime\n";
    }

    ~BasicEnemy() override {
        std::cout << "[BasicEnemy Plugin] Destructor\n";
    }

    std::string getName() const override {
        return "Basic Enemy";
    }

    std::string getType() const override {
        return "DYNAMIC PLUGIN (.so)";
    }

    void spawn(int x, int y) override {
        x_ = x;
        y_ = y;
        std::cout << "[BasicEnemy] Spawned at (" << x << ", " << y << ")\n";
        std::cout << "  → Loaded dynamically with dlopen()\n";
        std::cout << "  → Can be updated without recompiling main program\n";
        std::cout << "  → Modular and extensible\n";
    }

    void update(float deltaTime) override {
        x_ -= static_cast<int>(50 * deltaTime); // Move left
    }

    int getDamage() const override {
        return 10;
    }

private:
    int x_ = 0;
    int y_ = 0;
};

} // namespace RType

extern "C" {
    RType::IEnemyPlugin* createPlugin() {
        return new RType::BasicEnemy();
    }

    void destroyPlugin(RType::IEnemyPlugin* plugin) {
        delete plugin;
    }
}
