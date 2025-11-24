
#pragma once
#include <string>
#include <memory>

namespace RType {

class IEnemyPlugin {
public:
    virtual ~IEnemyPlugin() = default;

    virtual std::string getName() const = 0;
    virtual std::string getType() const = 0;

    virtual void spawn(int x, int y) = 0;
    virtual void update(float deltaTime) = 0;
    virtual int getDamage() const = 0;
};

extern "C" {
    typedef IEnemyPlugin* (*CreatePluginFunc)();
    typedef void (*DestroyPluginFunc)(IEnemyPlugin*);
}

} // namespace RType
