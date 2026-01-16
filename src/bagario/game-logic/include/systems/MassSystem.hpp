#pragma once

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "../components/BagarioComponents.hpp"
#include "BagarioConfig.hpp"

namespace bagario::systems {

/**
 * @brief System that handles mass-related mechanics
 *
 * Responsibilities:
 * - Update CircleCollider radius based on mass
 * - Apply mass decay for large cells
 * - Update cell speed based on mass
 */
class MassSystem : public ISystem {
public:
    MassSystem() = default;
    ~MassSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;
};

}
