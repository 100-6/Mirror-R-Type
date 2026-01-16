#include "systems/VirusSystem.hpp"
#include <iostream>

namespace bagario::systems {

VirusSystem::VirusSystem()
    : m_rng(std::random_device{}())
    , m_pos_dist_x(config::MAP_WIDTH * 0.1f, config::MAP_WIDTH * 0.9f)
    , m_pos_dist_y(config::MAP_HEIGHT * 0.1f, config::MAP_HEIGHT * 0.9f)
{
}

void VirusSystem::set_network_id_generator(NetworkIdGenerator generator) {
    m_network_id_generator = std::move(generator);
}

void VirusSystem::set_spawn_callback(SpawnCallback callback) {
    m_spawn_callback = std::move(callback);
}

void VirusSystem::set_destroy_callback(DestroyCallback callback) {
    m_destroy_callback = std::move(callback);
}

void VirusSystem::init(Registry& registry) {
    spawn_initial_viruses(registry);
    std::cout << "[VirusSystem] Initialized with " << m_virus_count << " viruses" << std::endl;
}

void VirusSystem::update(Registry& registry, float dt) {
    // Count current viruses and update absorption animations
    auto& viruses = registry.get_components<components::Virus>();
    auto& colliders = registry.get_components<components::CircleCollider>();
    auto& masses = registry.get_components<components::Mass>();
    auto& velocities = registry.get_components<Velocity>();
    m_virus_count = static_cast<int>(viruses.size());

    // Update virus absorption animations and moving virus friction
    for (size_t i = 0; i < viruses.size(); ++i) {
        Entity entity = viruses.get_entity_at(i);
        auto& virus = viruses[entity];

        // Handle moving virus friction (shot viruses)
        if (virus.is_moving && velocities.has_entity(entity)) {
            auto& vel = velocities[entity];
            float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);

            // Apply friction to slow down the virus
            float friction = config::VIRUS_FRICTION * dt;

            if (speed > friction) {
                float factor = (speed - friction) / speed;
                vel.x *= factor;
                vel.y *= factor;
            } else {
                // Virus has stopped
                vel.x = 0.0f;
                vel.y = 0.0f;
                virus.is_moving = false;
            }
        }

        if (virus.absorption_timer > 0.0f) {
            virus.absorption_timer -= dt;
            if (virus.absorption_timer <= 0.0f) {
                // Animation finished, reset scale
                virus.absorption_timer = 0.0f;
                virus.absorption_scale = 1.0f;
            }
            // Update collider radius AND mass with scale (visual effect sent to client)
            float scaled_mass = config::VIRUS_MASS * virus.absorption_scale * virus.absorption_scale;
            if (colliders.has_entity(entity)) {
                colliders[entity].radius = config::mass_to_radius(scaled_mass);
            }
            if (masses.has_entity(entity)) {
                masses[entity].value = scaled_mass;
            }
        } else {
            // Ensure mass is reset to base when not animating
            if (masses.has_entity(entity) && masses[entity].value != config::VIRUS_MASS) {
                masses[entity].value = config::VIRUS_MASS;
                if (colliders.has_entity(entity)) {
                    colliders[entity].radius = config::mass_to_radius(config::VIRUS_MASS);
                }
            }
        }
    }

    // Spawn more viruses if below max (slowly)
    if (m_virus_count < config::MAX_VIRUSES) {
        // Respawn viruses slowly over time
        static float respawn_timer = 0.0f;
        respawn_timer += dt;
        if (respawn_timer >= 5.0f) {  // Spawn a new virus every 5 seconds if needed
            respawn_timer = 0.0f;
            spawn_virus(registry, m_pos_dist_x(m_rng), m_pos_dist_y(m_rng));
        }
    }
}

void VirusSystem::shutdown() {
    m_virus_count = 0;
}

void VirusSystem::spawn_initial_viruses(Registry& registry) {
    for (int i = 0; i < config::INITIAL_VIRUSES; ++i) {
        float x = m_pos_dist_x(m_rng);
        float y = m_pos_dist_y(m_rng);
        spawn_virus(registry, x, y);
    }
}

size_t VirusSystem::spawn_virus(Registry& registry, float x, float y) {
    auto entity = registry.spawn_entity();

    registry.add_component<Position>(entity, Position{x, y});
    registry.add_component<Velocity>(entity, Velocity{0, 0});
    registry.add_component<components::Mass>(entity, components::Mass{config::VIRUS_MASS});
    float radius = config::mass_to_radius(config::VIRUS_MASS);
    registry.add_component<components::CircleCollider>(entity, components::CircleCollider{radius});
    registry.add_component<components::Virus>(entity, components::Virus{});

    uint32_t net_id = get_next_network_id();
    registry.add_component<components::NetworkId>(entity, components::NetworkId{net_id});

    if (m_spawn_callback) {
        m_spawn_callback(net_id, x, y, config::VIRUS_MASS);
    }

    m_virus_count++;
    return entity;
}

size_t VirusSystem::shoot_virus(Registry& registry, float x, float y, float dir_x, float dir_y) {
    auto entity = registry.spawn_entity();

    registry.add_component<Position>(entity, Position{x, y});
    // Set initial velocity - will be decayed by VirusSystem::update
    registry.add_component<Velocity>(entity, Velocity{
        dir_x * config::VIRUS_SHOOT_SPEED,
        dir_y * config::VIRUS_SHOOT_SPEED
    });
    registry.add_component<components::Mass>(entity, components::Mass{config::VIRUS_SHOOT_MASS});
    float radius = config::mass_to_radius(config::VIRUS_SHOOT_MASS);
    registry.add_component<components::CircleCollider>(entity, components::CircleCollider{radius});
    // Mark as moving virus (is_moving = true)
    registry.add_component<components::Virus>(entity, components::Virus{0, 1.0f, 0.0f, true});

    uint32_t net_id = get_next_network_id();
    registry.add_component<components::NetworkId>(entity, components::NetworkId{net_id});

    if (m_spawn_callback) {
        m_spawn_callback(net_id, x, y, config::VIRUS_SHOOT_MASS);
    }

    m_virus_count++;
    return entity;
}

uint32_t VirusSystem::get_next_network_id() {
    if (m_network_id_generator) {
        return m_network_id_generator();
    }
    return m_fallback_network_id++;
}

}