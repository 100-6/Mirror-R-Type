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

class PhysiqueSystem : public ISystem {
    private:
        int SCREEN_WIDTH = 1920; //on mettera un truc du config json plus tard
        int SCREEN_HEIGHT = 1080; //on mettera un truc du config json plus tard

    public:
        virtual ~PhysiqueSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;
};

#endif /* !PHYSIQUESYSTEM_HPP_ */