#include "systems/BagarioCollisionSystem.hpp"

namespace bagario::systems {

void BagarioCollisionSystem::set_collision_callback(CollisionCallback callback) {
    m_callback = std::move(callback);
}

void BagarioCollisionSystem::init(Registry& registry) {
}

void BagarioCollisionSystem::update(Registry& registry, float dt) {
    m_events.clear();
    auto& positions = registry.get_components<Position>();
    auto& masses = registry.get_components<components::Mass>();
    auto& colliders = registry.get_components<components::CircleCollider>();
    auto& foods = registry.get_components<components::Food>();
    auto& player_cells = registry.get_components<components::PlayerCell>();
    auto& owners = registry.get_components<components::CellOwner>();
    std::vector<size_t> cells;
    std::vector<size_t> food_entities;

    for (size_t i = 0; i < positions.size(); ++i) {
        Entity entity = positions.get_entity_at(i);
        if (!colliders.has_entity(entity))
            continue;
        if (foods.has_entity(entity))
            food_entities.push_back(entity);
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
                // TODO: Handle merge logic
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
