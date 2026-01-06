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
    resolve_wall_collisions(registry, predicted_x, predicted_y, player_collider);

    Position& position = positions[player];
    position.x = predicted_x;
    position.y = predicted_y;
}

void LocalPredictionSystem::clamp_position(float& x, float& y, const Collider& collider) const {
    if (x < 0.0f)
        x = 0.0f;
    if (x + collider.width > screen_width_)
        x = std::max(0.0f, screen_width_ - collider.width);

    if (y < 0.0f)
        y = 0.0f;
    if (y + collider.height > screen_height_)
        y = std::max(0.0f, screen_height_ - collider.height);
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

    for (size_t i = 0; i < walls.size(); ++i) {
        Entity wall_entity = walls.get_entity_at(i);

        if (!positions.has_entity(wall_entity) || !colliders.has_entity(wall_entity))
            continue;

        const Position& wall_pos = positions[wall_entity];
        const Collider& wall_col = colliders[wall_entity];

        bool overlap = (x < wall_pos.x + wall_col.width &&
                        x + player_collider.width > wall_pos.x &&
                        y < wall_pos.y + wall_col.height &&
                        y + player_collider.height > wall_pos.y);

        if (!overlap)
            continue;

        float overlap_left = (x + player_collider.width) - wall_pos.x;
        float overlap_right = (wall_pos.x + wall_col.width) - x;
        float overlap_top = (y + player_collider.height) - wall_pos.y;
        float overlap_bottom = (wall_pos.y + wall_col.height) - y;

        float min_overlap = std::min({overlap_left, overlap_right, overlap_top, overlap_bottom});

        if (min_overlap == overlap_left) {
            x = wall_pos.x - player_collider.width;
        } else if (min_overlap == overlap_right) {
            x = wall_pos.x + wall_col.width;
        } else if (min_overlap == overlap_top) {
            y = wall_pos.y - player_collider.height;
        } else {
            y = wall_pos.y + wall_col.height;
        }
    }
}

}
