/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** SystemMouvement
*/

#ifndef SYSTEMMOUVEMENT_HPP_
#define SYSTEMMOUVEMENT_HPP_

// Inclusions nécessaires
#include "ISystem.hpp"   // L'interface de base (avec update(Registry&))
#include "Registry.hpp"  // Pour la classe Registry
#include "Component.hpp" // Pour les composants PositionComponent et InputComponent
#include "Registry.hpp"  // Pour la classe Registry
#include <cmath>         // Pour std::sqrt
#include <iostream>      // Pour les messages de debug ou init

// L'alias Entity est déjà défini dans Registry.hpp, mais nous le redéfinissons ici
// au cas où ce fichier est inclus seul ou pour la clarté.
using Entity = size_t; 

class SystemMouvement : public ISystem {
public:
    virtual ~SystemMouvement() = default;

    // init/shutdown sont vides pour ce système de logique pure.
    void init(Registry& registry) override {
        std::cout << "MovementSystem: Initialisation." << std::endl;
    }
    void shutdown() override {
        std::cout << "MovementSystem: Arrêt." << std::endl;
    }

    // La fonction de logique principale
    void update(Registry& registry) override;
};

// --- 3. IMPLÉMENTATION DE LA LOGIQUE UPDATE ---

void SystemMouvement::update(Registry& registry)
{
    auto& inputs = registry.get_components<InputComponent>();
    auto& positions = registry.get_components<PositionComponent>();

    // Itération sur les entités ayant un InputComponent
    for (size_t i = 0; i < inputs.size(); ++i) {
        Entity entity = inputs.get_entity_at(i);

        // L'entité doit avoir un PositionComponent pour bouger
        if (!positions.has_entity(entity)) {
            continue;
        }

        auto& input = inputs.get_data_at(i); 
        auto& position = positions.get_data_by_entity_id(entity);

        // --- DÉPLACEMENT DISCRET PAR CASE (SIMPLE) ---

        // Note : On suppose qu'un seul déplacement est traité par cycle

        if (input.up) {
            position.y -= 1; // Monter d'une case (diminution des Y)
        } else if (input.down) {
            position.y += 1; // Descendre d'une case
        } 
        
        if (input.left) {
            position.x -= 1; // Aller à gauche d'une case
        } else if (input.right) {
            position.x += 1; // Aller à droite d'une case
        }

        // IMPORTANT : Réinitialiser l'Input après le déplacement, 
        // sinon l'entité continuera de bouger chaque frame.
        input.up = input.down = input.left = input.right = false;
    }
}


#endif /* !SYSTEMMOUVEMENT_HPP_ */