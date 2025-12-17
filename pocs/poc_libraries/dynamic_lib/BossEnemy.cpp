
#include "IEnemyPlugin.hpp"
#include <iostream>

namespace RType {

class BossEnemy : public IEnemyPlugin {
public:
    BossEnemy() {
        std::cout << "[BossEnemy Plugin] Constructor - Loaded from .so at runtime\n";
    }

    ~BossEnemy() override {
        std::cout << "[BossEnemy Plugin] Destructor\n";
    }

    std::string getName() const override {
        return "Boss Enemy";
    }

    std::string getType() const override {
        return "DYNAMIC PLUGIN (.so)";
    }

    void spawn(int x, int y) override {
        x_ = x;
        y_ = y;
        health_ = 1000;
        std::cout << "[BossEnemy] BOSS spawned at (" << x << ", " << y << ")\n";
        std::cout << "  → Health: " << health_ << "\n";
        std::cout << "  → Plugin can be modified without touching core game\n";
    }

    void update(float deltaTime) override {
        x_ -= static_cast<int>(20 * deltaTime);
    }

    int getDamage() const override {
        return 50; // Boss does more damage
    }

private:
    int x_ = 0;
    int y_ = 0;
    int health_ = 1000;
};

} // namespace RType

extern "C" {
    RType::IEnemyPlugin* createPlugin() {
        return new RType::BossEnemy();
    }

    void destroyPlugin(RType::IEnemyPlugin* plugin) {
        delete plugin;
    }
}
