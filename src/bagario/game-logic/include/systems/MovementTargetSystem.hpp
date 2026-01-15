#pragma once

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "../components/BagarioComponents.hpp"
#include "BagarioConfig.hpp"
#include <cmath>
#include <vector>

namespace bagario::systems {

/**
 * @brief System that moves cells towards their target (mouse position)
 *
 * Responsibilities:
 * - Calculate velocity based on target position
 * - Apply speed based on mass (bigger = slower)
 * - Handle split velocity decay
 */
class MovementTargetSystem : public ISystem {
public:
    MovementTargetSystem() = default;
    ~MovementTargetSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;
};

}
