#pragma once

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "../components/BagarioComponents.hpp"
#include "BagarioConfig.hpp"
#include <functional>
#include <cmath>
#include <vector>

namespace bagario::systems {

/**
 * @brief Collision events for external handling
 */
struct CollisionEvent {
    enum class Type {
        CELL_ATE_FOOD,
        CELL_ATE_CELL,
        CELL_HIT_VIRUS,
        PLAYER_ELIMINATED
    };

    Type type;
    size_t eater_entity;
    size_t eaten_entity;
    uint32_t eater_player_id;
    uint32_t eaten_player_id;
    float mass_gained;
};

/**
 * @brief System that handles collision detection and eating mechanics
 *
 * Responsibilities:
 * - Detect circle-circle collisions
 * - Handle cell eating food
 * - Handle cell eating smaller cells
 * - Handle virus collisions (splitting)
 */
class BagarioCollisionSystem : public ISystem {
public:
    using CollisionCallback = std::function<void(const CollisionEvent&)>;

    BagarioCollisionSystem() = default;
    ~BagarioCollisionSystem() override = default;

    void set_collision_callback(CollisionCallback callback);
    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

    const std::vector<CollisionEvent>& get_events() const;

private:
    float distance(const Position& a, const Position& b) const;
    bool check_eat_collision(
        const Position& eater_pos, const components::CircleCollider& eater_col,
        const Position& food_pos, const components::CircleCollider& food_col
    ) const;
    uint32_t get_owner_id(Registry& registry, size_t entity);
    void eat_cell(
        Registry& registry,
        size_t eater, size_t eaten,
        components::Mass& eater_mass, components::Mass& eaten_mass,
        uint32_t eater_owner, uint32_t eaten_owner
    );

    CollisionCallback m_callback;
    std::vector<CollisionEvent> m_events;
};

}
