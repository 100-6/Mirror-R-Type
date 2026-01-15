/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** CollisionSystem
*/

#ifndef COLLISIONSYSTEM_HPP_
#define COLLISIONSYSTEM_HPP_
#include "ecs/Registry.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/systems/ISystem.hpp"

class CollisionSystem : public ISystem {
    private:
        bool check_collision(const Position& pos1, const Position& pos2,
            const Collider& col1, const Collider& col2);
        void handle_projectiles_colisions(Registry& registry);

        // Scroll offset for world<->screen coordinate conversion
        // Walls are in WORLD coordinates, players/projectiles in SCREEN coordinates
        float m_currentScroll = 0.0f;
    public:
        virtual ~CollisionSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;

        /**
         * @brief Set the current scroll position for coordinate conversion
         * @param scroll Current scroll X offset (world coordinates)
         *
         * Must be called before update() to ensure proper collision detection
         * between screen-space entities (players, projectiles) and world-space walls
         */
        void setScroll(float scroll) { m_currentScroll = scroll; }
        
        template<typename TypeA, typename TypeB, typename Action>
        void scan_collisions(Registry& registry, Action action)
        {
            auto& positions = registry.get_components<Position>();
            auto& colliders = registry.get_components<Collider>();
            auto& typeA = registry.get_components<TypeA>();
            auto& typeB = registry.get_components<TypeB>();

            for (size_t i = 0; i < typeA.size(); i++)
            {
                Entity entity_A = typeA.get_entity_at(i);

                if (!positions.has_entity(entity_A) || !colliders.has_entity(entity_A))
                    continue;

                const Position& posA = positions.get_data_by_entity_id(entity_A);
                const Collider& colA = colliders.get_data_by_entity_id(entity_A);

                for (size_t j = 0; j < typeB.size(); j++)
                {
                    Entity entity_B = typeB.get_entity_at(j);

                    if (!positions.has_entity(entity_B) || !colliders.has_entity(entity_B))
                        continue;

                    if (entity_A == entity_B)
                        continue;

                    const Position& posB = positions.get_data_by_entity_id(entity_B);
                    const Collider& colB = colliders.get_data_by_entity_id(entity_B);

                    if (check_collision(posA, posB, colA, colB))
                        action(entity_A, entity_B);
                }
            }
        }
};

#endif /* !COLLISIONSYSTEM_HPP_ */
