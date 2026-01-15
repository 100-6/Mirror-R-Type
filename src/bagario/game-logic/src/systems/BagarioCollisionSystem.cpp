#include "systems/BagarioCollisionSystem.hpp"

namespace bagario::systems {

void BagarioCollisionSystem::set_collision_callback(CollisionCallback callback) {
    m_callback = std::move(callback);
}

void BagarioCollisionSystem::init(Registry& registry) {
}

void BagarioCollisionSystem::update(Registry& registry, float dt) {
    m_events.clear();
    m_virus_shoot_queue.clear();
    auto& positions = registry.get_components<Position>();
    auto& masses = registry.get_components<components::Mass>();
    auto& colliders = registry.get_components<components::CircleCollider>();
    auto& foods = registry.get_components<components::Food>();
    auto& player_cells = registry.get_components<components::PlayerCell>();
    auto& owners = registry.get_components<components::CellOwner>();
    auto& ejected_masses = registry.get_components<components::EjectedMass>();
    auto& viruses = registry.get_components<components::Virus>();
    std::vector<size_t> cells;
    std::vector<size_t> food_entities;
    std::vector<size_t> ejected_entities;
    std::vector<size_t> virus_entities;

    for (size_t i = 0; i < positions.size(); ++i) {
        Entity entity = positions.get_entity_at(i);
        if (!colliders.has_entity(entity))
            continue;
        if (foods.has_entity(entity))
            food_entities.push_back(entity);
        else if (ejected_masses.has_entity(entity))
            ejected_entities.push_back(entity);
        else if (viruses.has_entity(entity))
            virus_entities.push_back(entity);
        else if (player_cells.has_entity(entity) || owners.has_entity(entity))
            cells.push_back(entity);
    }
    for (size_t cell : cells) {
        if (!masses.has_entity(cell))
            continue;
        const auto& cell_pos = positions[cell];
        const auto& cell_collider = colliders[cell];
        auto& cell_mass = masses[cell];
        for (size_t food : food_entities) {
            if (!positions.has_entity(food) || !colliders.has_entity(food))
                continue;
            const auto& food_pos = positions[food];
            const auto& food_collider = colliders[food];
            if (check_eat_collision(cell_pos, cell_collider, food_pos, food_collider)) {
                float nutrition = foods.has_entity(food) ? foods[food].nutrition : config::FOOD_MASS;
                cell_mass.value += nutrition;
                registry.add_component<ToDestroy>(food, ToDestroy{});
                CollisionEvent event;
                event.type = CollisionEvent::Type::CELL_ATE_FOOD;
                event.eater_entity = cell;
                event.eaten_entity = food;
                event.eater_player_id = get_owner_id(registry, cell);
                event.eaten_player_id = 0;
                event.mass_gained = nutrition;
                m_events.push_back(event);
            }
        }
        uint32_t cell_owner = get_owner_id(registry, cell);
        for (size_t ejected : ejected_entities) {
            if (!positions.has_entity(ejected) || !colliders.has_entity(ejected) || !masses.has_entity(ejected))
                continue;
            uint32_t ejected_owner = ejected_masses[ejected].original_owner;
            if (ejected_owner == cell_owner) {
                auto& velocities = registry.get_components<Velocity>();
                if (velocities.has_entity(ejected)) {
                    float speed = std::sqrt(velocities[ejected].x * velocities[ejected].x +
                                           velocities[ejected].y * velocities[ejected].y);
                    if (speed > 50.0f)
                        continue;
                }
            }
            const auto& ejected_pos = positions[ejected];
            const auto& ejected_collider = colliders[ejected];
            if (check_eat_collision(cell_pos, cell_collider, ejected_pos, ejected_collider)) {
                float nutrition = masses[ejected].value;
                cell_mass.value += nutrition;
                registry.add_component<ToDestroy>(ejected, ToDestroy{});
                CollisionEvent event;
                event.type = CollisionEvent::Type::CELL_ATE_FOOD;
                event.eater_entity = cell;
                event.eaten_entity = ejected;
                event.eater_player_id = cell_owner;
                event.eaten_player_id = ejected_owner;
                event.mass_gained = nutrition;
                m_events.push_back(event);
            }
        }
        // Check cell vs virus collision
        for (size_t virus : virus_entities) {
            if (!positions.has_entity(virus) || !colliders.has_entity(virus))
                continue;
            // Skip if virus already marked for destruction
            auto& to_destroy_check = registry.get_components<ToDestroy>();
            if (to_destroy_check.has_entity(virus))
                continue;
            const auto& virus_pos = positions[virus];
            const auto& virus_collider = colliders[virus];
            float dist = distance(cell_pos, virus_pos);
            float combined_radius = cell_collider.radius + virus_collider.radius;
            // Only split if cell is big enough AND overlapping significantly
            if (cell_mass.value >= config::VIRUS_SPLIT_MASS && dist < combined_radius * 0.6f) {
                // Cell hits virus - trigger split event
                // Note: virus destruction is handled by BagarioSession after confirming split succeeded
                CollisionEvent event;
                event.type = CollisionEvent::Type::CELL_HIT_VIRUS;
                event.eater_entity = cell;
                event.eaten_entity = virus;
                event.eater_player_id = cell_owner;
                event.eaten_player_id = 0;
                event.mass_gained = 0;
                m_events.push_back(event);
            }
        }
    }
    // Check ejected mass vs virus collision (feeding viruses)
    for (size_t virus : virus_entities) {
        if (!positions.has_entity(virus) || !colliders.has_entity(virus) || !viruses.has_entity(virus))
            continue;
        const auto& virus_pos = positions[virus];
        const auto& virus_collider = colliders[virus];
        auto& virus_comp = viruses[virus];
        for (size_t ejected : ejected_entities) {
            if (!positions.has_entity(ejected) || !colliders.has_entity(ejected))
                continue;
            // Check if ejected mass is still moving (being fed to virus)
            auto& velocities = registry.get_components<Velocity>();
            if (!velocities.has_entity(ejected))
                continue;
            float speed = std::sqrt(velocities[ejected].x * velocities[ejected].x +
                                   velocities[ejected].y * velocities[ejected].y);
            if (speed < 10.0f)  // Only moving ejected mass can feed viruses
                continue;
            const auto& ejected_pos = positions[ejected];
            const auto& ejected_collider = colliders[ejected];
            if (check_eat_collision(virus_pos, virus_collider, ejected_pos, ejected_collider)) {
                // Virus eats the ejected mass
                registry.add_component<ToDestroy>(ejected, ToDestroy{});
                virus_comp.fed_count++;
                // Visual feedback: make virus "pulse" when absorbing mass
                virus_comp.absorption_scale = 1.25f;  // Grow by 25% (more visible)
                virus_comp.absorption_timer = 0.4f;   // Animation duration (longer)
                // Check if virus should pop and shoot a new one
                if (virus_comp.fed_count >= config::VIRUS_POP_THRESHOLD) {
                    // Use ejected mass velocity direction (same trajectory as the feeding mass)
                    float dir_x = velocities[ejected].x;
                    float dir_y = velocities[ejected].y;
                    float len = std::sqrt(dir_x * dir_x + dir_y * dir_y);
                    if (len > 0.001f) {
                        dir_x /= len;
                        dir_y /= len;
                    } else {
                        // Fallback: use position difference if velocity is zero
                        dir_x = ejected_pos.x - virus_pos.x;
                        dir_y = ejected_pos.y - virus_pos.y;
                        len = std::sqrt(dir_x * dir_x + dir_y * dir_y);
                        if (len > 0.001f) {
                            dir_x /= len;
                            dir_y /= len;
                        } else {
                            dir_x = 1.0f;
                            dir_y = 0.0f;
                        }
                    }
                    // Store direction for virus shooting (handled by VirusSystem)
                    m_virus_shoot_queue.push_back({virus, dir_x, dir_y});
                    virus_comp.fed_count = 0;
                }
            }
        }
    }
    for (size_t i = 0; i < cells.size(); ++i) {
        size_t cell_a = cells[i];
        if (!masses.has_entity(cell_a) || !positions.has_entity(cell_a))
            continue;
        const auto& pos_a = positions[cell_a];
        const auto& collider_a = colliders[cell_a];
        auto& mass_a = masses[cell_a];
        uint32_t owner_a = get_owner_id(registry, cell_a);
        for (size_t j = i + 1; j < cells.size(); ++j) {
            size_t cell_b = cells[j];
            if (!masses.has_entity(cell_b) || !positions.has_entity(cell_b))
                continue;
            const auto& pos_b = positions[cell_b];
            const auto& collider_b = colliders[cell_b];
            auto& mass_b = masses[cell_b];
            uint32_t owner_b = get_owner_id(registry, cell_b);
            if (owner_a == owner_b && owner_a != 0) {
                float dist = distance(pos_a, pos_b);
                float combined_radius = collider_a.radius + collider_b.radius;
                auto& merge_timers = registry.get_components<components::MergeTimer>();
                bool a_can_merge = !merge_timers.has_entity(cell_a) ||
                                   merge_timers[cell_a].can_merge;
                bool b_can_merge = !merge_timers.has_entity(cell_b) ||
                                   merge_timers[cell_b].can_merge;
                if (a_can_merge && b_can_merge) {
                    // Both can merge - need significant overlap (40% of combined radius)
                    // dist < combined_radius * 0.6 means ~40% overlap
                    if (dist < combined_radius * 0.6f) {
                        if (mass_a.value >= mass_b.value) {
                            mass_a.value += mass_b.value;
                            registry.add_component<ToDestroy>(cell_b, ToDestroy{});
                            colliders[cell_a].radius = config::mass_to_radius(mass_a.value);
                            // Emit merge event so session can update player_cells tracking
                            CollisionEvent event;
                            event.type = CollisionEvent::Type::CELL_MERGED;
                            event.eater_entity = cell_a;
                            event.eaten_entity = cell_b;
                            event.eater_player_id = owner_a;
                            event.eaten_player_id = owner_b;
                            event.mass_gained = mass_b.value;
                            m_events.push_back(event);
                        } else {
                            mass_b.value += mass_a.value;
                            registry.add_component<ToDestroy>(cell_a, ToDestroy{});
                            colliders[cell_b].radius = config::mass_to_radius(mass_b.value);
                            // Emit merge event so session can update player_cells tracking
                            CollisionEvent event;
                            event.type = CollisionEvent::Type::CELL_MERGED;
                            event.eater_entity = cell_b;
                            event.eaten_entity = cell_a;
                            event.eater_player_id = owner_b;
                            event.eaten_player_id = owner_a;
                            event.mass_gained = mass_a.value;
                            m_events.push_back(event);
                        }
                    }
                } else {
                    if (dist < combined_radius && dist > 0.001f) {
                        auto& velocities = registry.get_components<Velocity>();
                        if (positions.has_entity(cell_a) && positions.has_entity(cell_b)) {
                            float overlap = combined_radius - dist;
                            float dx = pos_b.x - pos_a.x;
                            float dy = pos_b.y - pos_a.y;
                            float nx = dx / dist;
                            float ny = dy / dist;
                            float push = overlap * 0.5f;
                            positions[cell_a].x -= nx * push;
                            positions[cell_a].y -= ny * push;
                            positions[cell_b].x += nx * push;
                            positions[cell_b].y += ny * push;
                        }
                    }
                }
                continue;
            }
            float dist = distance(pos_a, pos_b);
            float combined_radius = collider_a.radius + collider_b.radius;
            if (dist < combined_radius * config::EAT_OVERLAP_RATIO) {
                if (config::can_eat(mass_a.value, mass_b.value)) {
                    // A eats B
                    eat_cell(registry, cell_a, cell_b, mass_a, mass_b, owner_a, owner_b);
                } else if (config::can_eat(mass_b.value, mass_a.value)) {
                    // B eats A
                    eat_cell(registry, cell_b, cell_a, mass_b, mass_a, owner_b, owner_a);
                }
            }
        }
    }
    if (m_callback)
        for (const auto& event : m_events)
            m_callback(event);
}

void BagarioCollisionSystem::shutdown() {
    m_callback = nullptr;
    m_events.clear();
}

const std::vector<CollisionEvent>& BagarioCollisionSystem::get_events() const {
    return m_events;
}

float BagarioCollisionSystem::distance(const Position& a, const Position& b) const {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

bool BagarioCollisionSystem::check_eat_collision(
    const Position& eater_pos, const components::CircleCollider& eater_col,
    const Position& food_pos, const components::CircleCollider& food_col
) const {
    float dist = distance(eater_pos, food_pos);

    return dist < eater_col.radius - food_col.radius * 0.5f;
}

uint32_t BagarioCollisionSystem::get_owner_id(Registry& registry, size_t entity) {
    auto& player_cells = registry.get_components<components::PlayerCell>();
    auto& owners = registry.get_components<components::CellOwner>();

    if (player_cells.has_entity(entity))
        return player_cells[entity].player_id;
    if (owners.has_entity(entity))
        return owners[entity].owner_id;
    return 0;
}

void BagarioCollisionSystem::eat_cell(
    Registry& registry,
    size_t eater, size_t eaten,
    components::Mass& eater_mass, components::Mass& eaten_mass,
    uint32_t eater_owner, uint32_t eaten_owner
) {
    eater_mass.value += eaten_mass.value;

    registry.add_component<ToDestroy>(eaten, ToDestroy{});
    CollisionEvent event;
    event.type = CollisionEvent::Type::CELL_ATE_CELL;
    event.eater_entity = eater;
    event.eaten_entity = eaten;
    event.eater_player_id = eater_owner;
    event.eaten_player_id = eaten_owner;
    event.mass_gained = eaten_mass.value;
    m_events.push_back(event);
}

}
