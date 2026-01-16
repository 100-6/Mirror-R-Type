#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <cstdint>
#include <limits>
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "components/GameComponents.hpp"
#include "protocol/PacketTypes.hpp"
#include "TextureManager.hpp"

namespace rtype::client {

/**
 * @brief Manages client-side entity creation, updates, and destruction
 */
class EntityManager {
public:
    EntityManager(Registry& registry, TextureManager& textures, int screen_width, int screen_height);

    /**
     * @brief Spawn or update an entity from server data
     */
    Entity spawn_or_update_entity(uint32_t server_id, protocol::EntityType type,
                                  float x, float y, uint16_t health, uint8_t subtype);

    /**
     * @brief Remove an entity by server ID
     */
    void remove_entity(uint32_t server_id);

    /**
     * @brief Clear all remote entities
     */
    void clear_all();

    /**
     * @brief Process snapshot updates (mark entities as stale if not updated)
     */
    void process_snapshot_update(const std::unordered_set<uint32_t>& updated_ids);

    /**
     * @brief Update projectile positions locally (client-side prediction)
     */
    void update_projectiles(float delta_time);

    /**
     * @brief Update player name tags positions
     */
    void update_name_tags();

    /**
     * @brief Spawn a transient explosion effect at the given coordinates
     */
    void spawn_explosion(float x, float y, float scale);

    /**
     * @brief Set local player ID
     */
    void set_local_player_id(uint32_t player_id) { local_player_id_ = player_id; }

    /**
     * @brief Get local player server entity ID
     */
    uint32_t get_local_player_entity_id() const { return local_player_entity_id_; }

    /**
     * @brief Set player name
     */
    void set_player_name(uint32_t server_id, const std::string& name);

    /**
     * @brief Get entity by server ID
     */
    Entity get_entity(uint32_t server_id) const;

    /**
     * @brief Check if entity exists
     */
    bool has_entity(uint32_t server_id) const;

    /**
     * @brief Get entity type by server ID
     * @param server_id Server ID of the entity
     * @param out_type Output parameter for the entity type
     * @return true if entity exists and type was retrieved, false otherwise
     */
    bool get_entity_type(uint32_t server_id, protocol::EntityType& out_type) const;

private:
    Registry& registry_;
    TextureManager& textures_;
    int screen_width_;
    int screen_height_;

    // Entity tracking
    std::unordered_map<uint32_t, Entity> server_to_local_;
    std::unordered_map<uint32_t, protocol::EntityType> server_types_;
    std::unordered_map<uint32_t, uint8_t> stale_counters_;
    std::unordered_set<uint32_t> locally_integrated_;  // Projectiles updated client-side
    std::unordered_set<uint32_t> snapshot_updated_;

    // Player management
    uint32_t local_player_id_;
    uint32_t local_player_entity_id_;
    std::unordered_map<uint32_t, std::string> player_names_;  // player_id -> name
    std::unordered_map<uint32_t, Entity> player_name_tags_;   // server_id -> name tag entity
    std::unordered_map<uint32_t, uint8_t> server_to_player_id_;  // server_id -> player_id (from subtype)

    // Helper functions
    Sprite build_sprite(protocol::EntityType type, bool is_local_player, uint8_t subtype);
    void ensure_player_name_tag(uint32_t server_id, float x, float y);
    std::string get_player_label(uint32_t server_id);
};

}
