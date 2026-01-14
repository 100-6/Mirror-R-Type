#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <random>

#include "ecs/Registry.hpp"
#include "components/BagarioComponents.hpp"
#include "systems/MassSystem.hpp"
#include "systems/FoodSpawnerSystem.hpp"
#include "systems/BagarioCollisionSystem.hpp"
#include "systems/MapBoundsSystem.hpp"
#include "systems/MovementTargetSystem.hpp"
#include "Payloads.hpp"

namespace bagario::server {

/**
 * @brief Game session callbacks for network events
 */
struct SessionCallbacks {
    std::function<void(const protocol::ServerEntitySpawnPayload&)> on_entity_spawn;
    std::function<void(const protocol::ServerEntityDestroyPayload&)> on_entity_destroy;
    std::function<void(uint32_t player_id, uint32_t killer_id)> on_player_eliminated;
};

/**
 * @brief A single Bagario game session
 *
 * Contains:
 * - ECS Registry with all entities
 * - Game systems (Mass, Food, Collision, Bounds, Movement)
 * - Player management
 */
class BagarioSession {
public:
    BagarioSession();
    ~BagarioSession();

    /**
     * @brief Initialize the session
     */
    void init();

    /**
     * @brief Update game logic
     * @param dt Delta time in seconds
     */
    void update(float dt);

    /**
     * @brief Shutdown the session
     */
    void shutdown();

    /**
     * @brief Set callbacks for network events
     */
    void set_callbacks(const SessionCallbacks& callbacks);

    // Player management
    /**
     * @brief Add a new player to the session
     * @return Entity ID of the player's cell
     */
    size_t add_player(uint32_t player_id, const std::string& name, uint32_t color);

    /**
     * @brief Remove a player from the session
     */
    void remove_player(uint32_t player_id);

    /**
     * @brief Check if player exists
     */
    bool has_player(uint32_t player_id) const;

    /**
     * @brief Update player's movement target (mouse position)
     */
    void set_player_target(uint32_t player_id, float target_x, float target_y);

    /**
     * @brief Request player to split their cells
     */
    void player_split(uint32_t player_id);

    /**
     * @brief Request player to eject mass
     */
    void player_eject_mass(uint32_t player_id, float dir_x, float dir_y);

    /**
     * @brief Get current world state for network snapshot
     */
    std::vector<protocol::EntityState> get_snapshot() const;

    /**
     * @brief Get leaderboard data (top 10 players by mass)
     */
    std::vector<protocol::LeaderboardEntry> get_leaderboard() const;

    /**
     * @brief Get total mass of a player (sum of all their cells)
     */
    float get_player_total_mass(uint32_t player_id) const;

    /**
     * @brief Get player cell count
     */
    size_t get_player_cell_count(uint32_t player_id) const;

private:
    void register_components();
    void setup_systems();
    void handle_collision_event(const systems::CollisionEvent& event);
    size_t spawn_player_cell(uint32_t player_id, const std::string& name, uint32_t color,
                             float x, float y, float mass);
    uint32_t get_next_network_id();

    Registry m_registry;

    // Systems
    std::unique_ptr<systems::MassSystem> m_mass_system;
    std::unique_ptr<systems::FoodSpawnerSystem> m_food_spawner;
    std::unique_ptr<systems::BagarioCollisionSystem> m_collision_system;
    std::unique_ptr<systems::MapBoundsSystem> m_bounds_system;
    std::unique_ptr<systems::MovementTargetSystem> m_movement_target_system;

    // Player tracking
    // player_id -> list of entity IDs (cells owned by this player)
    std::unordered_map<uint32_t, std::vector<size_t>> m_player_cells;
    std::unordered_map<uint32_t, std::string> m_player_names;
    std::unordered_map<uint32_t, uint32_t> m_player_colors;

    // Network ID generation
    uint32_t m_next_network_id = 1;

    // Callbacks
    SessionCallbacks m_callbacks;

    // Random for spawn positions
    std::mt19937 m_rng{std::random_device{}()};
};

}
