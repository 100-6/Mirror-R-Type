#include "BagarioSession.hpp"
#include "ecs/CoreComponents.hpp"
#include "BagarioConfig.hpp"

#include <algorithm>
#include <iostream>

namespace bagario::server {

BagarioSession::BagarioSession() {}

BagarioSession::~BagarioSession() {
    shutdown();
}

void BagarioSession::init() {
    register_components();
    setup_systems();
    m_mass_system->init(m_registry);
    m_food_spawner->init(m_registry);
    m_collision_system->init(m_registry);
    m_bounds_system->init(m_registry);
    m_movement_target_system->init(m_registry);
    m_virus_system->init(m_registry);
    std::cout << "[BagarioSession] Session initialized" << std::endl;
}

void BagarioSession::shutdown() {
    if (m_mass_system)
        m_mass_system->shutdown();
    if (m_food_spawner)
        m_food_spawner->shutdown();
    if (m_collision_system)
        m_collision_system->shutdown();
    if (m_bounds_system)
        m_bounds_system->shutdown();
    if (m_movement_target_system)
        m_movement_target_system->shutdown();
    if (m_virus_system)
        m_virus_system->shutdown();
    m_player_cells.clear();
    m_player_names.clear();
    m_player_colors.clear();
}

void BagarioSession::update(float dt) {
    m_movement_target_system->update(m_registry, dt);
    auto& positions = m_registry.get_components<Position>();
    auto& velocities = m_registry.get_components<Velocity>();
    for (size_t i = 0; i < velocities.size(); ++i) {
        Entity entity = velocities.get_entity_at(i);
        if (!positions.has_entity(entity))
            continue;
        positions[entity].x += velocities.get_data_at(i).x * dt;
        positions[entity].y += velocities.get_data_at(i).y * dt;
    }
    m_mass_system->update(m_registry, dt);
    m_collision_system->update(m_registry, dt);
    m_bounds_system->update(m_registry, dt);
    m_food_spawner->update(m_registry, dt);
    m_virus_system->update(m_registry, dt);
    // Handle collision events
    for (const auto& event : m_collision_system->get_events())
        handle_collision_event(event);
    // Process virus shoot queue (viruses that were fed enough mass)
    process_virus_shoot_queue();
    // Clean up destroyed entities
    auto& to_destroy = m_registry.get_components<ToDestroy>();
    std::vector<Entity> entities_to_kill;
    for (size_t i = 0; i < to_destroy.size(); ++i)
        entities_to_kill.push_back(to_destroy.get_entity_at(i));
    for (auto entity : entities_to_kill)
        m_registry.kill_entity(entity);
}

void BagarioSession::set_callbacks(const SessionCallbacks& callbacks) {
    m_callbacks = callbacks;
}

size_t BagarioSession::add_player(uint32_t player_id, const std::string& name, uint32_t color) {
    std::uniform_real_distribution<float> dist_x(config::MAP_WIDTH * 0.1f, config::MAP_WIDTH * 0.9f);
    std::uniform_real_distribution<float> dist_y(config::MAP_HEIGHT * 0.1f, config::MAP_HEIGHT * 0.9f);
    float x = dist_x(m_rng);
    float y = dist_y(m_rng);
    auto entity = spawn_player_cell(player_id, name, color, x, y, config::STARTING_MASS);
    m_player_names[player_id] = name;
    m_player_colors[player_id] = color;

    std::cout << "[BagarioSession] Player " << player_id << " spawned at ("
              << x << ", " << y << ")" << std::endl;
    return entity;
}

void BagarioSession::remove_player(uint32_t player_id) {
    auto it = m_player_cells.find(player_id);

    if (it == m_player_cells.end())
        return;
    auto& positions = m_registry.get_components<Position>();
    for (size_t entity : it->second)
        if (positions.has_entity(entity))
            m_registry.add_component<ToDestroy>(entity, ToDestroy{});
    m_player_cells.erase(it);
    m_player_names.erase(player_id);
    m_player_colors.erase(player_id);
    std::cout << "[BagarioSession] Player " << player_id << " removed" << std::endl;
}

bool BagarioSession::has_player(uint32_t player_id) const {
    return m_player_cells.find(player_id) != m_player_cells.end();
}

void BagarioSession::set_player_target(uint32_t player_id, float target_x, float target_y) {
    auto it = m_player_cells.find(player_id);

    if (it == m_player_cells.end())
        return;
    auto& targets = m_registry.get_components<components::MovementTarget>();
    for (size_t entity : it->second) {
        if (targets.has_entity(entity)) {
            targets[entity].target_x = target_x;
            targets[entity].target_y = target_y;
        }
    }
}

void BagarioSession::player_split(uint32_t player_id) {
    auto it = m_player_cells.find(player_id);

    if (it == m_player_cells.end())
        return;
    auto& masses = m_registry.get_components<components::Mass>();
    auto& positions = m_registry.get_components<Position>();
    auto& targets = m_registry.get_components<components::MovementTarget>();
    std::vector<size_t> cells_to_split;
    for (size_t entity : it->second) {
        if (!masses.has_entity(entity))
            continue;
        if (masses[entity].value >= config::MIN_SPLIT_MASS &&
            it->second.size() + cells_to_split.size() < config::MAX_CELLS_PER_PLAYER) {
            cells_to_split.push_back(entity);
        }
    }
    auto& merge_timers = m_registry.get_components<components::MergeTimer>();
    for (size_t entity : cells_to_split) {
        float original_mass = masses[entity].value;
        float new_mass = original_mass * config::SPLIT_LOSS_FACTOR;
        masses[entity].value = new_mass;
        float dir_x = 1.0f, dir_y = 0.0f;
        float target_x = positions[entity].x;
        float target_y = positions[entity].y;
        if (targets.has_entity(entity) && positions.has_entity(entity)) {
            target_x = targets[entity].target_x;
            target_y = targets[entity].target_y;
            float dx = target_x - positions[entity].x;
            float dy = target_y - positions[entity].y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 0.001f) {
                dir_x = dx / dist;
                dir_y = dy / dist;
            }
        }
        float cell_radius = config::mass_to_radius(new_mass);
        float spawn_x = positions[entity].x + dir_x * (cell_radius * 2.0f);
        float spawn_y = positions[entity].y + dir_y * (cell_radius * 2.0f);
        auto new_entity = spawn_player_cell(
            player_id,
            m_player_names[player_id],
            m_player_colors[player_id],
            spawn_x, spawn_y,
            new_mass
        );
        if (targets.has_entity(new_entity)) {
            targets[new_entity].target_x = target_x;
            targets[new_entity].target_y = target_y;
        }
        auto& split_vels = m_registry.get_components<components::SplitVelocity>();
        if (!split_vels.has_entity(new_entity))
            m_registry.add_component<components::SplitVelocity>(new_entity, components::SplitVelocity{});
        split_vels[new_entity].vx = dir_x * config::SPLIT_SPEED_BOOST;
        split_vels[new_entity].vy = dir_y * config::SPLIT_SPEED_BOOST;
        split_vels[new_entity].decay_rate = config::SPLIT_DECAY_RATE;
        float merge_time = config::get_merge_time(new_mass);
        if (!merge_timers.has_entity(entity))
            m_registry.add_component<components::MergeTimer>(entity, components::MergeTimer{merge_time, false});
        else {
            merge_timers[entity].time_remaining = merge_time;
            merge_timers[entity].can_merge = false;
        }
        if (!merge_timers.has_entity(new_entity))
            m_registry.add_component<components::MergeTimer>(new_entity, components::MergeTimer{merge_time, false});
    }
}

