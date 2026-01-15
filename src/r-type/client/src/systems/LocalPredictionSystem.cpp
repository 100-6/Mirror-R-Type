/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LocalPredictionSystem implementation
*/

#include "systems/LocalPredictionSystem.hpp"
#include "EntityManager.hpp"
#include "ecs/Registry.hpp"
#include <algorithm>

namespace rtype::client {

LocalPredictionSystem::LocalPredictionSystem(EntityManager& entity_manager,
                                             float screen_width,
                                             float screen_height)
    : entity_manager_(entity_manager)
    , screen_width_(screen_width)
    , screen_height_(screen_height)
    , current_time_(0.0f) {
}

void LocalPredictionSystem::init(Registry& registry) {
    (void)registry;
}

void LocalPredictionSystem::shutdown() {
}

void LocalPredictionSystem::update(Registry& registry, float dt) {
    (void)dt;

    if (!registry.has_component_registered<LastServerState>())
        return;

    uint32_t local_id = entity_manager_.get_local_player_entity_id();
    if (local_id == 0 || !entity_manager_.has_entity(local_id))
        return;

    Entity player = entity_manager_.get_entity(local_id);
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& colliders = registry.get_components<Collider>();
    auto& last_states = registry.get_components<LastServerState>();

    if (!positions.has_entity(player) ||
        !velocities.has_entity(player) ||
        !colliders.has_entity(player) ||
        !last_states.has_entity(player)) {
        return;
    }

    const LastServerState& last_state = last_states[player];
    const Velocity& velocity = velocities[player];
    const Collider& player_collider = colliders[player];

    float elapsed = current_time_ - last_state.timestamp;
    if (elapsed < 0.0f)
        elapsed = 0.0f;
    if (elapsed > MAX_EXTRAPOLATION_SECONDS)
        elapsed = MAX_EXTRAPOLATION_SECONDS;

    float predicted_x = last_state.x + velocity.x * elapsed;
    float predicted_y = last_state.y + velocity.y * elapsed;

    clamp_position(predicted_x, predicted_y, player_collider);
    // Wall collisions are now synchronized via scroll_x in server snapshots,
    // so client-side prediction should match server-side collision detection.
    resolve_wall_collisions(registry, predicted_x, predicted_y, player_collider);

    Position& position = positions[player];
    position.x = predicted_x;
    position.y = predicted_y;
}

void LocalPredictionSystem::clamp_position(float& x, float& y, const Collider& collider) const {
    // Center-based clamping using half-extents
    float half_w = collider.width * 0.5f;
    float half_h = collider.height * 0.5f;

    // Clamp X to keep entity within screen bounds
    if (x - half_w < 0.0f)
        x = half_w;
    if (x + half_w > screen_width_)
        x = screen_width_ - half_w;

    // Clamp Y to keep entity within screen bounds
    if (y - half_h < 0.0f)
        y = half_h;
    if (y + half_h > screen_height_)
        y = screen_height_ - half_h;
}

void LocalPredictionSystem::resolve_wall_collisions(Registry& registry,
                                                    float& x,
                                                    float& y,
                                                    const Collider& player_collider) const {
    if (!registry.has_component_registered<Wall>())
        return;

    auto& positions = registry.get_components<Position>();
    auto& colliders = registry.get_components<Collider>();
    auto& walls = registry.get_components<Wall>();

    // Center-based collision resolution using half-extents
    float half_pw = player_collider.width * 0.5f;
    float half_ph = player_collider.height * 0.5f;

    for (size_t i = 0; i < walls.size(); ++i) {
        Entity wall_entity = walls.get_entity_at(i);

        if (!positions.has_entity(wall_entity) || !colliders.has_entity(wall_entity))
            continue;

        const Position& wall_pos = positions[wall_entity];
        const Collider& wall_col = colliders[wall_entity];

        float half_ww = wall_col.width * 0.5f;
        float half_wh = wall_col.height * 0.5f;

        // Check for overlap using center-based AABB
        bool overlap = (x + half_pw > wall_pos.x - half_ww &&
                        x - half_pw < wall_pos.x + half_ww &&
                        y + half_ph > wall_pos.y - half_wh &&
                        y - half_ph < wall_pos.y + half_wh);

        if (!overlap)
            continue;

        // Calculate overlap amounts on each axis
        float overlap_left = (x + half_pw) - (wall_pos.x - half_ww);
        float overlap_right = (wall_pos.x + half_ww) - (x - half_pw);
        float overlap_top = (y + half_ph) - (wall_pos.y - half_wh);
        float overlap_bottom = (wall_pos.y + half_wh) - (y - half_ph);

        float min_overlap = std::min({overlap_left, overlap_right, overlap_top, overlap_bottom});

        // Push player out on axis with minimum overlap
        if (min_overlap == overlap_left) {
            x -= overlap_left;
        } else if (min_overlap == overlap_right) {
            x += overlap_right;
        } else if (min_overlap == overlap_top) {
            y -= overlap_top;
        } else {
            y += overlap_bottom;
        }
    }
}

}
