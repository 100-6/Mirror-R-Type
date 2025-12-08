/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Main Client - Solo Game avec RenderSystem + Sprites
*/

#include <iostream>
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "cmath"
#include "ecs/systems/InputSystem.hpp"
#include "ecs/systems/MovementSystem.hpp"
#include "ecs/systems/PhysiqueSystem.hpp"
#include "ecs/systems/CollisionSystem.hpp"
#include "ecs/systems/DestroySystem.hpp"
#include "ecs/systems/RenderSystem.hpp"
#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/IInputPlugin.hpp"

int main() {
    // Configuration de la fenêtre
    const int SCREEN_WIDTH = 1920;
    const int SCREEN_HEIGHT = 1080;

    std::cout << "=== R-Type Client - Solo Game ===" << std::endl;
    std::cout << std::endl;

    // ============================================
    // CHARGEMENT DES PLUGINS
    // ============================================
    engine::PluginManager pluginManager;
    engine::IGraphicsPlugin* graphicsPlugin = nullptr;
    engine::IInputPlugin* inputPlugin = nullptr;

    std::cout << "Chargement du plugin graphique..." << std::endl;
    try {
        graphicsPlugin = pluginManager.load_plugin<engine::IGraphicsPlugin>(
            "plugins/raylib_graphics.so",
            "create_graphics_plugin"
        );
    } catch (const engine::PluginException& e) {
        std::cerr << "❌ Erreur lors du chargement du plugin graphique: " << e.what() << std::endl;
        return 1;
    }

    if (!graphicsPlugin) {
        std::cerr << "❌ Plugin graphique non disponible" << std::endl;
        return 1;
    }

    std::cout << "✓ Plugin graphique chargé: " << graphicsPlugin->get_name()
              << " v" << graphicsPlugin->get_version() << std::endl;

    std::cout << "Chargement du plugin d'input..." << std::endl;
    try {
        inputPlugin = pluginManager.load_plugin<engine::IInputPlugin>(
            "plugins/raylib_input.so",
            "create_input_plugin"
        );
    } catch (const engine::PluginException& e) {
        std::cerr << "❌ Erreur lors du chargement du plugin d'input: " << e.what() << std::endl;
        graphicsPlugin->shutdown();
        return 1;
    }

    if (!inputPlugin) {
        std::cerr << "❌ Plugin d'input non disponible" << std::endl;
        graphicsPlugin->shutdown();
        return 1;
    }

    std::cout << "✓ Plugin d'input chargé: " << inputPlugin->get_name()
              << " v" << inputPlugin->get_version() << std::endl;

    // Créer la fenêtre via le plugin
    if (!graphicsPlugin->create_window(SCREEN_WIDTH, SCREEN_HEIGHT, "R-Type Client - Solo Game")) {
        std::cerr << "❌ Erreur lors de la création de la fenêtre" << std::endl;
        return 1;
    }

    graphicsPlugin->set_vsync(true);
    std::cout << "✓ Fenêtre créée: " << SCREEN_WIDTH << "x" << SCREEN_HEIGHT << std::endl;
    std::cout << std::endl;

    // ============================================
    // CHARGEMENT DES TEXTURES VIA LE PLUGIN
    // ============================================
    std::cout << "Chargement des textures depuis assets/sprite/..." << std::endl;

    engine::TextureHandle backgroundTex = graphicsPlugin->load_texture("assets/sprite/Background.png");
    engine::TextureHandle playerTex = graphicsPlugin->load_texture("assets/sprite/player.png");
    engine::TextureHandle enemyTex = graphicsPlugin->load_texture("assets/sprite/enemy.png");
    engine::TextureHandle bulletTex = graphicsPlugin->load_texture("assets/sprite/bullet.png");

    if (backgroundTex == 0 || playerTex == 0 || enemyTex == 0 || bulletTex == 0) {
        std::cerr << "❌ Erreur lors du chargement des textures" << std::endl;
        graphicsPlugin->shutdown();
        return 1;
    }

    // Récupérer les tailles des textures
    engine::Vector2f playerSize = graphicsPlugin->get_texture_size(playerTex);
    engine::Vector2f enemySize = graphicsPlugin->get_texture_size(enemyTex);
    engine::Vector2f bulletSize = graphicsPlugin->get_texture_size(bulletTex);

    // Calculer les échelles pour des tailles de jeu raisonnables
    const float PLAYER_SCALE = 1.00f;  // 256x128 -> 64x32 pixels
    const float ENEMY_SCALE = 1.00f;    // 277x88 -> 55x18 pixels
    const float BULLET_SCALE = 1.00f;   // 93x10 -> 74x8 pixels

    float playerWidth = playerSize.x * PLAYER_SCALE;
    float playerHeight = playerSize.y * PLAYER_SCALE;
    float enemyWidth = enemySize.x * ENEMY_SCALE;
    float enemyHeight = enemySize.y * ENEMY_SCALE;
    float bulletWidth = bulletSize.x * BULLET_SCALE;
    float bulletHeight = bulletSize.y * BULLET_SCALE;

    // Get default checkerboard texture for walls
    engine::TextureHandle defaultTex = graphicsPlugin->get_default_texture();

    std::cout << "✓ Textures chargées:" << std::endl;
    std::cout << "  Player: " << playerWidth << "x" << playerHeight << std::endl;
    std::cout << "  Enemy: " << enemyWidth << "x" << enemyHeight << std::endl;
    std::cout << "  Bullet: " << bulletWidth << "x" << bulletHeight << std::endl;
    std::cout << "  Default (Pink/Black): 32x32" << std::endl;
    std::cout << std::endl;

    // ============================================
    // CREATION DU REGISTRY ET DES COMPOSANTS
    // ============================================
    Registry registry;
    registry.register_component<Position>();
    registry.register_component<Velocity>();
    registry.register_component<Input>();
    registry.register_component<Collider>();
    registry.register_component<Sprite>();
    registry.register_component<Controllable>();
    registry.register_component<Enemy>();
    registry.register_component<Projectile>();
    registry.register_component<Wall>();
    registry.register_component<Health>();
    registry.register_component<ToDestroy>();

    std::cout << "✓ Composants enregistres" << std::endl;

    // ============================================
    // CREATION ET ENREGISTREMENT DES SYSTEMES
    // ============================================

    // Enregistrer les systèmes dans l'ordre d'exécution
    registry.register_system<InputSystem>(*inputPlugin);
    registry.register_system<MovementSystem>();
    registry.register_system<PhysiqueSystem>();
    registry.register_system<CollisionSystem>();
    registry.register_system<RenderSystem>(*graphicsPlugin);
    registry.register_system<DestroySystem>();

    std::cout << "✓ Systemes enregistres :" << std::endl;
    std::cout << "  1. InputSystem    - Capture les inputs du joueur" << std::endl;
    std::cout << "  2. MovementSystem - Calcule la velocite en fonction des inputs" << std::endl;
    std::cout << "  3. PhysiqueSystem - Applique la velocite, friction, limites d'ecran" << std::endl;
    std::cout << "  4. CollisionSystem- Gere les collisions et marque les entites a detruire" << std::endl;
    std::cout << "  5. DestroySystem  - Detruit les entites marquees pour destruction" << std::endl;
    std::cout << "  6. RenderSystem   - Rendu des sprites via plugin graphique" << std::endl;
    std::cout << std::endl;

    // ============================================
    // CREATION DU BACKGROUND
    // ============================================
    Entity background = registry.spawn_entity();
    registry.add_component(background, Position{0.0f, 0.0f});
    registry.add_component(background, Sprite{
        backgroundTex,
        static_cast<float>(SCREEN_WIDTH),
        static_cast<float>(SCREEN_HEIGHT),
        0.0f,
        engine::Color::White,
        0.0f,
        0.0f,
        -100  // Layer très bas pour être en arrière-plan
    });

    std::cout << "✓ Background cree" << std::endl;
    std::cout << std::endl;

    // ============================================
    // CREATION DU JOUEUR
    // ============================================
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{200.0f, SCREEN_HEIGHT / 2.0f});
    registry.add_component(player, Velocity{0.0f, 0.0f});
    registry.add_component(player, Input{});
    registry.add_component(player, Collider{playerWidth, playerHeight});
    registry.add_component(player, Sprite{
        playerTex,           // texture
        playerWidth,         // width
        playerHeight,        // height
        0.0f,               // rotation
        engine::Color::White, // tint
        0.0f,               // origin_x
        0.0f,               // origin_y
        1                   // layer
    });
    registry.add_component(player, Controllable{300.0f}); // Vitesse de 300 pixels/s
    registry.add_component(player, Health{100, 100});

    std::cout << "✓ Joueur cree avec sprite" << std::endl;
    std::cout << "  Position: (200, " << SCREEN_HEIGHT / 2.0f << ")" << std::endl;
    std::cout << "  Taille: " << playerWidth << "x" << playerHeight << std::endl;
    std::cout << "  Vitesse max: 300 pixels/s" << std::endl;
    std::cout << std::endl;

    // ============================================
    // CREATION DES MURS
    // ============================================
    std::cout << "✓ Creation des murs (Gris)..." << std::endl;

    // Mur vertical gauche
    Entity wall1 = registry.spawn_entity();
    registry.add_component(wall1, Position{400.0f, 200.0f});
    registry.add_component(wall1, Collider{20.0f, 680.0f});
    registry.add_component(wall1, Sprite{defaultTex, 20.0f, 680.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(wall1, Wall{});

    // Mur vertical droit
    Entity wall2 = registry.spawn_entity();
    registry.add_component(wall2, Position{1500.0f, 200.0f});
    registry.add_component(wall2, Collider{20.0f, 680.0f});
    registry.add_component(wall2, Sprite{defaultTex, 20.0f, 680.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(wall2, Wall{});

    // Mur horizontal haut
    Entity wall3 = registry.spawn_entity();
    registry.add_component(wall3, Position{420.0f, 200.0f});
    registry.add_component(wall3, Collider{1080.0f, 20.0f});
    registry.add_component(wall3, Sprite{defaultTex, 1080.0f, 20.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(wall3, Wall{});

    // Mur horizontal bas
    Entity wall4 = registry.spawn_entity();
    registry.add_component(wall4, Position{420.0f, 860.0f});
    registry.add_component(wall4, Collider{1080.0f, 20.0f});
    registry.add_component(wall4, Sprite{defaultTex, 1080.0f, 20.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(wall4, Wall{});

    // Obstacles internes
    Entity obstacle1 = registry.spawn_entity();
    registry.add_component(obstacle1, Position{700.0f, 400.0f});
    registry.add_component(obstacle1, Collider{80.0f, 80.0f});
    registry.add_component(obstacle1, Sprite{defaultTex, 80.0f, 80.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(obstacle1, Wall{});

    Entity obstacle2 = registry.spawn_entity();
    registry.add_component(obstacle2, Position{1100.0f, 600.0f});
    registry.add_component(obstacle2, Collider{80.0f, 80.0f});
    registry.add_component(obstacle2, Sprite{defaultTex, 80.0f, 80.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(obstacle2, Wall{});

    std::cout << "  - 4 murs delimitant l'arene" << std::endl;
    std::cout << "  - 2 obstacles internes" << std::endl;
    std::cout << std::endl;

    // ============================================
    // CREATION D'ENNEMIS (AVEC SPRITES)
    // ============================================
    std::cout << "✓ Creation d'ennemis avec sprites..." << std::endl;

    Entity enemy1 = registry.spawn_entity();
    registry.add_component(enemy1, Position{900.0f, 400.0f});
    registry.add_component(enemy1, Collider{enemyWidth, enemyHeight});
    registry.add_component(enemy1, Sprite{enemyTex, enemyWidth, enemyHeight, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});
    registry.add_component(enemy1, Enemy{});
    registry.add_component(enemy1, Health{50, 50});

    Entity enemy2 = registry.spawn_entity();
    registry.add_component(enemy2, Position{1200.0f, 500.0f});
    registry.add_component(enemy2, Collider{enemyWidth, enemyHeight});
    registry.add_component(enemy2, Sprite{enemyTex, enemyWidth, enemyHeight, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});
    registry.add_component(enemy2, Enemy{});
    registry.add_component(enemy2, Health{50, 50});

    Entity enemy3 = registry.spawn_entity();
    registry.add_component(enemy3, Position{800.0f, 700.0f});
    registry.add_component(enemy3, Collider{enemyWidth, enemyHeight});
    registry.add_component(enemy3, Sprite{enemyTex, enemyWidth, enemyHeight, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});
    registry.add_component(enemy3, Enemy{});
    registry.add_component(enemy3, Health{50, 50});

    std::cout << "  - 3 ennemis places dans l'arene" << std::endl;
    std::cout << std::endl;

    // ============================================
    // INSTRUCTIONS
    // ============================================
    std::cout << "=== CONTROLES ===" << std::endl;
    std::cout << "  WASD ou Fleches  : Deplacer le joueur" << std::endl;
    std::cout << "  ESPACE           : Tirer (a implementer)" << std::endl;
    std::cout << "  ESC              : Quitter" << std::endl;
    std::cout << std::endl;
    std::cout << "=== FONCTIONNALITES ACTIVES ===" << std::endl;
    std::cout << "  ✓ Input        : Capture clavier/souris" << std::endl;
    std::cout << "  ✓ Movement     : Calcul velocite + normalisation diagonales" << std::endl;
    std::cout << "  ✓ Physique     : Application velocite + friction (0.98)" << std::endl;
    std::cout << "  ✓ Collision    : Detection et repulsion murs" << std::endl;
    std::cout << "  ✓ Limites      : Joueur reste dans l'ecran (1920x1080)" << std::endl;
    std::cout << std::endl;
    std::cout << "Demarrage de la boucle de jeu..." << std::endl;
    std::cout << std::endl;

    // ============================================
    // VARIABLES DE JEU
    // ============================================
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& colliders = registry.get_components<Collider>();
    auto& inputs = registry.get_components<Input>();

    int frameCount = 0;
    float debugTimer = 0.0f;
    float shootCooldown = 0.0f;

    // ============================================
    // BOUCLE DE JEU PRINCIPALE
    // ============================================
    while (graphicsPlugin->is_window_open()) {
        float dt = 1.0f / 60.0f;  // Fixed timestep
        frameCount++;
        debugTimer += dt;

        // === UPDATE ===

        // Exécuter tous les systèmes dans l'ordre
        // 1. InputSystem capture les inputs
        // 2. MovementSystem calcule la vélocité
        // 3. PhysiqueSystem applique vélocité + friction + limites
        // 4. CollisionSystem gère les collisions
        // 5. RenderSystem effectue le rendu
        registry.run_systems(dt);

        // === SHOOTING MECHANIC ===
        shootCooldown -= dt;
        if (inputs.has_entity(player)) {
            const Input& playerInput = inputs[player];
            if (playerInput.fire && shootCooldown <= 0.0f) {
                // Spawn projectile
                const Position& playerPos = positions[player];
                const Collider& playerCol = colliders[player];

                Entity projectile = registry.spawn_entity();
                registry.add_component(projectile, Position{
                    playerPos.x + playerCol.width,  // Spawn at right side of player
                    playerPos.y + playerCol.height / 2.0f - bulletHeight / 2.0f  // Centered vertically
                });
                registry.add_component(projectile, Velocity{800.0f, 0.0f});  // Fast horizontal velocity
                registry.add_component(projectile, Collider{bulletWidth, bulletHeight});
                registry.add_component(projectile, Sprite{bulletTex, bulletWidth, bulletHeight, 0.0f, engine::Color::White, 0.0f, 0.0f, 0});
                registry.add_component(projectile, Projectile{});

                shootCooldown = 0.2f;  // 200ms cooldown between shots
            }
        }

        // Note: RenderSystem a déjà appelé clear() et draw_sprite()
        // On peut maintenant ajouter les textes de debug par-dessus

        // === AFFICHAGE DES STATS À L'ÉCRAN ===
        if (inputs.has_entity(player) && positions.has_entity(player) && velocities.has_entity(player)) {
            const Position& playerPos = positions[player];
            const Velocity& playerVel = velocities[player];
            const Input& playerInput = inputs[player];

            int yOffset = 10;
            int lineHeight = 25;

            // Position
            std::string posText = "Position: (" + std::to_string(static_cast<int>(playerPos.x)) + ", " +
                                  std::to_string(static_cast<int>(playerPos.y)) + ")";
            graphicsPlugin->draw_text(posText, engine::Vector2f(10.0f, yOffset),
                                     engine::Color{255, 255, 0, 255}, engine::INVALID_HANDLE, 20);
            yOffset += lineHeight;

            // Vélocité
            std::string velText = "Velocity: (" + std::to_string(static_cast<int>(playerVel.x)) + ", " +
                                  std::to_string(static_cast<int>(playerVel.y)) + ")";
            graphicsPlugin->draw_text(velText, engine::Vector2f(10.0f, yOffset),
                                     engine::Color{255, 255, 0, 255}, engine::INVALID_HANDLE, 20);
            yOffset += lineHeight;

            // Touches pressées
            std::string keysText = "Keys: ";
            if (playerInput.up) keysText += "UP ";
            if (playerInput.down) keysText += "DOWN ";
            if (playerInput.left) keysText += "LEFT ";
            if (playerInput.right) keysText += "RIGHT ";
            if (playerInput.fire) keysText += "FIRE ";
            if (keysText == "Keys: ") keysText += "NONE";

            graphicsPlugin->draw_text(keysText, engine::Vector2f(10.0f, yOffset),
                                     engine::Color{0, 255, 255, 255}, engine::INVALID_HANDLE, 20);
            yOffset += lineHeight;

            // FPS / Frame count
            std::string fpsText = "Frame: " + std::to_string(frameCount) + " (60 FPS)";
            graphicsPlugin->draw_text(fpsText, engine::Vector2f(10.0f, yOffset),
                                     engine::Color{0, 255, 0, 255}, engine::INVALID_HANDLE, 20);
        }

        // Afficher le frame complet (sprites + UI)
        graphicsPlugin->display();
    }

    // ============================================
    // NETTOYAGE
    // ============================================
    inputPlugin->shutdown();
    graphicsPlugin->shutdown();

    std::cout << "=== Fin de la demo ===" << std::endl;
    std::cout << "Total frames: " << frameCount << std::endl;

    return 0;
}
