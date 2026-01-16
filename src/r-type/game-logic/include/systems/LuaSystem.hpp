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

class LuaSystem : public ISystem {
    public:
        LuaSystem();
        ~LuaSystem() override = default;

        void init(Registry& registry) override;
        void update(Registry& registry, float dt) override;
        void shutdown() override;

    private:
        sol::state lua_;

        void bindComponents(Registry& registry);
        void loadScript(const std::string& path);
};

#endif /* !LUASYSTEM_HPP_ */
