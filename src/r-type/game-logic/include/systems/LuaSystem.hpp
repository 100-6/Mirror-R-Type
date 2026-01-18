/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LuaSystem
*/

#ifndef LUASYSTEM_HPP_
#define LUASYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "sol/sol.hpp"
#include <unordered_map>
#include <string>

class LuaSystem : public ISystem {
    public:
        LuaSystem();
        ~LuaSystem() override = default;

        void init(Registry& registry) override;
        void update(Registry& registry, float dt) override;
        void shutdown() override;

    private:
        sol::state lua_;
        std::unordered_map<std::string, sol::function> scriptCache_;
        std::unordered_map<std::string, sol::function> bossScriptCache_;

        void bindComponents(Registry& registry);
        void bindBossFunctions(Registry& registry);
        void loadScript(const std::string& path);
        void updateEnemyScripts(Registry& registry, float dt);
        void updateBossScripts(Registry& registry, float dt);
};

#endif /* !LUASYSTEM_HPP_ */
