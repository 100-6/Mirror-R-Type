/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** PhysiqueSystem
*/

#ifndef PHYSIQUESYSTEM_HPP_
    #define PHYSIQUESYSTEM_HPP_
    #include "ISystem.hpp"
    #include "ecs/CoreComponents.hpp"
    #include "ecs/Registry.hpp"
    #include "GameConfig.hpp"

class PhysiqueSystem : public ISystem {
    private:
        static constexpr float SCREEN_WIDTH = rtype::shared::config::SCREEN_WIDTH;
        static constexpr float SCREEN_HEIGHT = rtype::shared::config::SCREEN_HEIGHT;

    public:
        virtual ~PhysiqueSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;
};

#endif /* !PHYSIQUESYSTEM_HPP_ */
