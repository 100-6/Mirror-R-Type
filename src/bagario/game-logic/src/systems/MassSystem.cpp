#include "systems/MassSystem.hpp"

namespace bagario::systems {

void MassSystem::init(Registry& registry) {
}

void MassSystem::update(Registry& registry, float dt) {
    auto& masses = registry.get_components<components::Mass>();
    auto& colliders = registry.get_components<components::CircleCollider>();

    for (size_t i = 0; i < masses.size(); ++i) {
        Entity entity = masses.get_entity_at(i);
        auto& mass = masses.get_data_at(i);
        if (colliders.has_entity(entity)) {
            auto& collider = colliders[entity];
            collider.radius = config::mass_to_radius(mass.value);
        }
        if (mass.value > config::MASS_DECAY_THRESHOLD) {
            float decay = mass.value * config::MASS_DECAY_RATE * dt;
            mass.value = std::max(config::MIN_MASS, mass.value - decay);
        }
    }
}

void MassSystem::shutdown() {
}

}
