#pragma once

#include "ecs/systems/ISystem.hpp"
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "../components/BagarioComponents.hpp"
#include "BagarioConfig.hpp"
#include <random>
#include <functional>

namespace bagario::systems {

/**
 * @brief System that spawns and manages food pellets
 *
 * Responsibilities:
 * - Maintain a target number of food pellets on the map
 * - Spawn food at random positions
 * - Assign random colors to food
 */
class FoodSpawnerSystem : public ISystem {
public:
    using SpawnCallback = std::function<void(size_t entity_id, float x, float y, uint32_t color)>;
    using NetworkIdGenerator = std::function<uint32_t()>;

    FoodSpawnerSystem();
    ~FoodSpawnerSystem() override = default;

    void set_spawn_callback(SpawnCallback callback);
    void set_network_id_generator(NetworkIdGenerator generator);
    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

private:
    void spawn_food_batch(Registry& registry, int count);
    void spawn_single_food(Registry& registry);
    uint32_t generate_random_color();
    uint32_t get_next_network_id();

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_pos_dist_x;
    std::uniform_real_distribution<float> m_pos_dist_y;
    std::uniform_int_distribution<int> m_color_dist;

    float m_spawn_timer = 0.0f;
    float m_spawn_interval = 1.0f / config::FOOD_SPAWN_RATE;

    uint32_t m_fallback_network_id = 100000;  // Start high to avoid conflicts if no generator set

    SpawnCallback m_spawn_callback;
    NetworkIdGenerator m_network_id_generator;
};

}
