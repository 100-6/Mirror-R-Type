#pragma once

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "../components/BagarioComponents.hpp"
#include "BagarioConfig.hpp"
#include <algorithm>

namespace bagario::systems {

/**
 * @brief System that keeps entities within map boundaries
 *
 * Responsibilities:
 * - Clamp entity positions to map bounds
 * - Account for entity radius when clamping
 */
class MapBoundsSystem : public ISystem {
public:
    MapBoundsSystem(float width = config::MAP_WIDTH, float height = config::MAP_HEIGHT);
    ~MapBoundsSystem() override = default;

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;
    void set_bounds(float width, float height);

private:
    float m_map_width;
    float m_map_height;
};

}
