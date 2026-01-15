/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ClientHitFlashSystem - Client-side hit flash effect
*/

#ifndef CLIENTHITFLASHSYSTEM_HPP_
#define CLIENTHITFLASHSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include <unordered_map>

namespace rtype::client {

/**
 * @brief Client-side system that handles white flash effects when entities take damage
 * Tracks HP changes and applies visual flash effect
 */
class ClientHitFlashSystem : public ISystem {
public:
    ClientHitFlashSystem() = default;
    ~ClientHitFlashSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

private:
    // Track previous HP values to detect damage
    std::unordered_map<Entity, int> previous_hp_;
};

} // namespace rtype::client

#endif /* !CLIENTHITFLASHSYSTEM_HPP_ */
