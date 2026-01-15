#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

#include "PacketTypes.hpp"
#include "Payloads.hpp"
#include "LocalGameState.hpp"
#include "BagarioConfig.hpp"

namespace bagario::client {

/**
 * @brief Cached entity data with interpolation support
 */
struct CachedEntity {
    uint32_t entity_id = 0;
    protocol::EntityType type = protocol::EntityType::FOOD;

    // Current position (target from last snapshot)
    float x = 0.0f;
    float y = 0.0f;

    // Previous position (for interpolation)
    float prev_x = 0.0f;
    float prev_y = 0.0f;

    // Other properties
    float mass = 1.0f;
    uint32_t color = 0xFFFFFFFF;
    uint32_t owner_id = 0;

    // Skin for player cells (deserialized from skin data)
    PlayerSkin skin;
    bool has_skin = false;

    // Interpolation progress (0.0 = at prev position, 1.0 = at current position)
    float interpolation_t = 1.0f;

    /**
     * @brief Get interpolated X position
     */
    float get_interpolated_x() const {
        return prev_x + (x - prev_x) * interpolation_t;
    }

    /**
     * @brief Get interpolated Y position
     */
    float get_interpolated_y() const {
        return prev_y + (y - prev_y) * interpolation_t;
    }

    /**
     * @brief Calculate radius from mass
     */
    float get_radius() const {
        return config::mass_to_radius(mass);
    }
};

/**
 * @brief Leaderboard information
 */
struct LeaderboardInfo {
    std::vector<protocol::LeaderboardEntry> entries;
};

/**
 * @brief Client-side game state manager
 *
 * Maintains a cache of all entities received from the server
 * and handles interpolation between snapshots.
 */
class ClientGameState {
public:
    ClientGameState();
    ~ClientGameState() = default;

    // ============== Entity Management ==============

    /**
     * @brief Update entity cache from a server snapshot
     * @param header Snapshot header (tick, entity count)
     * @param entities Entity states from snapshot
     */
    void update_from_snapshot(const protocol::ServerSnapshotPayload& header,
                              const std::vector<protocol::EntityState>& entities);

    /**
     * @brief Handle entity spawn event
     */
    void handle_entity_spawn(const protocol::ServerEntitySpawnPayload& spawn);

    /**
     * @brief Handle entity destroy event
     */
    void handle_entity_destroy(const protocol::ServerEntityDestroyPayload& destroy);

    /**
     * @brief Update a player's skin
     * @param player_id Player ID (owner_id)
     * @param skin_data Serialized skin data
     */
    void update_player_skin(uint32_t player_id, const std::vector<uint8_t>& skin_data);

    /**
     * @brief Update interpolation progress for all entities
     * @param dt Delta time since last frame
     */
    void update_interpolation(float dt);

    // ============== Entity Queries ==============

    /**
     * @brief Get all cached entities
     */
    const std::unordered_map<uint32_t, CachedEntity>& get_entities() const {
        return m_entities;
    }

    /**
     * @brief Get a specific entity by ID
     * @return Pointer to entity or nullptr if not found
     */
    const CachedEntity* get_entity(uint32_t id) const;

    /**
     * @brief Get the first cell of the local player
     * @return Pointer to player cell or nullptr if not found/dead
     */
    const CachedEntity* get_local_player_cell() const;

    /**
     * @brief Get all cells owned by a player
     */
    std::vector<const CachedEntity*> get_player_cells(uint32_t player_id) const;

    /**
     * @brief Calculate total mass of a player (all their cells)
     */
    float get_player_total_mass(uint32_t player_id) const;

    /**
     * @brief Calculate center of mass for a player's cells
     * @param out_x Output X coordinate
     * @param out_y Output Y coordinate
     * @return true if player has cells, false otherwise
     */
    bool get_player_center(uint32_t player_id, float& out_x, float& out_y) const;

    // ============== Player Info ==============

    void set_local_player_id(uint32_t id) { m_local_player_id = id; }
    uint32_t get_local_player_id() const { return m_local_player_id; }

    // ============== Map Info ==============

    void set_map_size(float width, float height);
    float get_map_width() const { return m_map_width; }
    float get_map_height() const { return m_map_height; }

    // ============== Leaderboard ==============

    void update_leaderboard(const protocol::ServerLeaderboardPayload& header,
                           const std::vector<protocol::LeaderboardEntry>& entries);
    const LeaderboardInfo& get_leaderboard() const { return m_leaderboard; }

    // ============== Server State ==============

    uint32_t get_last_server_tick() const { return m_last_server_tick; }

    /**
     * @brief Clear all game state (on disconnect)
     */
    void clear();

private:
    std::unordered_map<uint32_t, CachedEntity> m_entities;
    std::unordered_map<uint32_t, PlayerSkin> m_player_skins;  // player_id -> skin

    uint32_t m_local_player_id = 0;
    float m_map_width = config::MAP_WIDTH;
    float m_map_height = config::MAP_HEIGHT;
    uint32_t m_last_server_tick = 0;
    LeaderboardInfo m_leaderboard;

    // Interpolation timing (matches 20Hz snapshot rate = 50ms)
    static constexpr float INTERPOLATION_DURATION = 0.05f;
};

}
