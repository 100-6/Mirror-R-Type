#include "ClientGameState.hpp"
#include <algorithm>
#include <unordered_set>
#include <cmath>

namespace bagario::client {

ClientGameState::ClientGameState() = default;

void ClientGameState::update_from_snapshot(const protocol::ServerSnapshotPayload& header,
                                           const std::vector<protocol::EntityState>& entities) {
    m_last_server_tick = header.server_tick;

    // Track which entities are in this snapshot
    std::unordered_set<uint32_t> seen_ids;

    for (const auto& state : entities) {
        seen_ids.insert(state.entity_id);

        auto it = m_entities.find(state.entity_id);
        if (it != m_entities.end()) {
            // Update existing entity - store current interpolated position as new start point
            // This prevents visual jumps when snapshots arrive before interpolation completes
            auto& cached = it->second;
            cached.prev_x = cached.get_interpolated_x();
            cached.prev_y = cached.get_interpolated_y();
            cached.x = state.position_x;
            cached.y = state.position_y;
            cached.mass = state.mass;
            cached.color = state.color;
            cached.interpolation_t = 0.0f;  // Reset interpolation
        } else {
            // New entity
            CachedEntity cached;
            cached.entity_id = state.entity_id;
            cached.type = state.entity_type;
            cached.x = state.position_x;
            cached.y = state.position_y;
            cached.prev_x = state.position_x;  // No interpolation for new entities
            cached.prev_y = state.position_y;
            cached.mass = state.mass;
            cached.color = state.color;
            cached.owner_id = state.owner_id;
            cached.interpolation_t = 1.0f;  // Start fully interpolated

            // Apply stored skin if this is a player cell with known skin
            if (state.entity_type == protocol::EntityType::PLAYER_CELL) {
                auto skin_it = m_player_skins.find(state.owner_id);
                if (skin_it != m_player_skins.end()) {
                    cached.skin = skin_it->second;
                    cached.has_skin = true;
                }
            }

            m_entities[state.entity_id] = cached;
        }
    }

    // Remove entities not in snapshot (they were destroyed)
    for (auto it = m_entities.begin(); it != m_entities.end();) {
        if (seen_ids.find(it->first) == seen_ids.end()) {
            it = m_entities.erase(it);
        } else {
            ++it;
        }
    }
}

void ClientGameState::handle_entity_spawn(const protocol::ServerEntitySpawnPayload& spawn) {
    CachedEntity cached;
    cached.entity_id = spawn.entity_id;
    cached.type = spawn.entity_type;
    cached.x = spawn.spawn_x;
    cached.y = spawn.spawn_y;
    cached.prev_x = spawn.spawn_x;
    cached.prev_y = spawn.spawn_y;
    cached.mass = spawn.mass;
    cached.color = spawn.color;
    cached.owner_id = spawn.owner_id;
    cached.interpolation_t = 1.0f;

    // Apply stored skin if applicable
    if (spawn.entity_type == protocol::EntityType::PLAYER_CELL) {
        auto skin_it = m_player_skins.find(spawn.owner_id);
        if (skin_it != m_player_skins.end()) {
            cached.skin = skin_it->second;
            cached.has_skin = true;
        }
    }

    m_entities[spawn.entity_id] = cached;
}

void ClientGameState::handle_entity_destroy(const protocol::ServerEntityDestroyPayload& destroy) {
    m_entities.erase(destroy.entity_id);
}

void ClientGameState::update_player_skin(uint32_t player_id, const std::vector<uint8_t>& skin_data) {
    PlayerSkin skin;
    if (!skin.deserialize(skin_data)) {
        return;
    }

    // Store the skin for this player
    m_player_skins[player_id] = skin;

    // Update all entities owned by this player
    for (auto& [id, entity] : m_entities) {
        if (entity.type == protocol::EntityType::PLAYER_CELL && entity.owner_id == player_id) {
            entity.skin = skin;
            entity.has_skin = true;
        }
    }
}

void ClientGameState::update_interpolation(float dt) {
    float progress = dt / INTERPOLATION_DURATION;

    for (auto& [id, entity] : m_entities) {
        if (entity.interpolation_t < 1.0f) {
            entity.interpolation_t = std::min(1.0f, entity.interpolation_t + progress);
        }
    }
}

const CachedEntity* ClientGameState::get_entity(uint32_t id) const {
    auto it = m_entities.find(id);
    return (it != m_entities.end()) ? &it->second : nullptr;
}

const CachedEntity* ClientGameState::get_local_player_cell() const {
    if (m_local_player_id == 0) {
        return nullptr;
    }

    // Find any cell owned by the local player
    for (const auto& [id, entity] : m_entities) {
        if (entity.type == protocol::EntityType::PLAYER_CELL &&
            entity.owner_id == m_local_player_id) {
            return &entity;
        }
    }

    return nullptr;
}

std::vector<const CachedEntity*> ClientGameState::get_player_cells(uint32_t player_id) const {
    std::vector<const CachedEntity*> cells;

    for (const auto& [id, entity] : m_entities) {
        if (entity.type == protocol::EntityType::PLAYER_CELL &&
            entity.owner_id == player_id) {
            cells.push_back(&entity);
        }
    }

    return cells;
}

float ClientGameState::get_player_total_mass(uint32_t player_id) const {
    float total = 0.0f;

    for (const auto& [id, entity] : m_entities) {
        if (entity.type == protocol::EntityType::PLAYER_CELL &&
            entity.owner_id == player_id) {
            total += entity.mass;
        }
    }

    return total;
}

bool ClientGameState::get_player_center(uint32_t player_id, float& out_x, float& out_y) const {
    float total_x = 0.0f;
    float total_y = 0.0f;
    float total_mass = 0.0f;

    for (const auto& [id, entity] : m_entities) {
        if (entity.type == protocol::EntityType::PLAYER_CELL &&
            entity.owner_id == player_id) {
            // Weight by mass for center of mass
            float x = entity.get_interpolated_x();
            float y = entity.get_interpolated_y();
            total_x += x * entity.mass;
            total_y += y * entity.mass;
            total_mass += entity.mass;
        }
    }

    if (total_mass == 0.0f) {
        return false;
    }

    out_x = total_x / total_mass;
    out_y = total_y / total_mass;
    return true;
}

void ClientGameState::set_map_size(float width, float height) {
    m_map_width = width;
    m_map_height = height;
}

void ClientGameState::update_leaderboard(const protocol::ServerLeaderboardPayload& header,
                                         const std::vector<protocol::LeaderboardEntry>& entries) {
    m_leaderboard.entries = entries;
}

void ClientGameState::clear() {
    m_entities.clear();
    m_player_skins.clear();
    m_local_player_id = 0;
    m_last_server_tick = 0;
    m_leaderboard.entries.clear();
}

}  // namespace bagario::client