void BagarioSession::player_eject_mass(uint32_t player_id, float dir_x, float dir_y) {
    auto it = m_player_cells.find(player_id);

    if (it == m_player_cells.end())
        return;
    float len = std::sqrt(dir_x * dir_x + dir_y * dir_y);
    if (len < 0.001f) {
        dir_x = 1.0f;
        dir_y = 0.0f;
    } else {
        dir_x /= len;
        dir_y /= len;
    }
    auto& masses = m_registry.get_components<components::Mass>();
    auto& positions = m_registry.get_components<Position>();
    for (size_t entity : it->second) {
        if (!masses.has_entity(entity) || !positions.has_entity(entity))
            continue;
        if (masses[entity].value < config::MIN_EJECT_MASS)
            continue;
        masses[entity].value -= config::EJECT_MASS_COST;
        float cell_radius = config::mass_to_radius(masses[entity].value);
        float spawn_x = positions[entity].x + dir_x * (cell_radius + 20.0f);
        float spawn_y = positions[entity].y + dir_y * (cell_radius + 20.0f);
        auto ejected = m_registry.spawn_entity();
        m_registry.add_component<Position>(ejected, Position{spawn_x, spawn_y});
        m_registry.add_component<Velocity>(ejected, Velocity{
            dir_x * config::EJECT_SPEED,
            dir_y * config::EJECT_SPEED
        });
        m_registry.add_component<components::Mass>(ejected, components::Mass{config::EJECT_MASS_VALUE});
        float radius = config::mass_to_radius(config::EJECT_MASS_VALUE);
        m_registry.add_component<components::CircleCollider>(ejected, components::CircleCollider{radius});
        m_registry.add_component<components::EjectedMass>(ejected, components::EjectedMass{
            config::EJECT_DECAY_TIME,
            player_id
        });
        uint32_t net_id = get_next_network_id();
        m_registry.add_component<components::NetworkId>(ejected, components::NetworkId{net_id});
        if (m_callbacks.on_entity_spawn) {
            protocol::ServerEntitySpawnPayload payload;
            payload.entity_id = net_id;
            payload.entity_type = protocol::EntityType::EJECTED_MASS;
            payload.spawn_x = spawn_x;
            payload.spawn_y = spawn_y;
            payload.mass = config::EJECT_MASS_VALUE;
            payload.color = m_player_colors[player_id];
            payload.owner_id = player_id;
            m_callbacks.on_entity_spawn(payload);
        }
    }
}

