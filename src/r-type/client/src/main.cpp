/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Main Client - Solo Game avec RenderSystem + Sprites
*/

#include <iostream>
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "cmath"
#include "ecs/systems/InputSystem.hpp"
#include "systems/PlayerInputSystem.hpp"
#include "ecs/systems/MovementSystem.hpp"
#include "systems/ShootingSystem.hpp"
#include "ecs/systems/PhysiqueSystem.hpp"
#include "systems/ScrollingSystem.hpp"
#include "systems/CollisionSystem.hpp"
#include "ecs/systems/DestroySystem.hpp"
#include "ecs/systems/RenderSystem.hpp"
#include "systems/ScoreSystem.hpp"

#include "ecs/systems/AudioSystem.hpp"
#include "systems/HealthSystem.hpp"
#include "systems/HitEffectSystem.hpp"
#include "systems/AISystem.hpp"
#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "plugin_manager/IAudioPlugin.hpp"
#include "plugin_manager/INetworkPlugin.hpp"
#include "systems/ClientNetworkSystem.hpp"

int main() {
    // Configuration de la fenêtre
    const int SCREEN_WIDTH = 1920;
    const int SCREEN_HEIGHT = 1080;

    std::cout << "=== R-Type Client ===" << std::endl;

    // Check if we want multiplayer mode
    bool multiplayer_mode = true;  // Set to true to connect to server

    if (multiplayer_mode) {
        std::cout << "Mode: MULTIPLAYER - Connecting to server..." << std::endl;
    } else {
        std::cout << "Mode: SOLO - Local game" << std::endl;
    }
    std::cout << std::endl;

    // ==
    // CHARGEMENT DES PLUGINS
    // ==
    engine::PluginManager pluginManager;
    engine::IGraphicsPlugin* graphicsPlugin = nullptr;
    engine::IInputPlugin* inputPlugin = nullptr;
    engine::IAudioPlugin* audioPlugin = nullptr;
    engine::INetworkPlugin* networkPlugin = nullptr;

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
              
    std::cout << "Chargement du plugin audio..." << std::endl;
    try {
        audioPlugin = pluginManager.load_plugin<engine::IAudioPlugin>(
            "plugins/miniaudio_audio.so",
            "create_audio_plugin"
        );
    } catch (const engine::PluginException& e) {
        std::cerr << "⚠️ Erreur lors du chargement du plugin audio (sons désactivés): " << e.what() << std::endl;
        // On continue sans audio
    }

    if (audioPlugin) {
        std::cout << "✓ Plugin audio chargé: " << audioPlugin->get_name()
                  << " v" << audioPlugin->get_version() << std::endl;
    } else {
        std::cout << "⚠️ Plugin audio non disponible." << std::endl;
    }

    std::cout << "Chargement du plugin réseau..." << std::endl;
    try {
        networkPlugin = pluginManager.load_plugin<engine::INetworkPlugin>(
            "plugins/asio_network.so",
            "create_network_plugin"
        );
    } catch (const engine::PluginException& e) {
        std::cerr << "⚠️ Erreur lors du chargement du plugin réseau (mode solo): " << e.what() << std::endl;
        // On continue sans réseau
    }

    if (networkPlugin) {
        std::cout << "✓ Plugin réseau chargé: " << networkPlugin->get_name()
                  << " v" << networkPlugin->get_version() << std::endl;
    } else {
        std::cout << "⚠️ Plugin réseau non disponible - Mode solo uniquement." << std::endl;
    }

    // Créer la fenêtre via le plugin
    std::string window_title = multiplayer_mode ? "R-Type Client - Multiplayer" : "R-Type Client - Solo Game";
    if (!graphicsPlugin->create_window(SCREEN_WIDTH, SCREEN_HEIGHT, window_title.c_str())) {
        std::cerr << "❌ Erreur lors de la création de la fenêtre" << std::endl;
        return 1;
    }

    graphicsPlugin->set_vsync(true);
    std::cout << "✓ Fenêtre créée: " << SCREEN_WIDTH << "x" << SCREEN_HEIGHT << std::endl;
    std::cout << std::endl;

    // ==
    // CHARGEMENT DES TEXTURES VIA LE PLUGIN
    // ==
    std::cout << "Chargement des textures depuis assets/sprite/..." << std::endl;

    engine::TextureHandle backgroundTex = graphicsPlugin->load_texture("assets/sprite/symmetry.png");
    engine::TextureHandle playerTex = graphicsPlugin->load_texture("assets/sprite/player.png");
    engine::TextureHandle enemyTex = graphicsPlugin->load_texture("assets/sprite/enemy.png");
    engine::TextureHandle bulletTex = graphicsPlugin->load_texture("assets/sprite/bullet.png");

    if (backgroundTex == 0 || playerTex == 0 || enemyTex == 0 || bulletTex == 0) {
        std::cerr << "❌ Erreur lors du chargement des textures" << std::endl;
        graphicsPlugin->shutdown();
        if (audioPlugin) audioPlugin->shutdown();
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

    // ==
    // CREATION DU REGISTRY ET DES COMPOSANTS
    // ==
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
    registry.register_component<HitFlash>();
    registry.register_component<Damage>();
    registry.register_component<ToDestroy>();
    registry.register_component<Weapon>();
    registry.register_component<Score>();
    registry.register_component<Background>();
    registry.register_component<Invulnerability>();
    registry.register_component<AI>();
    registry.register_component<Scrollable>();
    registry.register_component<NoFriction>(); // Add NoFriction registration

    std::cout << "✓ Composants enregistres" << std::endl;

    // ==
    // CREATION ET ENREGISTREMENT DES SYSTEMES
    // ==

    // Enregistrer les systèmes dans l'ordre d'exécution
    if (multiplayer_mode && networkPlugin) {
        registry.register_system<rtype::client::ClientNetworkSystem>(*networkPlugin, "127.0.0.1", 4242,
                                                                       playerTex, playerWidth, playerHeight);
    }

    registry.register_system<InputSystem>(*inputPlugin);        // 1. Read raw keys from plugin
    registry.register_system<PlayerInputSystem>();              // 2. Interpret keys for R-Type

    if (!multiplayer_mode) {
        // Solo mode - run all game logic locally
        registry.register_system<MovementSystem>();
        registry.register_system<ShootingSystem>();
        registry.register_system<PhysiqueSystem>();
        registry.register_system<ScrollingSystem>(-100.0f, static_cast<float>(SCREEN_WIDTH));
        registry.register_system<CollisionSystem>();
        registry.register_system<HealthSystem>();
        registry.register_system<HitEffectSystem>();
        registry.register_system<ScoreSystem>();
        registry.register_system<AISystem>(*graphicsPlugin);
    } else {
        // Multiplayer mode - server authoritative, positions come from snapshots
        // DO NOT register MovementSystem - the server handles movement
        // DO NOT register PhysiqueSystem - positions are updated by ClientNetworkSystem from snapshots
        // The client only sends inputs and renders entities received from server
    }

    if (audioPlugin) {
        registry.register_system<AudioSystem>(*audioPlugin);
    }
    registry.register_system<DestroySystem>();
    registry.register_system<RenderSystem>(*graphicsPlugin);

    std::cout << "✓ Systemes enregistres :" << std::endl;
    if (networkPlugin) {
        std::cout << "  0. ClientNetworkSystem - Connexion au serveur et sync des joueurs" << std::endl;
    }
    std::cout << "  1. InputSystem       - Capture raw key states from plugin" << std::endl;
    std::cout << "  2. PlayerInputSystem - Interpret keys for R-Type actions" << std::endl;
    std::cout << "  3. MovementSystem    - Listen to movement events and update velocity" << std::endl;
    std::cout << "  4. ShootingSystem    - Ecoute les evenements de tir et cree des projectiles" << std::endl;
    std::cout << "  5. PhysiqueSystem    - Applique la velocite, friction, limites d'ecran" << std::endl;
    std::cout << "  6. CollisionSystem   - Gere les collisions et marque les entites a detruire" << std::endl;
    std::cout << "  7. ScoreSystem       - Met a jour le score" << std::endl;
    if (audioPlugin) {
        std::cout << "  8. AudioSystem       - Joue des sons sur evenements" << std::endl;
    }
    std::cout << "  9. DestroySystem     - Detruit les entites marquees pour destruction" << std::endl;
    std::cout << " 10. RenderSystem      - Rendu des sprites via plugin graphique" << std::endl;
    std::cout << std::endl;

    // ==
    // CREATION DES ENTITES DU JEU
    // ==
    Entity player = 0;  // Player entity (needed in both modes)

    if (!multiplayer_mode) {
        // ==
        // MODE SOLO - Créer tout le monde du jeu localement
        // ==

        // CREATION DU BACKGROUND (Défilement infini avec 2 images)
        std::cout << "✓ Creation du background defilant..." << std::endl;
        Entity background1 = registry.spawn_entity();
    registry.add_component(background1, Position{0.0f, 0.0f});
    registry.add_component(background1, Background{});  // Tag pour identifier le background
    registry.add_component(background1, Scrollable{
        1.0f,   // speedMultiplier: vitesse normale (100%)
        true,   // wrap: boucle infinie
        false   // destroyOffscreen: ne pas détruire
    });
    registry.add_component(background1, Sprite{
        backgroundTex,
        static_cast<float>(SCREEN_WIDTH),
        static_cast<float>(SCREEN_HEIGHT),
        0.0f,
        engine::Color::White,
        0.0f,
        0.0f,
        -100  // Layer très bas pour être en arrière-plan
    });

    // Deuxième background (juste à droite du premier pour seamless scrolling)
    Entity background2 = registry.spawn_entity();
    registry.add_component(background2, Position{static_cast<float>(SCREEN_WIDTH), 0.0f});
    registry.add_component(background2, Background{});  // Tag pour identifier le background
    registry.add_component(background2, Scrollable{
        1.0f,   // speedMultiplier: vitesse normale (100%)
        true,   // wrap: boucle infinie
        false   // destroyOffscreen: ne pas détruire
    });
    registry.add_component(background2, Sprite{
        backgroundTex,
        static_cast<float>(SCREEN_WIDTH),
        static_cast<float>(SCREEN_HEIGHT),
        0.0f,
        engine::Color::White,
        0.0f,
        0.0f,
        -100
    });

    std::cout << "✓ Background defilant cree (2 images en boucle infinie via ScrollingSystem)" << std::endl;
    std::cout << "  Vitesse: -100 px/s (definie dans ScrollingSystem)" << std::endl;
    std::cout << std::endl;

    // ==
    // CREATION DU JOUEUR
    // ==
    Entity player = registry.spawn_entity();
    registry.add_component(player, Position{200.0f, SCREEN_HEIGHT / 2.0f});
    registry.add_component(player, Velocity{0.0f, 0.0f});
    registry.add_component(player, Input{});  // Component pour capturer les inputs
    registry.add_component(player, Collider{playerWidth, playerHeight});
    registry.add_component(player, Controllable{300.0f}); // Vitesse de 300 pixels/s
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

    // ARME - Les stats sont dans CombatConfig.hpp (defines)
    registry.add_component(player, Weapon{
        WeaponType::BASIC,           // Type: BASIC/SPREAD/BURST/LASER
        999.0f,                      // Temps depuis dernier tir
        0,                           // Compteur de rafale
        Sprite{                      // Apparence des projectiles
            bulletTex,
            bulletWidth,
            bulletHeight,
            0.0f,
            engine::Color{255, 100, 255, 255},
            0.0f,
            0.0f,
            1
        }
    });

    registry.add_component(player, Health{100, 100});
    registry.add_component(player, Score{0});
    registry.add_component(player, Invulnerability{0.0f});

    std::cout << "✓ Joueur cree avec sprite et arme SPREAD" << std::endl;
    std::cout << "  Position: (200, " << SCREEN_HEIGHT / 2.0f << ")" << std::endl;
    std::cout << "  Taille: " << playerWidth << "x" << playerHeight << std::endl;
    std::cout << "  Vitesse max: 300 pixels/s" << std::endl;
    std::cout << "  Arme: SPREAD (5 projectiles, 40° d'éventail)" << std::endl;
    std::cout << std::endl;

    // ==
    // CREATION DES MURS
    // ==
    std::cout << "✓ Creation des murs (Gris)..." << std::endl;

    // Mur vertical gauche - PARTIE HAUTE (avec un trou au milieu pour tirer)
    Entity wall1_top = registry.spawn_entity();
    registry.add_component(wall1_top, Position{400.0f, 200.0f});
    registry.add_component(wall1_top, Collider{20.0f, 240.0f});  // Hauteur réduite
    registry.add_component(wall1_top, Sprite{defaultTex, 20.0f, 240.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(wall1_top, Wall{});
    registry.add_component(wall1_top, Scrollable{1.0f, false, true}); // Scroll et détruit hors écran

    // Mur vertical gauche - PARTIE BASSE (trou de 200px au milieu)
    Entity wall1_bottom = registry.spawn_entity();
    registry.add_component(wall1_bottom, Position{400.0f, 640.0f});  // Commence après le trou
    registry.add_component(wall1_bottom, Collider{20.0f, 240.0f});  // Hauteur réduite
    registry.add_component(wall1_bottom, Sprite{defaultTex, 20.0f, 240.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(wall1_bottom, Wall{});
    registry.add_component(wall1_bottom, Scrollable{1.0f, false, true}); // Scroll et détruit hors écran

    // Mur vertical droit
    Entity wall2 = registry.spawn_entity();
    registry.add_component(wall2, Position{1500.0f, 200.0f});
    registry.add_component(wall2, Collider{20.0f, 680.0f});
    registry.add_component(wall2, Sprite{defaultTex, 20.0f, 680.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(wall2, Wall{});
    registry.add_component(wall2, Scrollable{1.0f, false, true}); // Scroll et détruit hors écran

    // Mur horizontal haut
    Entity wall3 = registry.spawn_entity();
    registry.add_component(wall3, Position{420.0f, 200.0f});
    registry.add_component(wall3, Collider{1080.0f, 20.0f});
    registry.add_component(wall3, Sprite{defaultTex, 1080.0f, 20.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(wall3, Wall{});
    registry.add_component(wall3, Scrollable{1.0f, false, true}); // Scroll et détruit hors écran

    // Mur horizontal bas
    Entity wall4 = registry.spawn_entity();
    registry.add_component(wall4, Position{420.0f, 860.0f});
    registry.add_component(wall4, Collider{1080.0f, 20.0f});
    registry.add_component(wall4, Sprite{defaultTex, 1080.0f, 20.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(wall4, Wall{});
    registry.add_component(wall4, Scrollable{1.0f, false, true}); // Scroll et détruit hors écran

    // Obstacles internes
    Entity obstacle1 = registry.spawn_entity();
    registry.add_component(obstacle1, Position{700.0f, 400.0f});
    registry.add_component(obstacle1, Collider{80.0f, 80.0f});
    registry.add_component(obstacle1, Sprite{defaultTex, 80.0f, 80.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(obstacle1, Wall{});
    registry.add_component(obstacle1, Scrollable{1.0f, false, true}); // Scroll et détruit hors écran

    Entity obstacle2 = registry.spawn_entity();
    registry.add_component(obstacle2, Position{1100.0f, 600.0f});
    registry.add_component(obstacle2, Collider{80.0f, 80.0f});
    registry.add_component(obstacle2, Sprite{defaultTex, 80.0f, 80.0f, 0.0f, engine::Color::White, 0.0f, 0.0f, -1});
    registry.add_component(obstacle2, Wall{});
    registry.add_component(obstacle2, Scrollable{1.0f, false, true}); // Scroll et détruit hors écran

    std::cout << "  - 4 murs delimitant l'arene (avec Scrollable)" << std::endl;
    std::cout << "  - 2 obstacles internes (avec Scrollable)" << std::endl;
    std::cout << "  - Tous les murs scrollent avec la map et se detruisent hors ecran" << std::endl;
    std::cout << std::endl;

    // ==
    // CREATION D'ENNEMIS (AVEC SPRITES)
    // ==
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

        std::cout << "  - 3 ennemis places dans l'arene (avec Scrollable)" << std::endl;
        std::cout << "  - Les ennemis scrollent avec la map et se detruisent hors ecran" << std::endl;
        std::cout << std::endl;

    } else {
        // ==
        // MODE MULTIPLAYER - Créer seulement le background, les joueurs viennent du serveur
        // ==
        std::cout << "✓ Mode multiplayer - Creation du monde local..." << std::endl;

        // Create static background (no scrolling in multiplayer)
        Entity background1 = registry.spawn_entity();
        registry.add_component(background1, Position{0.0f, 0.0f});
        registry.add_component(background1, Sprite{
            backgroundTex,
            static_cast<float>(SCREEN_WIDTH),
            static_cast<float>(SCREEN_HEIGHT),
            0.0f,
            engine::Color::White,
            0.0f,
            0.0f,
            -100  // Layer très bas pour être en arrière-plan
        });

        // Create input-only entity for capturing keyboard input
        // This entity has ONLY Input component - no Position/Velocity/Sprite
        // The actual player entity will come from server snapshots
        Entity inputEntity = registry.spawn_entity();
        registry.add_component(inputEntity, Input{});

        std::cout << "  - Background statique cree" << std::endl;
        std::cout << "  - Entite Input creee pour capturer les controles clavier" << std::endl;
        std::cout << "  - Joueurs seront recus du serveur via snapshots" << std::endl;
        std::cout << std::endl;
    }

    // ==
    // INSTRUCTIONS
    // ==
    std::cout << "=== CONTROLES ===" << std::endl;
    std::cout << "  WASD ou Fleches  : Deplacer le joueur" << std::endl;
    std::cout << "  ESPACE           : Tirer" << std::endl;
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

    // ==
    // VARIABLES DE JEU
    // ==
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& colliders = registry.get_components<Collider>();
    // auto& inputs = registry.get_components<Input>(); // No longer used after refactor
    auto& scores = registry.get_components<Score>();

    auto& weapons = registry.get_components<Weapon>();

    auto& healths = registry.get_components<Health>();


    int frameCount = 0;
    float debugTimer = 0.0f;
    // float shootCooldown = 0.0f; // No longer used after refactor

    // ==
    // BOUCLE DE JEU PRINCIPALE
    // ==
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
        // 5. ShootingSystem gère la création de projectiles selon l'arme
        // 6. RenderSystem effectue le rendu
        registry.run_systems(dt);

        // === Old SHOOTING MECHANIC (removed as now handled by ShootingSystem) ===
        // This block is removed as ShootingSystem now handles bullet spawning via events.

        // Note: RenderSystem a déjà appelé clear() et draw_sprite()
        // On peut maintenant ajouter les textes de debug par-dessus

        // === AFFICHAGE DES STATS À L'ÉCRAN ===
        if (positions.has_entity(player) && velocities.has_entity(player)) { // Removed inputs.has_entity(player)
            const Position& playerPos = positions[player];
            const Velocity& playerVel = velocities[player];
            // const Input& playerInput = inputs[player]; // No longer used

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

            // Touches pressées - Display from InputSystem events instead of Input component
            // For now, removing. If needed, a debug system could show last input events.
            // std::string keysText = "Keys: ";
            // if (playerInput.up) keysText += "UP ";
            // if (playerInput.down) keysText += "DOWN ";
            // if (playerInput.left) keysText += "LEFT ";
            // if (playerInput.right) keysText += "RIGHT ";
            // if (playerInput.fire) keysText += "FIRE ";
            // if (keysText == "Keys: ") keysText += "NONE";

            // graphicsPlugin->draw_text(keysText, engine::Vector2f(10.0f, yOffset),
            //                          engine::Color{0, 255, 255, 255}, engine::INVALID_HANDLE, 20);
            // yOffset += lineHeight;

            // FPS / Frame count
            std::string fpsText = "Frame: " + std::to_string(frameCount) + " (60 FPS)";
            graphicsPlugin->draw_text(fpsText, engine::Vector2f(10.0f, yOffset),
                                     engine::Color{0, 255, 0, 255}, engine::INVALID_HANDLE, 20);
            yOffset += lineHeight;

            // Score (dans le debug)
            if (scores.has_entity(player)) {
                std::string scoreText = "Score: " + std::to_string(scores[player].value);
                graphicsPlugin->draw_text(scoreText, engine::Vector2f(10.0f, yOffset),
                                         engine::Color{255, 0, 255, 255}, engine::INVALID_HANDLE, 20);
            }
        }

        // Score affiché en grand en haut à droite (toujours visible)
        if (scores.has_entity(player)) {
            std::string scoreText = "SCORE: " + std::to_string(scores[player].value);
            graphicsPlugin->draw_text(scoreText, engine::Vector2f(SCREEN_WIDTH - 300.0f, 30.0f),
                                     engine::Color{255, 255, 0, 255}, engine::INVALID_HANDLE, 40);
        }

        // Vie du joueur affichée en haut à gauche (toujours visible)
        if (healths.has_entity(player)) {
            int hp = healths[player].current;
            int maxHp = healths[player].max;
            std::string healthText = "HP: " + std::to_string(hp) + " / " + std::to_string(maxHp);

            // Couleur selon la vie restante
            engine::Color healthColor;
            float hpPercent = static_cast<float>(hp) / static_cast<float>(maxHp);
            if (hpPercent > 0.6f)
                healthColor = engine::Color{0, 255, 0, 255};     // Vert
            else if (hpPercent > 0.3f)
                healthColor = engine::Color{255, 165, 0, 255};   // Orange
            else
                healthColor = engine::Color{255, 0, 0, 255};     // Rouge

            graphicsPlugin->draw_text(healthText, engine::Vector2f(30.0f, 30.0f),
                                     healthColor, engine::INVALID_HANDLE, 40);
        }

        // Afficher le frame complet (sprites + UI)
        graphicsPlugin->display();
    }

    // ==
    // NETTOYAGE
    // ==
    inputPlugin->shutdown();
    graphicsPlugin->shutdown();
    if (audioPlugin) audioPlugin->shutdown();

    std::cout << "=== Fin de la demo ===" << std::endl;
    std::cout << "Total frames: " << frameCount << std::endl;

    return 0;
}
