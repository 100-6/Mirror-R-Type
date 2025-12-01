/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** RaylibInputPlugin - Raylib implementation of input plugin
*/

#ifndef RAYLIBINPUTPLUGIN_HPP_
#define RAYLIBINPUTPLUGIN_HPP_

#include "plugin_manager/IInputPlugin.hpp"
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include <raylib.h>
#include <unordered_map>

/**
 * @brief Implémentation du plugin d'input utilisant Raylib
 * 
 * Ce plugin lit les entrées clavier/souris via Raylib et met à jour
 * les composants Input des entités dans le Registry.
 */
class RaylibInputPlugin : public engine::IInputPlugin {
    private:
        // Mappage des touches engine::Key vers Raylib KeyboardKey
        std::unordered_map<engine::Key, int> key_mapping;
        
        // État des touches (pour détecter just_pressed/just_released)
        std::unordered_map<engine::Key, bool> previous_key_state;
        std::unordered_map<engine::MouseButton, bool> previous_mouse_state;
        
        // État d'initialisation
        bool initialized = false;

        /**
         * @brief Initialise le mappage des touches
         */
        void init_key_mapping();

        /**
         * @brief Convertit une touche engine::Key en KeyboardKey Raylib
         */
        int to_raylib_key(engine::Key key) const;

        /**
         * @brief Convertit un bouton souris en code Raylib
         */
        int to_raylib_mouse_button(engine::MouseButton button) const;

    public:
        RaylibInputPlugin();
        virtual ~RaylibInputPlugin() = default;

        // Implémentation de IPlugin
        bool initialize() override;
        void shutdown() override;
        bool is_initialized() const override { return initialized; }
        const char* get_name() const override { return "RaylibInputPlugin"; }
        const char* get_version() const override { return "1.0.0"; }

        // Implémentadztion de IInputPlugin - Keyboard
        bool is_key_pressed(engine::Key key) const override;
        bool is_key_just_pressed(engine::Key key) const override;
        bool is_key_just_released(engine::Key key) const override;

        // Implémentation de IInputPlugin - Mouse
        bool is_mouse_button_pressed(engine::MouseButton button) const override;
        bool is_mouse_button_just_pressed(engine::MouseButton button) const override;
        bool is_mouse_button_just_released(engine::MouseButton button) const override;
        engine::Vector2f get_mouse_position() const override;
        float get_mouse_wheel_delta() const override;

        // Implémentation de IInputPlugin - Gamepad
        bool is_gamepad_connected(int gamepad_id) const override;
        bool is_gamepad_button_pressed(int gamepad_id, int button) const override;
        float get_gamepad_axis(int gamepad_id, int axis) const override;

        // Update
        void update() override;
};

#endif /* !RAYLIBINPUTPLUGIN_HPP_ */
