/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** DestroySystem
*/

#ifndef DESTROYSYSTEM_HPP_
#define DESTROYSYSTEM_HPP_
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/systems/ISystem.hpp"

class DestroySystem : public ISystem {
    public:
        virtual ~DestroySystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;
};

#endif /* !DESTROYSYSTEM_HPP_ */
