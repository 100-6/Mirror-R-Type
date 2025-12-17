/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** InputSystem - Gère les entrées utilisateur via un IInputPlugin
*/

#ifndef INPUTSYSTEM_HPP_
#define INPUTSYSTEM_HPP_

#include "ISystem.hpp"
#include "ecs/Registry.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include <memory>

/**
 * @brief Système qui met à jour les composants Input en utilisant un IInputPlugin
 * 
 * Ce système lit les entrées via un plugin d'input (Raylib, SDL, GLFW, etc.)
 * et met à jour les composants Input des entités qui en ont un.
 */
class InputSystem : public ISystem {
    private:
        engine::IInputPlugin& input_plugin;  // Référence vers le plugin (non possédé)

    public:
        /**
         * @brief Constructeur avec un plugin d'input
         * @param plugin Référence vers le plugin d'input à utiliser
         */
        explicit InputSystem(engine::IInputPlugin& plugin);
        virtual ~InputSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;
};

#endif /* !INPUTSYSTEM_HPP_ */
