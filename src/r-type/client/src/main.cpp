/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Main Client - Solo Game avec RenderSystem + Sprites
*/

#include <iostream>
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "components/CombatHelpers.hpp"
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
#include "systems/WaveSpawnerSystem.hpp"
#include "systems/BonusSystem.hpp"
#include "systems/AttachmentSystem.hpp"
#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "plugin_manager/IAudioPlugin.hpp"

int main() {
    // Configuration de la fenêtre
    const int SCREEN_WIDTH = 1920;
    const int SCREEN_HEIGHT = 1080;

    std::cout << "=== R-Type Client - Solo Game ===" << std::endl;
    std::cout << std::endl;

    // ==
    // CHARGEMENT DES PLUGINS
    // ==
    engine::PluginManager pluginManager;
    engine::IGraphicsPlugin* graphicsPlugin = nullptr;
    engine::IInputPlugin* inputPlugin = nullptr;
    engine::IAudioPlugin* audioPlugin = nullptr;

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

    // Créer la fenêtre via le plugin
    if (!graphicsPlugin->create_window(SCREEN_WIDTH, SCREEN_HEIGHT, "R-Type Client - Solo Game")) {
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
    engine::TextureHandle bulletTex = graphicsPlugin->load_texture("assets/sprite/bullet.png");

    if (backgroundTex == 0 || playerTex == 0 || bulletTex == 0) {
        std::cerr << "❌ Erreur lors du chargement des textures" << std::endl;
        graphicsPlugin->shutdown();
        if (audioPlugin) audioPlugin->shutdown();
        return 1;
    }

    // Récupérer les tailles des textures
    engine::Vector2f playerSize = graphicsPlugin->get_texture_size(playerTex);
    engine::Vector2f bulletSize = graphicsPlugin->get_texture_size(bulletTex);

    // Calculer les échelles pour des tailles de jeu raisonnables
    const float PLAYER_SCALE = 1.00f;  // 256x128 -> 64x32 pixels
    const float BULLET_SCALE = 1.00f;   // 93x10 -> 74x8 pixels

    float playerWidth = playerSize.x * PLAYER_SCALE;
    float playerHeight = playerSize.y * PLAYER_SCALE;
    float bulletWidth = bulletSize.x * BULLET_SCALE;
    float bulletHeight = bulletSize.y * BULLET_SCALE;

    std::cout << "✓ Textures chargées:" << std::endl;
    std::cout << "  Player: " << playerWidth << "x" << playerHeight << std::endl;
    std::cout << "  Bullet: " << bulletWidth << "x" << bulletHeight << std::endl;
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
    registry.register_component<NoFriction>();
    registry.register_component<WaveController>();
    registry.register_component<Bonus>();
    registry.register_component<Shield>();
    registry.register_component<SpeedBoost>();
    registry.register_component<CircleEffect>();
    registry.register_component<TextEffect>();
    registry.register_component<Attached>();

    std::cout << "✓ Composants enregistres" << std::endl;

    // ==
    // CREATION ET ENREGISTREMENT DES SYSTEMES
    // ==

    // Enregistrer les systèmes dans l'ordre d'exécution
    registry.register_system<InputSystem>(*inputPlugin);        // 1. Read raw keys from plugin
    registry.register_system<PlayerInputSystem>();              // 2. Interpret keys for R-Type
    registry.register_system<MovementSystem>();
    registry.register_system<ShootingSystem>();
    registry.register_system<PhysiqueSystem>();
    registry.register_system<ScrollingSystem>(-100.0f, static_cast<float>(SCREEN_WIDTH));
    registry.register_system<CollisionSystem>();
    registry.register_system<HealthSystem>();
    registry.register_system<HitEffectSystem>();
    registry.register_system<ScoreSystem>();
    registry.register_system<AttachmentSystem>();

    // AI System - gere le comportement des ennemis (mouvement, tir)
    registry.register_system<AISystem>(*graphicsPlugin);

    // Wave Spawner System - charge la configuration et spawn les ennemis/murs
    // Note: init() sera appelé automatiquement par register_system
    registry.register_system<WaveSpawnerSystem>(*graphicsPlugin);

    // Bonus System - spawn et collecte des bonus (HP, Bouclier, Vitesse)
    registry.register_system<BonusSystem>(*graphicsPlugin, SCREEN_WIDTH, SCREEN_HEIGHT);

    if (audioPlugin) {
        registry.register_system<AudioSystem>(*audioPlugin);
    }
    registry.register_system<DestroySystem>();
    registry.register_system<RenderSystem>(*graphicsPlugin);

    std::cout << "✓ Systemes enregistres :" << std::endl;
    std::cout << "  1. InputSystem         - Capture raw key states from plugin" << std::endl;
    std::cout << "  2. PlayerInputSystem   - Interpret keys for R-Type actions" << std::endl;
    std::cout << "  3. MovementSystem      - Listen to movement events and update velocity" << std::endl;
    std::cout << "  4. ShootingSystem      - Ecoute les evenements de tir et cree des projectiles" << std::endl;
    std::cout << "  5. PhysiqueSystem      - Applique la velocite, friction, limites d'ecran" << std::endl;
    std::cout << "  6. ScrollingSystem     - Gere le scrolling automatique" << std::endl;
    std::cout << "  7. CollisionSystem     - Gere les collisions et marque les entites a detruire" << std::endl;
    std::cout << "  8. HealthSystem        - Gere les degats et la vie" << std::endl;
    std::cout << "  9. HitEffectSystem     - Effets visuels lors des impacts" << std::endl;
    std::cout << "  10. ScoreSystem        - Met a jour le score" << std::endl;
    std::cout << "  11. AISystem           - Comportement IA (mouvement, tir des ennemis)" << std::endl;
    std::cout << "  12. WaveSpawnerSystem  - Spawn automatique base sur le scrolling (JSON)" << std::endl;
    std::cout << "  13. BonusSystem        - Spawn et collecte des bonus (HP, Bouclier, Vitesse)" << std::endl;
    if (audioPlugin) {
        std::cout << "  14. AudioSystem        - Joue des sons sur evenements" << std::endl;
    }
    std::cout << "  15. DestroySystem      - Detruit les entites marquees pour destruction" << std::endl;
    std::cout << "  16. RenderSystem       - Rendu des sprites via plugin graphique" << std::endl;
    std::cout << std::endl;

    // ==
    // CREATION DU BACKGROUND (Défilement infini avec 2 images)
    // ==
    // Premier background
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
    registry.add_component(player, create_weapon(WeaponType::CHARGE, bulletTex));

    registry.add_component(player, Health{100, 100});
    registry.add_component(player, Score{0});
    registry.add_component(player, Invulnerability{0.0f});

    std::cout << "✓ Joueur cree avec sprite et arme SPREAD" << std::endl;
    std::cout << "  Position: (200, " << SCREEN_HEIGHT / 2.0f << ")" << std::endl;
    std::cout << "  Taille: " << playerWidth << "x" << playerHeight << std::endl;
    std::cout << "  Vitesse max: 300 pixels/s" << std::endl;
    std::cout << "  Arme: SPREAD (5 projectiles, 40° d'éventail)" << std::endl;
    std::cout << std::endl;

    std::cout << "✓ Murs et ennemis seront spawnes par le WaveSpawnerSystem" << std::endl;
    std::cout << std::endl;

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
    std::cout << "  ✓ Bonus        : HP (vert), Bouclier (violet), Vitesse (bleu)" << std::endl;
    std::cout << std::endl;
    std::cout << "Demarrage de la boucle de jeu..." << std::endl;
    std::cout << std::endl;

    // ==
    // VARIABLES DE JEU
    // ==
    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& colliders = registry.get_components<Collider>();
    auto& scores = registry.get_components<Score>();
    auto& weapons = registry.get_components<Weapon>();
    auto& healths = registry.get_components<Health>();
    auto& waveControllers = registry.get_components<WaveController>();

    int frameCount = 0;
    float debugTimer = 0.0f;

    // ==
    // BOUCLE DE JEU PRINCIPALE
    // ==
    while (graphicsPlugin->is_window_open()) {
        float dt = 1.0f / 60.0f;  // Fixed timestep
        frameCount++;
        debugTimer += dt;

        // === UPDATE ===
        registry.run_systems(dt);

        // === AFFICHAGE DES STATS À L'ÉCRAN ===
        if (positions.has_entity(player) && velocities.has_entity(player)) {
            const Position& playerPos = positions[player];
            const Velocity& playerVel = velocities[player];

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

        // Wave number displayed in the center-top of the screen
        // Find the WaveController component
        for (size_t i = 0; i < waveControllers.size(); ++i) {
            if (waveControllers.has_entity(i)) {
                const auto& waveCtrl = waveControllers[i];

                if (waveCtrl.currentWaveNumber > 0) {
                    std::string waveText = "WAVE " + std::to_string(waveCtrl.currentWaveNumber) +
                                           " / " + std::to_string(waveCtrl.totalWaveCount);
                    graphicsPlugin->draw_text(waveText, engine::Vector2f(SCREEN_WIDTH / 2.0f - 100.0f, 30.0f),
                                             engine::Color{255, 255, 255, 255}, engine::INVALID_HANDLE, 40);
                } else if (waveCtrl.allWavesCompleted) {
                    std::string waveText = "ALL WAVES COMPLETE!";
                    graphicsPlugin->draw_text(waveText, engine::Vector2f(SCREEN_WIDTH / 2.0f - 200.0f, 30.0f),
                                             engine::Color{0, 255, 0, 255}, engine::INVALID_HANDLE, 40);
                }
                break; // Only one WaveController should exist
            }
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
