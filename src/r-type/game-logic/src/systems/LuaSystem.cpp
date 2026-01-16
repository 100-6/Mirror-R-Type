/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LuaSystem - Handles Lua scripting for AI and boss behavior
*/

#include "systems/LuaSystem.hpp"
#include "components/GameComponents.hpp"
#include "AssetsPaths.hpp"
#include "ecs/CoreComponents.hpp"
#include <filesystem>
#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LuaSystem::LuaSystem()
{
    lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::os);
}

void LuaSystem::init(Registry& registry)
{
    bindComponents(registry);
    bindBossFunctions(registry);
}

void LuaSystem::bindComponents(Registry& registry)
{
    std::cout << "[LuaSystem] Binding components to Lua..." << std::endl;
    registry.register_component<Script>();
    registry.register_component<BossScript>();

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

void LuaSystem::bindBossFunctions(Registry& registry)
{
    std::cout << "[LuaSystem] Binding boss functions to Lua..." << std::endl;

    // Spawn a single boss projectile
    lua_.set_function("spawn_boss_projectile", [&registry](float x, float y, float vx, float vy, int damage) {
        Entity proj = registry.spawn_entity();
        registry.add_component(proj, Position{x, y});
        registry.add_component(proj, Velocity{vx, vy});

        Projectile p;
        p.faction = ProjectileFaction::Enemy;
        p.lifetime = 10.0f;
        registry.add_component(proj, p);
        registry.add_component(proj, Damage{damage});
        registry.add_component(proj, Collider{10.0f, 10.0f});
        registry.add_component(proj, NoFriction{});
    });

    // Spawn 360-degree spray pattern
    lua_.set_function("spawn_pattern_360", [&registry](float x, float y, int count, float speed, int damage) {
        float angleStep = (2.0f * static_cast<float>(M_PI)) / static_cast<float>(count);
        for (int i = 0; i < count; ++i) {
            float angle = static_cast<float>(i) * angleStep;
            float vx = std::cos(angle) * speed;
            float vy = std::sin(angle) * speed;

            Entity proj = registry.spawn_entity();
            registry.add_component(proj, Position{x, y});
            registry.add_component(proj, Velocity{vx, vy});

            Projectile p;
            p.faction = ProjectileFaction::Enemy;
            p.lifetime = 8.0f;
            p.angle = angle;
            registry.add_component(proj, p);
            registry.add_component(proj, Damage{damage});
            registry.add_component(proj, Collider{10.0f, 10.0f});
            registry.add_component(proj, NoFriction{});
        }
    });

    // Spawn aimed burst pattern toward nearest player
    lua_.set_function("spawn_pattern_aimed", [&registry](float x, float y, int count, float speed, int damage, float spreadAngle) {
        // Find nearest player
        float targetX = x - 500.0f;  // Default: aim left
        float targetY = y;

        auto& positions = registry.get_components<Position>();
        auto& controllables = registry.get_components<Controllable>();
        float minDist = 9999999.0f;

        for (size_t i = 0; i < controllables.size(); ++i) {
            Entity entity = controllables.get_entity_at(i);
            if (positions.has_entity(entity)) {
                auto& pos = positions[entity];
                float dist = (pos.x - x)*(pos.x - x) + (pos.y - y)*(pos.y - y);
                if (dist < minDist) {
                    minDist = dist;
                    targetX = pos.x;
                    targetY = pos.y;
                }
            }
        }

        // Calculate base angle toward player
        float dx = targetX - x;
        float dy = targetY - y;
        float baseAngle = std::atan2(dy, dx);

        // Convert spread angle to radians
        float spreadRad = spreadAngle * static_cast<float>(M_PI) / 180.0f;
        float startAngle = baseAngle - spreadRad / 2.0f;
        float angleStep = (count > 1) ? spreadRad / static_cast<float>(count - 1) : 0.0f;

        for (int i = 0; i < count; ++i) {
            float angle = startAngle + static_cast<float>(i) * angleStep;
            float vx = std::cos(angle) * speed;
            float vy = std::sin(angle) * speed;

            Entity proj = registry.spawn_entity();
            registry.add_component(proj, Position{x, y});
            registry.add_component(proj, Velocity{vx, vy});

            Projectile p;
            p.faction = ProjectileFaction::Enemy;
            p.lifetime = 8.0f;
            p.angle = angle;
            registry.add_component(proj, p);
            registry.add_component(proj, Damage{damage});
            registry.add_component(proj, Collider{10.0f, 10.0f});
            registry.add_component(proj, NoFriction{});
        }
    });

    // Spawn spiral pattern with rotation offset
    lua_.set_function("spawn_pattern_spiral", [&registry](float x, float y, int count, float speed, int damage, float rotationOffset) {
        float rotationRad = rotationOffset * static_cast<float>(M_PI) / 180.0f;
        float angleStep = (2.0f * static_cast<float>(M_PI)) / static_cast<float>(count);

        for (int i = 0; i < count; ++i) {
            float angle = rotationRad + static_cast<float>(i) * angleStep;
            float vx = std::cos(angle) * speed;
            float vy = std::sin(angle) * speed;

            Entity proj = registry.spawn_entity();
            registry.add_component(proj, Position{x, y});
            registry.add_component(proj, Velocity{vx, vy});

            Projectile p;
            p.faction = ProjectileFaction::Enemy;
            p.lifetime = 8.0f;
            p.angle = angle;
            registry.add_component(proj, p);
            registry.add_component(proj, Damage{damage});
            registry.add_component(proj, Collider{10.0f, 10.0f});
            registry.add_component(proj, NoFriction{});
        }
    });

    // Spawn random barrage pattern
    lua_.set_function("spawn_pattern_random", [&registry](float x, float y, int count, float speed, int damage) {
        for (int i = 0; i < count; ++i) {
            // Random angle between 0 and 2*PI
            float angle = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f * static_cast<float>(M_PI);
            // Slight speed variation
            float speedVar = speed * (0.8f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.4f);

            float vx = std::cos(angle) * speedVar;
            float vy = std::sin(angle) * speedVar;

            Entity proj = registry.spawn_entity();
            registry.add_component(proj, Position{x, y});
            registry.add_component(proj, Velocity{vx, vy});

            Projectile p;
            p.faction = ProjectileFaction::Enemy;
            p.lifetime = 8.0f;
            p.angle = angle;
            registry.add_component(proj, p);
            registry.add_component(proj, Damage{damage});
            registry.add_component(proj, Collider{10.0f, 10.0f});
            registry.add_component(proj, NoFriction{});
        }
    });
}

void LuaSystem::update(Registry& registry, float dt)
{
    // Defensive: Ensure types are registered
    if (!lua_["Velocity"].valid()) {
        lua_.new_usertype<Position>("Position", "x", &Position::x, "y", &Position::y);
        lua_.new_usertype<Velocity>("Velocity", "x", &Velocity::x, "y", &Velocity::y);
    }

    // Update regular enemy scripts
    updateEnemyScripts(registry, dt);

    // Update boss scripts
    updateBossScripts(registry, dt);
}

void LuaSystem::updateEnemyScripts(Registry& registry, float dt)
{
    if (!registry.has_component_registered<Script>()) return;

    auto& scripts = registry.get_components<Script>();
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();

    for (size_t i = 0; i < scripts.size(); ++i) {
        Entity entity = scripts.get_entity_at(i);

        if (!positions.has_entity(entity) || !velocities.has_entity(entity))
            continue;

        auto& script = scripts.get_data_at(i);
        auto& pos = positions[entity];
        auto& vel = velocities[entity];

        if (script.path.empty()) continue;

        // Load script if not cached
        if (scriptCache_.find(script.path) == scriptCache_.end()) {
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
                scriptCache_[script.path] = result;
            } else {
                std::cerr << "Script must return a function: " << script.path << std::endl;
                continue;
            }
        }

        // Execute cached function
        if (scriptCache_.find(script.path) != scriptCache_.end()) {
            sol::function& func = scriptCache_[script.path];
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

void LuaSystem::updateBossScripts(Registry& registry, float dt)
{
    if (!registry.has_component_registered<BossScript>()) return;
    if (!registry.has_component_registered<Health>()) return;

    auto& bossScripts = registry.get_components<BossScript>();
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& healths = registry.get_components<Health>();

    for (size_t i = 0; i < bossScripts.size(); ++i) {
        Entity entity = bossScripts.get_entity_at(i);

        if (!positions.has_entity(entity) || !velocities.has_entity(entity) || !healths.has_entity(entity))
            continue;

        auto& bossScript = bossScripts.get_data_at(i);
        auto& pos = positions[entity];
        auto& vel = velocities[entity];
        auto& health = healths[entity];

        if (bossScript.path.empty()) continue;

        // Update timers
        bossScript.phase_timer += dt;
        bossScript.attack_timer += dt;

        // Calculate health percentage and determine phase
        float healthPercent = static_cast<float>(health.current) / static_cast<float>(health.max);
        int newPhase = 0;
        if (healthPercent <= 0.33f) {
            newPhase = 2;
        } else if (healthPercent <= 0.66f) {
            newPhase = 1;
        }

        // Check for phase transition
        if (newPhase != bossScript.current_phase) {
            std::cout << "[LuaSystem] Boss phase transition: " << bossScript.current_phase << " -> " << newPhase << std::endl;
            bossScript.current_phase = newPhase;
            bossScript.phase_timer = 0.0f;  // Reset phase timer on transition
        }

        // Load script if not cached
        if (bossScriptCache_.find(bossScript.path) == bossScriptCache_.end()) {
            std::string fullPath = std::string(assets::paths::AI_SCRIPTS_BASE_PATH) + bossScript.path;

            if (!std::filesystem::exists(fullPath)) {
                fullPath = bossScript.path;
            }

            sol::load_result loadRes = lua_.load_file(fullPath);
            if (!loadRes.valid()) {
                sol::error err = loadRes;
                std::cerr << "[LuaSystem] Failed to load boss script: " << bossScript.path << " Error: " << err.what() << std::endl;
                continue;
            }

            sol::protected_function scriptFunc = loadRes;
            sol::protected_function_result result = scriptFunc();
            if (!result.valid()) {
                sol::error err = result;
                std::cerr << "[LuaSystem] Failed to execute boss script body: " << bossScript.path << " Error: " << err.what() << std::endl;
                continue;
            }

            if (result.get_type() == sol::type::function) {
                bossScriptCache_[bossScript.path] = result;
                std::cout << "[LuaSystem] Loaded boss script: " << bossScript.path << std::endl;
            } else {
                std::cerr << "[LuaSystem] Boss script must return a function: " << bossScript.path << std::endl;
                continue;
            }
        }

        // Execute cached function
        if (bossScriptCache_.find(bossScript.path) != bossScriptCache_.end()) {
            sol::function& func = bossScriptCache_[bossScript.path];
            sol::protected_function pfunc = func;

            // Create Lua tables for components
            sol::table posTable = lua_.create_table_with("x", pos.x, "y", pos.y);
            sol::table velTable = lua_.create_table_with("x", vel.x, "y", vel.y);

            // Create boss state table
            sol::table bossState = lua_.create_table_with(
                "phase", bossScript.current_phase,
                "phase_timer", bossScript.phase_timer,
                "attack_timer", bossScript.attack_timer,
                "health_percent", healthPercent
            );

            auto result = pfunc(dt, posTable, velTable, bossState);

            if (!result.valid()) {
                sol::error err = result;
                std::cerr << "[LuaSystem] Error running boss script: " << bossScript.path << " Error: " << err.what() << std::endl;
            } else {
                // Apply position and velocity from script
                pos.x = posTable.get_or("x", pos.x);
                pos.y = posTable.get_or("y", pos.y);
                vel.x = velTable.get_or("x", vel.x);
                vel.y = velTable.get_or("y", vel.y);

                // Check if attack was performed (script returns true)
                bool attackPerformed = false;
                if (result.get_type() == sol::type::boolean) {
                    attackPerformed = result.get<bool>();
                }

                if (attackPerformed) {
                    bossScript.attack_timer = 0.0f;  // Reset attack cooldown
                }
            }
        }
    }
}

void LuaSystem::loadScript(const std::string& path)
{
    // This method is kept for compatibility but not currently used
    (void)path;
}

void LuaSystem::shutdown()
{
    scriptCache_.clear();
    bossScriptCache_.clear();
}
