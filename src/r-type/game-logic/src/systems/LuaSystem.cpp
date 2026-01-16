/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LuaSystem - Simplified version without wall detection
*/

#include "systems/LuaSystem.hpp"
#include "components/GameComponents.hpp"
#include "AssetsPaths.hpp"
#include "ecs/CoreComponents.hpp"
#include <filesystem>
#include <iostream>

LuaSystem::LuaSystem()
{
    lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::os);
}

void LuaSystem::init(Registry& registry)
{
    bindComponents(registry);
}

void LuaSystem::bindComponents(Registry& registry)
{
    std::cout << "[LuaSystem] Binding components to Lua..." << std::endl;
    registry.register_component<Script>();

    lua_.new_usertype<Position>("Position",
        "x", &Position::x,
        "y", &Position::y
    );

    lua_.new_usertype<Velocity>("Velocity",
        "x", &Velocity::x,
        "y", &Velocity::y
    );

    // Find nearest player function for AI scripts
    lua_.set_function("find_nearest_player", [&registry](float x, float y) -> std::tuple<float, float, bool> {
        float minDist = 9999999.0f;
        float targetX = 0;
        float targetY = 0;
        bool found = false;

        auto& positions = registry.get_components<Position>();
        auto& controllables = registry.get_components<Controllable>();

        for (size_t i = 0; i < controllables.size(); ++i) {
             Entity entity = controllables.get_entity_at(i);
             if (positions.has_entity(entity)) {
                 auto& pos = positions[entity];
                 float dist = (pos.x - x)*(pos.x - x) + (pos.y - y)*(pos.y - y);
                 if (dist < minDist) {
                     minDist = dist;
                     targetX = pos.x;
                     targetY = pos.y;
                     found = true;
                 }
             }
        }
        return std::make_tuple(targetX, targetY, found);
    });
}

void LuaSystem::update(Registry& registry, float dt)
{
    if (!registry.has_component_registered<Script>()) return;

    // Defensive: Ensure types are registered
    if (!lua_["Velocity"].valid()) {
        lua_.new_usertype<Position>("Position", "x", &Position::x, "y", &Position::y);
        lua_.new_usertype<Velocity>("Velocity", "x", &Velocity::x, "y", &Velocity::y);
    }

    auto& scripts = registry.get_components<Script>();
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();

    static std::unordered_map<std::string, sol::function> scriptCache;

    for (size_t i = 0; i < scripts.size(); ++i) {
        Entity entity = scripts.get_entity_at(i);

        if (!positions.has_entity(entity) || !velocities.has_entity(entity))
            continue;

        auto& script = scripts.get_data_at(i);
        auto& pos = positions[entity];
        auto& vel = velocities[entity];

        if (script.path.empty()) continue;

        // Load script if not cached
        if (scriptCache.find(script.path) == scriptCache.end()) {
            std::string fullPath = std::string(assets::paths::AI_SCRIPTS_BASE_PATH) + script.path;

            if (!std::filesystem::exists(fullPath)) {
                fullPath = script.path;
            }

            sol::load_result loadRes = lua_.load_file(fullPath);
            if (!loadRes.valid()) {
                sol::error err = loadRes;
                std::cerr << "Failed to load script: " << script.path << " Error: " << err.what() << std::endl;
                continue;
            }

            sol::protected_function scriptFunc = loadRes;
            sol::protected_function_result result = scriptFunc();
            if (!result.valid()) {
                sol::error err = result;
                std::cerr << "Failed to execute script body: " << script.path << " Error: " << err.what() << std::endl;
                continue;
            }

            if (result.get_type() == sol::type::function) {
                scriptCache[script.path] = result;
            } else {
                std::cerr << "Script must return a function: " << script.path << std::endl;
                continue;
            }
        }

        // Execute cached function
        if (scriptCache.find(script.path) != scriptCache.end()) {
            sol::function& func = scriptCache[script.path];
            sol::protected_function pfunc = func;

            // Create Lua tables for components
            sol::table posTable = lua_.create_table_with("x", pos.x, "y", pos.y);
            sol::table velTable = lua_.create_table_with("x", vel.x, "y", vel.y);

            auto result = pfunc(dt, posTable, velTable);

            if (!result.valid()) {
                sol::error err = result;
                std::cerr << "Error running script: " << script.path << " Error: " << err.what() << std::endl;
            } else {
                // Simply apply position and velocity from script
                pos.x = posTable.get_or("x", pos.x);
                pos.y = posTable.get_or("y", pos.y);
                vel.x = velTable.get_or("x", vel.x);
                vel.y = velTable.get_or("y", vel.y);
            }
        }
    }
}

void LuaSystem::shutdown()
{
}
