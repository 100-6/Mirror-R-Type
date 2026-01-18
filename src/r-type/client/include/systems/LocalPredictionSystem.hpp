/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LocalPredictionSystem - client-side player extrapolation (screen clamping only)
** Wall collisions are handled server-side only to avoid desync
*/

#pragma once

#include "ecs/systems/ISystem.hpp"
#include "ClientComponents.hpp"
#include "ecs/CoreComponents.hpp"

class Registry;

namespace rtype::client {

class EntityManager;

class LocalPredictionSystem : public ISystem {
public:
    LocalPredictionSystem(EntityManager& entity_manager,
                          float screen_width,
                          float screen_height);
    ~LocalPredictionSystem() override = default;

    void init(Registry& registry) override;
    void shutdown() override;
    void update(Registry& registry, float dt) override;

    void set_current_time(float time_seconds) { current_time_ = time_seconds; }

private:
    EntityManager& entity_manager_;
    float screen_width_;
    float screen_height_;
    float current_time_;
    static constexpr float MAX_EXTRAPOLATION_SECONDS = 0.2f;

    void clamp_position(float& x, float& y, const Collider& collider) const;
};

}

