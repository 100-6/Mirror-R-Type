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
 * @brief System that manages viruses
 *
 * Responsibilities:
 * - Spawn initial viruses
 * - Maintain virus count on the map
 * - Handle virus shooting when fed enough mass
 */
class VirusSystem : public ISystem {
public:
    using NetworkIdGenerator = std::function<uint32_t()>;
    using SpawnCallback = std::function<void(uint32_t net_id, float x, float y, float mass)>;
    using DestroyCallback = std::function<void(uint32_t net_id)>;

    VirusSystem();
    ~VirusSystem() override = default;

    void set_network_id_generator(NetworkIdGenerator generator);
    void set_spawn_callback(SpawnCallback callback);
    void set_destroy_callback(DestroyCallback callback);

    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override;

    /**
     * @brief Spawn a virus at the given position
     */
    size_t spawn_virus(Registry& registry, float x, float y);

    /**
     * @brief Shoot a new virus from an existing virus in a direction
     */
    size_t shoot_virus(Registry& registry, float x, float y, float dir_x, float dir_y);

    /**
     * @brief Get current virus count
     */
    int get_virus_count() const { return m_virus_count; }

private:
    void spawn_initial_viruses(Registry& registry);
    uint32_t get_next_network_id();

    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_pos_dist_x;
    std::uniform_real_distribution<float> m_pos_dist_y;

    int m_virus_count = 0;
    uint32_t m_fallback_network_id = 200000;

    NetworkIdGenerator m_network_id_generator;
    SpawnCallback m_spawn_callback;
    DestroyCallback m_destroy_callback;
};

}