#include "systems/FoodSpawnerSystem.hpp"

namespace bagario::systems {

FoodSpawnerSystem::FoodSpawnerSystem()
    : m_rng(std::random_device{}())
    , m_pos_dist_x(0.0f, config::MAP_WIDTH)
    , m_pos_dist_y(0.0f, config::MAP_HEIGHT)
    , m_color_dist(0, 255) {}

void FoodSpawnerSystem::set_spawn_callback(SpawnCallback callback) {
    m_spawn_callback = std::move(callback);
}

void FoodSpawnerSystem::set_network_id_generator(NetworkIdGenerator generator) {
    m_network_id_generator = std::move(generator);
}

uint32_t FoodSpawnerSystem::get_next_network_id() {
    if (m_network_id_generator) {
        return m_network_id_generator();
    }
    // Fallback to internal counter starting at high value
    return m_fallback_network_id++;
}

void FoodSpawnerSystem::init(Registry& registry) {
    // Spawn only a small initial batch to avoid network spike
    spawn_food_batch(registry, config::INITIAL_FOOD);
}

void FoodSpawnerSystem::update(Registry& registry, float dt) {
    m_spawn_timer += dt;
    auto& foods = registry.get_components<components::Food>();
    int food_count = static_cast<int>(foods.size());

    if (food_count >= config::MAX_FOOD) {
        m_ramp_up_complete = true;
        return;
    }

    // Two spawn modes: ramp-up (fast) and normal (slow replacement)
    if (!m_ramp_up_complete) {
        // Ramp-up mode: spawn batches at faster intervals until we reach MAX_FOOD
        if (m_spawn_timer >= config::FOOD_SPAWN_INTERVAL) {
            int to_spawn = std::min(
                config::FOOD_SPAWN_BATCH,
                config::MAX_FOOD - food_count
            );
            spawn_food_batch(registry, to_spawn);
            m_spawn_timer = 0.0f;
        }
    } else {
        // Normal mode: slow spawn rate to replace eaten food
        float normal_interval = 1.0f / config::FOOD_SPAWN_RATE;
        if (m_spawn_timer >= normal_interval) {
            spawn_single_food(registry);
            m_spawn_timer = 0.0f;
        }
    }
}

void FoodSpawnerSystem::shutdown() {
    m_spawn_callback = nullptr;
}

void FoodSpawnerSystem::spawn_food_batch(Registry& registry, int count) {
    for (int i = 0; i < count; ++i)
        spawn_single_food(registry);
}

void FoodSpawnerSystem::spawn_single_food(Registry& registry) {
    auto entity = registry.spawn_entity();
    float x = m_pos_dist_x(m_rng);
    float y = m_pos_dist_y(m_rng);
    uint32_t color = generate_random_color();

    registry.add_component<Position>(entity, Position{x, y});
    registry.add_component<Velocity>(entity, Velocity{0, 0});
    registry.add_component<components::Mass>(entity, components::Mass{config::FOOD_MASS});
    float radius = config::mass_to_radius(config::FOOD_MASS);
    registry.add_component<components::CircleCollider>(entity, components::CircleCollider{radius});
    registry.add_component<components::Food>(entity, components::Food{config::FOOD_MASS, color});
    registry.add_component<components::NetworkId>(entity, components::NetworkId{get_next_network_id()});
    if (m_spawn_callback)
        m_spawn_callback(entity, x, y, color);
}

uint32_t FoodSpawnerSystem::generate_random_color() {
    uint8_t r = static_cast<uint8_t>(m_color_dist(m_rng));
    uint8_t g = static_cast<uint8_t>(m_color_dist(m_rng));
    uint8_t b = static_cast<uint8_t>(m_color_dist(m_rng));

    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

}