std::vector<protocol::EntityState> BagarioSession::get_snapshot() const {
    std::vector<protocol::EntityState> states;
    auto& registry = const_cast<Registry&>(m_registry);
    auto& positions = registry.get_components<Position>();
    auto& masses = registry.get_components<components::Mass>();
    auto& network_ids = registry.get_components<components::NetworkId>();
    auto& player_cells = registry.get_components<components::PlayerCell>();
    auto& owners = registry.get_components<components::CellOwner>();
    auto& foods = registry.get_components<components::Food>();
    auto& ejected = registry.get_components<components::EjectedMass>();
    auto& viruses = registry.get_components<components::Virus>();

    for (size_t i = 0; i < positions.size(); ++i) {
        Entity entity = positions.get_entity_at(i);
        if (!masses.has_entity(entity) || !network_ids.has_entity(entity))
            continue;
        protocol::EntityState state;
        state.entity_id = network_ids[entity].id;
        state.position_x = positions.get_data_at(i).x;
        state.position_y = positions.get_data_at(i).y;
        state.mass = masses[entity].value;
        if (player_cells.has_entity(entity)) {
            state.entity_type = protocol::EntityType::PLAYER_CELL;
            state.owner_id = player_cells[entity].player_id;
            state.color = player_cells[entity].color;
        } else if (owners.has_entity(entity)) {
            state.entity_type = protocol::EntityType::PLAYER_CELL;
            state.owner_id = owners[entity].owner_id;
            auto color_it = m_player_colors.find(owners[entity].owner_id);
            state.color = color_it != m_player_colors.end() ? color_it->second : 0xFFFFFFFF;
        } else if (ejected.has_entity(entity)) {
            state.entity_type = protocol::EntityType::EJECTED_MASS;
            state.owner_id = ejected[entity].original_owner;
            auto color_it = m_player_colors.find(ejected[entity].original_owner);
            state.color = color_it != m_player_colors.end() ? color_it->second : 0xFFFFFFFF;
        } else if (viruses.has_entity(entity)) {
            state.entity_type = protocol::EntityType::VIRUS;
            state.owner_id = 0;
            state.color = 0x00C800FF;  // Green
        } else if (foods.has_entity(entity)) {
            state.entity_type = protocol::EntityType::FOOD;
            state.owner_id = 0;
            state.color = foods[entity].color;
        } else {
            continue;
        }
        states.push_back(state);
    }
    return states;
}

