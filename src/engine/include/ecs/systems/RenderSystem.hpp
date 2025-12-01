/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** RenderSystem - Gère le rendu des entités via un IGraphicsPlugin
*/

#ifndef RENDERSYSTEM_HPP_
#define RENDERSYSTEM_HPP_

#include "ISystem.hpp"
#include "ecs/Registry.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"

/**
 * @brief Système qui affiche les entités en utilisant un IGraphicsPlugin
 *
 * Ce système parcourt toutes les entités ayant Position + Sprite
 * et les affiche via un plugin graphique (Raylib, SDL, SFML, etc.)
 * Les sprites sont triés par layer avant l'affichage pour gérer le z-ordering.
 */
class RenderSystem : public ISystem {
    private:
        engine::IGraphicsPlugin* graphics_plugin;  // Référence vers le plugin (non possédé)
        mutable engine::Sprite temp_sprite;        // Sprite temporaire réutilisé pour éviter allocations

    public:
        /**
         * @brief Constructeur avec un plugin graphique
         * @param plugin Pointeur vers le plugin graphique à utiliser
         */
        explicit RenderSystem(engine::IGraphicsPlugin* plugin);
        virtual ~RenderSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry) override;
};

#endif /* !RENDERSYSTEM_HPP_ */
