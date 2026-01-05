/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** SpaceshipManager - Gestion des vaisseaux depuis une spritesheet
*/

#pragma once

#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/CommonTypes.hpp"

namespace rtype::client {

/**
 * @brief Énumérations pour identifier les vaisseaux
 */
enum class ShipColor { 
    GREEN = 0, 
    RED = 1, 
    BLUE = 2 
};

enum class ShipType { 
    SCOUT = 0,      // Éclaireur
    FIGHTER = 1,    // Chasseur
    CRUISER = 2,    // Croiseur
    BOMBER = 3,     // Bombardier
    CARRIER = 4     // Porte-avions
};

/**
 * @brief Informations de découpage pour un sprite dans une spritesheet
 */
struct SpriteRect {
    int x;          // Position X dans la spritesheet
    int y;          // Position Y dans la spritesheet
    int width;      // Largeur du sprite
    int height;     // Hauteur du sprite
};

/**
 * @brief Gestionnaire de vaisseaux spatiaux
 * 
 * Charge une spritesheet contenant tous les vaisseaux et permet
 * de créer des sprites pour chaque type/couleur de vaisseau.
 */
class SpaceshipManager {
public:
    /**
     * @brief Constructeur
     * @param graphics Référence au plugin graphique
     */
    explicit SpaceshipManager(engine::IGraphicsPlugin& graphics);

    /**
     * @brief Charge la spritesheet des vaisseaux
     * @param path Chemin vers le fichier Spaceships.png
     * @return true si le chargement a réussi
     */
    bool load_spritesheet(const std::string& path);

    /**
     * @brief Crée un sprite pour un vaisseau spécifique
     * @param color Couleur du vaisseau
     * @param type Type du vaisseau
     * @param scale Échelle du sprite (1.0 = taille normale)
     * @return Sprite configuré pour le vaisseau demandé
     */
    engine::Sprite create_ship_sprite(ShipColor color, ShipType type, float scale = 1.0f) const;

    /**
     * @brief Obtient le rectangle source pour un vaisseau
     * @param color Couleur du vaisseau
     * @param type Type du vaisseau
     * @return Rectangle de découpage dans la spritesheet
     */
    SpriteRect get_ship_rect(ShipColor color, ShipType type) const;

    /**
     * @brief Obtient le handle de texture de la spritesheet
     */
    engine::TextureHandle get_spritesheet_handle() const { return spritesheet_handle_; }

    /**
     * @brief Vérifie si la spritesheet est chargée
     */
    bool is_loaded() const { return spritesheet_handle_ != engine::INVALID_HANDLE; }

    /**
     * @brief Décharge la spritesheet
     */
    void unload();

private:
    engine::IGraphicsPlugin& graphics_;
    engine::TextureHandle spritesheet_handle_;

    // Dimensions d'une cellule dans la spritesheet
    static constexpr int SPRITE_WIDTH = 64;
    static constexpr int SPRITE_HEIGHT = 64;
};

} // namespace rtype::client