std::vector<protocol::LeaderboardEntry> BagarioSession::get_leaderboard() const {
    std::vector<std::pair<uint32_t, float>> player_masses;

    for (const auto& [player_id, cells] : m_player_cells) {
        float total_mass = get_player_total_mass(player_id);
        player_masses.emplace_back(player_id, total_mass);
    }
    std::sort(player_masses.begin(), player_masses.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    std::vector<protocol::LeaderboardEntry> entries;
    size_t count = std::min(player_masses.size(), static_cast<size_t>(config::LEADERBOARD_SIZE));
    for (size_t i = 0; i < count; ++i) {
        protocol::LeaderboardEntry entry;
        entry.player_id = player_masses[i].first;
        entry.total_mass = player_masses[i].second;
        auto name_it = m_player_names.find(entry.player_id);
        if (name_it != m_player_names.end())
            entry.set_name(name_it->second);
        entries.push_back(entry);
    }
    return entries;
}

float BagarioSession::get_player_total_mass(uint32_t player_id) const {
    auto it = m_player_cells.find(player_id);

    if (it == m_player_cells.end())
        return 0.0f;
    auto& registry = const_cast<Registry&>(m_registry);
    auto& masses = registry.get_components<components::Mass>();
    float total = 0.0f;
    for (size_t entity : it->second)
        if (masses.has_entity(entity))
            total += masses[entity].value;
    return total;
}

size_t BagarioSession::get_player_cell_count(uint32_t player_id) const {
    auto it = m_player_cells.find(player_id);

    return it != m_player_cells.end() ? it->second.size() : 0;
}

void BagarioSession::register_components() {
    m_registry.register_component<Position>();
    m_registry.register_component<Velocity>();
    m_registry.register_component<ToDestroy>();
    m_registry.register_component<components::Mass>();
    m_registry.register_component<components::CircleCollider>();
    m_registry.register_component<components::PlayerCell>();
    m_registry.register_component<components::CellOwner>();
    m_registry.register_component<components::Food>();
    m_registry.register_component<components::Virus>();
    m_registry.register_component<components::EjectedMass>();
    m_registry.register_component<components::MovementTarget>();
    m_registry.register_component<components::MergeTimer>();
    m_registry.register_component<components::SplitVelocity>();
    m_registry.register_component<components::NetworkId>();
}

void BagarioSession::setup_systems() {
    m_mass_system = std::make_unique<systems::MassSystem>();
    m_food_spawner = std::make_unique<systems::FoodSpawnerSystem>();
    m_collision_system = std::make_unique<systems::BagarioCollisionSystem>();
    m_bounds_system = std::make_unique<systems::MapBoundsSystem>();
    m_movement_target_system = std::make_unique<systems::MovementTargetSystem>();
    m_virus_system = std::make_unique<systems::VirusSystem>();

    // Share network ID generator with spawners to avoid ID conflicts
    m_food_spawner->set_network_id_generator([this]() {
        return get_next_network_id();
    });
    m_virus_system->set_network_id_generator([this]() {
        return get_next_network_id();
    });

    // Virus spawn callback
    m_virus_system->set_spawn_callback([this](uint32_t net_id, float x, float y, float mass) {
        if (m_callbacks.on_entity_spawn) {
            protocol::ServerEntitySpawnPayload payload;
            payload.entity_id = net_id;
            payload.entity_type = protocol::EntityType::VIRUS;
            payload.spawn_x = x;
            payload.spawn_y = y;
            payload.mass = mass;
            payload.color = 0x00C800FF;  // Green
            payload.owner_id = 0;
            m_callbacks.on_entity_spawn(payload);
        }
    });

    m_collision_system->set_collision_callback([this](const systems::CollisionEvent& event) {
        handle_collision_event(event);
    });
}

void BagarioSession::handle_collision_event(const systems::CollisionEvent& event) {
    if (event.type == systems::CollisionEvent::Type::CELL_ATE_CELL) {
        if (event.eaten_player_id != 0) {
            auto it = m_player_cells.find(event.eaten_player_id);
            if (it != m_player_cells.end()) {
                auto& cells = it->second;
                cells.erase(
                    std::remove(cells.begin(), cells.end(), event.eaten_entity),
                    cells.end()
                );
                if (cells.empty()) {
                    if (m_callbacks.on_player_eliminated)
                        m_callbacks.on_player_eliminated(event.eaten_player_id, event.eater_player_id);
                    m_player_cells.erase(it);
                }
            }
        }
    } else if (event.type == systems::CollisionEvent::Type::CELL_MERGED) {
        // Same player cells merged - remove the eaten cell from tracking
        if (event.eaten_player_id != 0) {
            auto it = m_player_cells.find(event.eaten_player_id);
            if (it != m_player_cells.end()) {
                auto& cells = it->second;
                cells.erase(
                    std::remove(cells.begin(), cells.end(), event.eaten_entity),
                    cells.end()
                );
            }
        }
    } else if (event.type == systems::CollisionEvent::Type::CELL_HIT_VIRUS) {
        // Cell hit a virus - split into multiple pieces
        bool did_split = handle_virus_split(event.eater_player_id, event.eater_entity);
        // Only destroy the virus if the player actually split
        if (did_split) {
            m_registry.add_component<ToDestroy>(event.eaten_entity, ToDestroy{});
        }
    }
    if (m_callbacks.on_entity_destroy) {
        auto& network_ids = m_registry.get_components<components::NetworkId>();
        auto& positions = m_registry.get_components<Position>();
        if (network_ids.has_entity(event.eaten_entity)) {
            protocol::ServerEntityDestroyPayload payload;
            payload.entity_id = network_ids[event.eaten_entity].id;
            payload.reason = protocol::DestroyReason::EATEN;
            payload.killer_id = event.eater_player_id;
            if (positions.has_entity(event.eaten_entity)) {
                payload.position_x = positions[event.eaten_entity].x;
                payload.position_y = positions[event.eaten_entity].y;
            }
            m_callbacks.on_entity_destroy(payload);
        }
    }
}

bool BagarioSession::handle_virus_split(uint32_t player_id, size_t cell_entity) {
    auto& masses = m_registry.get_components<components::Mass>();
    auto& positions = m_registry.get_components<Position>();
    auto& targets = m_registry.get_components<components::MovementTarget>();

    if (!masses.has_entity(cell_entity) || !positions.has_entity(cell_entity))
        return false;

    auto it = m_player_cells.find(player_id);
    if (it == m_player_cells.end())
        return false;

    float original_mass = masses[cell_entity].value;
    float cell_x = positions[cell_entity].x;
    float cell_y = positions[cell_entity].y;

    // Calculate how many pieces to split into (up to VIRUS_SPLIT_COUNT, limited by max cells)
    int current_cells = static_cast<int>(it->second.size());
    int max_new_cells = config::MAX_CELLS_PER_PLAYER - current_cells;
    int split_count = std::min(config::VIRUS_SPLIT_COUNT - 1, max_new_cells);

    if (split_count <= 0)
        return false;  // Can't split, already at max cells

    // Divide mass among all new cells + original
    float mass_per_cell = original_mass / (split_count + 1);
    masses[cell_entity].value = mass_per_cell;

    // Get target direction for the main split direction
    float target_x = cell_x, target_y = cell_y;
    if (targets.has_entity(cell_entity)) {
        target_x = targets[cell_entity].target_x;
        target_y = targets[cell_entity].target_y;
    }

    auto& merge_timers = m_registry.get_components<components::MergeTimer>();
    float merge_time = config::get_merge_time(mass_per_cell);

    // Add merge timer to original cell
    if (!merge_timers.has_entity(cell_entity))
        m_registry.add_component<components::MergeTimer>(cell_entity, components::MergeTimer{merge_time, false});
    else {
        merge_timers[cell_entity].time_remaining = merge_time;
        merge_timers[cell_entity].can_merge = false;
    }

    // Spawn split cells in different directions (radial pattern)
    constexpr float PI = 3.14159265358979323846f;
    for (int i = 0; i < split_count; ++i) {
        float angle = (2.0f * PI * i) / split_count;
        float dir_x = std::cos(angle);
        float dir_y = std::sin(angle);

        float cell_radius = config::mass_to_radius(mass_per_cell);
        float spawn_x = cell_x + dir_x * (cell_radius * 2.0f);
        float spawn_y = cell_y + dir_y * (cell_radius * 2.0f);

        auto new_entity = spawn_player_cell(
            player_id,
            m_player_names[player_id],
            m_player_colors[player_id],
            spawn_x, spawn_y,
            mass_per_cell
        );

        // Set target for new cell
        if (targets.has_entity(new_entity)) {
            targets[new_entity].target_x = target_x;
            targets[new_entity].target_y = target_y;
        }

        // Add split velocity
        auto& split_vels = m_registry.get_components<components::SplitVelocity>();
        if (!split_vels.has_entity(new_entity))
            m_registry.add_component<components::SplitVelocity>(new_entity, components::SplitVelocity{});
        split_vels[new_entity].vx = dir_x * config::SPLIT_SPEED_BOOST;
        split_vels[new_entity].vy = dir_y * config::SPLIT_SPEED_BOOST;
        split_vels[new_entity].decay_rate = config::SPLIT_DECAY_RATE;

        // Add merge timer
        if (!merge_timers.has_entity(new_entity))
            m_registry.add_component<components::MergeTimer>(new_entity, components::MergeTimer{merge_time, false});
    }
    return true;  // Split succeeded
}

void BagarioSession::process_virus_shoot_queue() {
    auto& positions = m_registry.get_components<Position>();
    auto& colliders = m_registry.get_components<components::CircleCollider>();

    for (const auto& request : m_collision_system->get_virus_shoot_queue()) {
        if (!positions.has_entity(request.virus_entity) || !colliders.has_entity(request.virus_entity))
            continue;

        float virus_x = positions[request.virus_entity].x;
        float virus_y = positions[request.virus_entity].y;
        float virus_radius = colliders[request.virus_entity].radius;

        // Spawn new virus in the direction
        float spawn_x = virus_x + request.dir_x * (virus_radius * 2.0f);
        float spawn_y = virus_y + request.dir_y * (virus_radius * 2.0f);

        m_virus_system->shoot_virus(m_registry, spawn_x, spawn_y, request.dir_x, request.dir_y);
    }
}

size_t BagarioSession::spawn_player_cell(uint32_t player_id, const std::string& name,
                                         uint32_t color, float x, float y, float mass) {
    auto entity = m_registry.spawn_entity();

    m_registry.add_component<Position>(entity, Position{x, y});
    m_registry.add_component<Velocity>(entity, Velocity{0, 0});
    m_registry.add_component<components::Mass>(entity, components::Mass{mass});
    float radius = config::mass_to_radius(mass);
    m_registry.add_component<components::CircleCollider>(entity, components::CircleCollider{radius});
    components::PlayerCell cell;
    cell.player_id = player_id;
    cell.color = color;
    cell.name = name;
    m_registry.add_component<components::PlayerCell>(entity, std::move(cell));
    m_registry.add_component<components::MovementTarget>(entity,
        components::MovementTarget{x, y});
    uint32_t net_id = get_next_network_id();
    m_registry.add_component<components::NetworkId>(entity, components::NetworkId{net_id});
    m_player_cells[player_id].push_back(entity);
    if (m_callbacks.on_entity_spawn) {
        protocol::ServerEntitySpawnPayload payload;
        payload.entity_id = net_id;
        payload.entity_type = protocol::EntityType::PLAYER_CELL;
        payload.spawn_x = x;
        payload.spawn_y = y;
        payload.mass = mass;
        payload.color = color;
        payload.owner_id = player_id;
        std::strncpy(payload.owner_name, name.c_str(), sizeof(payload.owner_name) - 1);
        m_callbacks.on_entity_spawn(payload);
    }
    return entity;
}

uint32_t BagarioSession::get_next_network_id() {
    return m_next_network_id++;
}

}
