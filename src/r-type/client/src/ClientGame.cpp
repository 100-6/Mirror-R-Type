#include "ClientGame.hpp"
#include "ecs/systems/SpriteAnimationSystem.hpp"
#include "ecs/systems/MovementSystem.hpp"
#include "ecs/systems/AudioSystem.hpp"
#include "systems/AttachmentSystem.hpp"
#include "systems/ScrollingSystem.hpp"
#include "ecs/systems/RenderSystem.hpp"
#include "systems/HUDSystem.hpp"
#include "systems/LocalPredictionSystem.hpp"
#include "systems/ColliderDebugSystem.hpp"
// Note: CollisionSystem not needed on client - server handles all collisions
#include "systems/BonusSystem.hpp"
#include "systems/CompanionSystem.hpp"
#include "systems/MuzzleFlashSystem.hpp"
#include "systems/HitEffectSystem.hpp"
#include "systems/ClientHitFlashSystem.hpp"
#include "systems/MapConfigLoader.hpp"
#include "protocol/NetworkConfig.hpp"
#include "protocol/Payloads.hpp"
#include "plugin_manager/PluginPaths.hpp"
#include "ecs/events/InputEvents.hpp"
#include "ecs/events/GameEvents.hpp"
#include "ClientComponents.hpp"
#include "components/GameComponents.hpp"
#include "components/LevelComponents.hpp"
#include "ecs/CoreComponents.hpp"
#include "AssetsPaths.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <nlohmann/json.hpp>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
#endif

namespace rtype::client {

ClientGame::ClientGame(int screen_width, int screen_height)
    : screen_width_(screen_width)
    , screen_height_(screen_height)
    , tcp_port_(protocol::config::DEFAULT_TCP_PORT)
    , graphics_plugin_(nullptr)
    , input_plugin_(nullptr)
    , audio_plugin_(nullptr)
    , network_plugin_(nullptr)
    , running_(false)
    , client_tick_(0)
    , current_time_(0.0f) {
}

ClientGame::~ClientGame() {
    shutdown();
}

bool ClientGame::initialize(const std::string& host, uint16_t tcp_port, const std::string& player_name) {
    host_ = host;
    tcp_port_ = tcp_port;
    player_name_ = player_name;

//     std::cout << "=== R-Type Network Client ===\n";
//     std::cout << "Server: " << host_ << ":" << tcp_port_ << '\n';

    if (!load_plugins())
        return false;

    registry_ = std::make_unique<Registry>();
    setup_registry();

    texture_manager_ = std::make_unique<TextureManager>(*graphics_plugin_);
    if (!load_textures())
        return false;

    setup_background();

    screen_manager_ = std::make_unique<ScreenManager>(
        *registry_, screen_width_, screen_height_,
        texture_manager_->get_background(),
        texture_manager_->get_menu_background(),
        graphics_plugin_
    );
    screen_manager_->initialize();

    // Set callback for back to menu button
    screen_manager_->set_back_to_menu_callback([this]() {
        // Reset GameState to PLAYING so player can start a new game
        auto& gameStates = registry_->get_components<GameState>();
        for (size_t i = 0; i < gameStates.size(); ++i) {
            Entity entity = gameStates.get_entity_at(i);
            gameStates[entity].currentState = GameStateType::PLAYING;
            gameStates[entity].finalScore = 0;
        }

        // Emit scene change event for menu music
        registry_->get_event_bus().publish(ecs::SceneChangeEvent{
            ecs::SceneChangeEvent::SceneType::MENU, 0
        });

        // Reset to main menu
        menu_manager_->set_screen(GameScreen::MAIN_MENU);
        screen_manager_->set_screen(GameScreen::MAIN_MENU);
    });

    entity_manager_ = std::make_unique<EntityManager>(
        *registry_, *texture_manager_, screen_width_, screen_height_
    );

    setup_systems();
    setup_map_system();

    status_overlay_ = std::make_unique<StatusOverlay>(
        *registry_, screen_manager_->get_status_entity()
    );

    network_client_ = std::make_unique<rtype::client::NetworkClient>(*network_plugin_);
    setup_network_callbacks();

    // Initialize lag compensation systems
    interpolation_system_ = std::make_unique<InterpolationSystem>();
    debug_network_overlay_ = std::make_unique<DebugNetworkOverlay>(false);

    console_overlay_ = std::make_unique<ConsoleOverlay>(
        static_cast<float>(screen_width_),
        static_cast<float>(screen_height_)
    );
    console_overlay_->set_command_callback([this](const std::string& cmd) {
        handle_console_command(cmd);
    });
    network_client_->set_on_admin_auth_result([this](bool success, const std::string& msg) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !console_overlay_)
            return;
        if (success) {
            admin_authenticated_ = true;
            console_overlay_->add_success(msg);
        } else {
            admin_authenticated_ = false;
            console_overlay_->add_error("Auth failed: " + msg);
        }
    });
    network_client_->set_on_admin_command_result([this](bool success, const std::string& msg) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !console_overlay_)
            return;
        if (success)
            console_overlay_->add_success(msg);
        else
            console_overlay_->add_error(msg);
    });

    // Initialize chat overlay
    chat_overlay_ = std::make_unique<ChatOverlay>(
        static_cast<float>(screen_width_),
        static_cast<float>(screen_height_)
    );
    chat_overlay_->set_send_callback([this](const std::string& message) {
        if (network_client_) {
            network_client_->send_chat_message(message);
        }
    });
    network_client_->set_on_chat_message([this](uint32_t sender_id, const std::string& sender_name, const std::string& message) {
        std::cout << "[ClientGame] Chat callback: " << sender_name << ": " << message << "\n";
        if (chat_overlay_) {
            chat_overlay_->add_message(sender_id, sender_name, message);
            std::cout << "[ClientGame] Message added to chat overlay\n";
        }
    });

    // Initialize menu manager
    menu_manager_ = std::make_unique<MenuManager>(*network_client_, screen_width_, screen_height_);
    menu_manager_->initialize();

    // Connect audio settings from UI to AudioSystem
    if (audio_plugin_ && menu_manager_->get_settings_screen()) {
        auto* settings_screen = menu_manager_->get_settings_screen();

        // Set initial volumes from AudioSystem
        try {
            AudioSystem& audio_sys = registry_->get_system<AudioSystem>();
            settings_screen->set_initial_volumes(
                audio_sys.getMasterVolume(),
                audio_sys.getMusicVolume(),
                audio_sys.getSfxVolume(),
                audio_sys.getAmbianceVolume()
            );

            // Set callback to update AudioSystem when settings change
            settings_screen->set_audio_settings_callback([this](const AudioSettings& settings) {
                try {
                    AudioSystem& audio = registry_->get_system<AudioSystem>();
                    audio.setMasterVolume(settings.master);
                    audio.setMusicVolume(settings.music);
                    audio.setSfxVolume(settings.sfx);
                    audio.setAmbianceVolume(settings.ambiance);
                } catch (const std::exception& e) {
                    std::cerr << "[ClientGame] Failed to update audio settings: " << e.what() << std::endl;
                }
            });
        } catch (const std::exception& e) {
            std::cerr << "[ClientGame] Failed to initialize audio settings: " << e.what() << std::endl;
        }
    }

    // Create input handler with key bindings from settings
    input_handler_ = std::make_unique<InputHandler>(
        *input_plugin_,
        menu_manager_->get_settings_screen() ? &menu_manager_->get_settings_screen()->get_key_bindings() : nullptr
    );

    // Setup connection screen callback
    if (menu_manager_->get_connection_screen()) {
        menu_manager_->get_connection_screen()->set_defaults(host_, tcp_port_, player_name_);
        menu_manager_->get_connection_screen()->set_connect_callback(
            [this](const std::string& host, uint16_t port, const std::string& player_name) {
                // Store connection parameters
                host_ = host;
                tcp_port_ = port;
                player_name_ = player_name;

                status_overlay_->set_connection("Connecting to " + host_ + ":" + std::to_string(tcp_port_));
                status_overlay_->refresh();

                if (!network_client_->connect(host_, tcp_port_)) {
                    std::cerr << "[ClientGame] Failed to connect to server\n";
                    if (menu_manager_->get_connection_screen()) {
                        menu_manager_->get_connection_screen()->set_error_message("Connection failed");
                    }
                    return;
                }

                network_client_->send_connect(player_name_);

                // Connection successful, switch to main menu
                menu_manager_->set_screen(GameScreen::MAIN_MENU);
                screen_manager_->set_screen(GameScreen::MAIN_MENU);
            }
        );
    }

    // Start at connection screen
    menu_manager_->set_screen(GameScreen::CONNECTION);
    screen_manager_->set_screen(GameScreen::CONNECTION);

    // Emit scene change event for menu music at startup
    registry_->get_event_bus().publish(ecs::SceneChangeEvent{
        ecs::SceneChangeEvent::SceneType::MENU, 0
    });

    running_ = true;

    return true;
}

bool ClientGame::load_plugins() {
    try {
        graphics_plugin_ = plugin_manager_.load_plugin<engine::IGraphicsPlugin>(
            engine::PluginPaths::get_plugin_path(engine::PluginPaths::RAYLIB_GRAPHICS),
            "create_graphics_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "[ClientGame] Failed to load graphics plugin: " << e.what() << '\n';
        return false;
    }

    if (!graphics_plugin_ || !graphics_plugin_->create_window(screen_width_, screen_height_, "R-Type - Multiplayer")) {
        std::cerr << "[ClientGame] Unable to initialize graphics plugin\n";
        return false;
    }
    graphics_plugin_->set_vsync(true);

    try {
        input_plugin_ = plugin_manager_.load_plugin<engine::IInputPlugin>(
            engine::PluginPaths::get_plugin_path(engine::PluginPaths::RAYLIB_INPUT),
            "create_input_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "[ClientGame] Failed to load input plugin: " << e.what() << '\n';
        graphics_plugin_->shutdown();
        return false;
    }

    try {
        audio_plugin_ = plugin_manager_.load_plugin<engine::IAudioPlugin>(
            engine::PluginPaths::get_plugin_path(engine::PluginPaths::MINIAUDIO_AUDIO),
            "create_audio_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "[ClientGame] Audio plugin unavailable: " << e.what() << '\n';
    }

    try {
        network_plugin_ = plugin_manager_.load_plugin<engine::INetworkPlugin>(
            engine::PluginPaths::get_plugin_path(engine::PluginPaths::ASIO_NETWORK),
            "create_network_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "[ClientGame] Failed to load network plugin: " << e.what() << '\n';
        graphics_plugin_->shutdown();
        if (audio_plugin_)
            audio_plugin_->shutdown();
        return false;
    }

    if (!network_plugin_->initialize()) {
        std::cerr << "[ClientGame] Network plugin initialization failed\n";
        graphics_plugin_->shutdown();
        if (audio_plugin_)
            audio_plugin_->shutdown();
        return false;
    }

    return true;
}

bool ClientGame::load_textures() {
    if (!texture_manager_->load_all()) {
        std::cerr << "[ClientGame] Failed to load textures\n";
        return false;
    }
    return true;
}

void ClientGame::setup_registry() {
    registry_->register_component<Position>();
    registry_->register_component<Velocity>();
    registry_->register_component<::Sprite>();
    registry_->register_component<SpriteAnimation>();
    registry_->register_component<Collider>();
    registry_->register_component<Health>();
    registry_->register_component<Score>();
    registry_->register_component<Controllable>();
    registry_->register_component<LocalPlayer>();
    registry_->register_component<Background>();
    registry_->register_component<Scrollable>();
    registry_->register_component<NetworkId>();
    registry_->register_component<UIText>();
    registry_->register_component<GameState>();
    registry_->register_component<WaveController>();
    registry_->register_component<Shield>();
    registry_->register_component<SpeedBoost>();
    registry_->register_component<CircleEffect>();
    registry_->register_component<TextEffect>();
    registry_->register_component<Bonus>();
    registry_->register_component<Invulnerability>();
    registry_->register_component<ShotAnimation>();
    registry_->register_component<BulletAnimation>();
    registry_->register_component<ExplosionAnimation>();
    registry_->register_component<Attached>();
    registry_->register_component<Wall>();
    registry_->register_component<Damage>();
    registry_->register_component<Projectile>();
    registry_->register_component<ProjectileOwner>();
    registry_->register_component<Enemy>();
    registry_->register_component<NoFriction>();
    registry_->register_component<BonusWeapon>();
    registry_->register_component<BonusLifetime>();
    registry_->register_component<ToDestroy>();
    registry_->register_component<HitFlash>();
    registry_->register_component<FlashOverlay>();
    // Level system components
    // registry_->register_component<game::CheckpointManager>();
    // Client-side extrapolation component
    registry_->register_component<LastServerState>();

    // Create GameState entity for HUD visibility control
    Entity gameStateEntity = registry_->spawn_entity();
    registry_->add_component(gameStateEntity, GameState{GameStateType::PLAYING});
}

void ClientGame::setup_systems() {
    if (!registry_)
        return;

    // Audio system - handles sound effects and music
    if (audio_plugin_) {
        registry_->register_system<AudioSystem>(*audio_plugin_, assets::paths::AUDIO_CONFIG);
    }

    // Background scrolling
    registry_->register_system<ScrollingSystem>(-100.0f, static_cast<float>(screen_width_));
    // Visual and gameplay systems
    registry_->register_system<AttachmentSystem>();
    registry_->register_system<SpriteAnimationSystem>();
    registry_->register_system<MovementSystem>();
    // Note: CollisionSystem is server-side only - client receives damage from server snapshots

    if (entity_manager_) {
        registry_->register_system<LocalPredictionSystem>(
            *entity_manager_,
            static_cast<float>(screen_width_),
            static_cast<float>(screen_height_)
        );
    }

    registry_->register_system<RenderSystem>(*graphics_plugin_);
    registry_->register_system<ColliderDebugSystem>(*graphics_plugin_);
    registry_->register_system<HUDSystem>(*graphics_plugin_, input_plugin_, screen_width_, screen_height_);

    // Bonus system - handles bonus collection and effects (with graphics for sprite loading)
    registry_->register_system<BonusSystem>(graphics_plugin_, screen_width_, screen_height_);

    // Companion system - handles companion turret spawn/destroy via ECS events
    registry_->register_system<CompanionSystem>(graphics_plugin_);
    registry_->get_system<CompanionSystem>().set_companion_texture(texture_manager_->get_bonus_weapon());

    // Muzzle flash system - handles muzzle flash effects via ECS events
    registry_->register_system<MuzzleFlashSystem>(graphics_plugin_);
    registry_->get_system<MuzzleFlashSystem>().set_muzzle_flash_texture(texture_manager_->get_shot_frame_1());

    // Client hit flash system - handles white flash effects when entities take damage
    registry_->register_system<rtype::client::ClientHitFlashSystem>();
}

void ClientGame::setup_background() {
    background1_ = registry_->spawn_entity();
    registry_->add_component(background1_, Position{0.0f, 0.0f});
    registry_->add_component(background1_, Background{});
    registry_->add_component(background1_, Scrollable{1.0f, true, false});
    registry_->add_component(background1_, ::Sprite{
        texture_manager_->get_background(),
        static_cast<float>(screen_width_),
        static_cast<float>(screen_height_),
        0.0f,
        engine::Color::White,
        0.0f,
        0.0f,
        -100
    });

    background2_ = registry_->spawn_entity();
    registry_->add_component(background2_, Position{static_cast<float>(screen_width_), 0.0f});
    registry_->add_component(background2_, Background{});
    registry_->add_component(background2_, Scrollable{1.0f, true, false});
    registry_->add_component(background2_, ::Sprite{
        texture_manager_->get_background(),
        static_cast<float>(screen_width_),
        static_cast<float>(screen_height_),
        0.0f,
        engine::Color::White,
        0.0f,
        0.0f,
        -100
    });

    wave_tracker_ = registry_->spawn_entity();
    WaveController wave_ctrl;
    wave_ctrl.totalWaveCount = 0;
    wave_ctrl.currentWaveNumber = 1;
    wave_ctrl.currentWaveIndex = 0;
    wave_ctrl.totalScrollDistance = 0.0f;
    wave_ctrl.allWavesCompleted = false;
    registry_->add_component(wave_tracker_, wave_ctrl);
}

void ClientGame::apply_map_theme(uint16_t map_id) {
    current_map_id_ = map_id;

    // Define color tint for each map
    engine::Color tint;
    switch (map_id) {
        case 1:  // Nebula Outpost - default blue/cyan tint
            tint = engine::Color{255, 255, 255, 255};  // No tint (original)
            break;
        case 2:  // Asteroid Belt - red/orange tint
            tint = engine::Color{255, 150, 130, 255};
            break;
        case 3:  // Bydo Mothership - purple/violet tint
            tint = engine::Color{200, 150, 255, 255};
            break;
        default:
            tint = engine::Color{255, 255, 255, 255};
            break;
    }

    // Apply tint to both background entities
    auto& sprites = registry_->get_components<::Sprite>();
    if (sprites.has_entity(background1_)) {
        sprites[background1_].tint = tint;
    }
    if (sprites.has_entity(background2_)) {
        sprites[background2_].tint = tint;
    }

//     std::cout << "[ClientGame] Applied theme for map " << map_id << "\n";
    entity_manager_->set_current_map_id(map_id);

    // If map is Level 2 (Infinite Nebula), enable procedural mobs in WaveController
    if (map_id == 2) {
        if (registry_->has_component_registered<WaveController>() && 
            registry_->get_components<WaveController>().has_entity(wave_tracker_)) {
            auto& waveCtrl = registry_->get_components<WaveController>()[wave_tracker_];
            waveCtrl.proceduralMobs = true;
            waveCtrl.currentWaveNumber = 1; // Ensure we start at 1
            std::cout << "[ClientGame] Enabled procedural mobs for Infinite Level " << map_id << "\n";
        }
    }
}

void ClientGame::load_level_checkpoints(uint16_t map_id) {
    (void)map_id;
    // Checkpoints removed - dynamic respawn used
}

void ClientGame::apply_pending_level_transition() {
    if (!pending_level_transition_apply_) return;

    uint16_t next_level = pending_level_id_;
    std::cout << "[ClientGame] Applying level transition to Level " << next_level << "\n";

    // Map visual assets based on level
    std::string mapIdTransitionStr;
    switch (next_level) {
        case 0:   // Debug: Quick Test
        case 99:  // Debug: Instant Boss
            mapIdTransitionStr = "nebula_outpost";
            break;
        case 1:   // Level 1: Mars Assault
            mapIdTransitionStr = "mars_outpost";
            break;
        case 2:   // Level 2: Nebula Station
            mapIdTransitionStr = "nebula_outpost";
            break;
        case 3:   // Level 3: Uranus Station
            mapIdTransitionStr = "urasnus_outpost";
            break;
        case 4:   // Level 4: Jupiter Orbit
            mapIdTransitionStr = "jupiter_outpost";
            break;
        default:
            mapIdTransitionStr = "nebula_outpost";
            break;
    }

    // Load the selected map
    load_map(mapIdTransitionStr);
    if (chunk_manager_) {
        chunk_manager_->setScrollSpeed(server_scroll_speed_, registry_.get());
    }

    // Apply map-specific theme (background color)
    apply_map_theme(next_level);

    // Load checkpoints for the level
    load_level_checkpoints(next_level);

    // Clear local entities (enemies, projectiles, bonuses) to prepare for new level
    if (registry_) {
        auto& enemies = registry_->get_components<Enemy>();
        std::vector<Entity> to_kill;
        for (size_t i = 0; i < enemies.size(); ++i) {
            to_kill.push_back(enemies.get_entity_at(i));
        }
        auto& projectiles = registry_->get_components<Projectile>();
        for (size_t i = 0; i < projectiles.size(); ++i) {
            to_kill.push_back(projectiles.get_entity_at(i));
        }
        auto& bonuses = registry_->get_components<Bonus>();
        for (size_t i = 0; i < bonuses.size(); ++i) {
            to_kill.push_back(bonuses.get_entity_at(i));
        }
        for (Entity e : to_kill) {
            registry_->kill_entity(e);
        }
    }

    // Also clear walls for level transition as map changes
    if (registry_) {
        auto& walls = registry_->get_components<Wall>();
        std::vector<Entity> to_kill;
        for (size_t i = 0; i < walls.size(); ++i) {
            to_kill.push_back(walls.get_entity_at(i));
        }
        for (Entity e : to_kill) {
            registry_->kill_entity(e);
        }
    }

    // Reset prediction system
    if (prediction_system_) {
        prediction_system_->reset();
    }

    // Reset local map scroll tracking
    map_scroll_x_ = 0.0;

    // Reset ChunkManager to clear old map chunks
    if (chunk_manager_) {
        chunk_manager_->reset(*registry_);
    }

    // Reset background entities position
    auto& positions = registry_->get_components<Position>();
    if (positions.has_entity(background1_)) positions[background1_].x = 0.0f;
    if (positions.has_entity(background2_)) positions[background2_].x = 0.0f;

    // Mark transition as applied
    pending_level_transition_apply_ = false;
    pending_level_id_ = 0;
}

void ClientGame::apply_pending_respawn() {
    if (!pending_respawn_apply_) return;

    std::cout << "[ClientGame] Applying respawn cleanup\n";

    // Reset prediction system to prevent "input buffer full" accumulation
    if (prediction_system_) {
        prediction_system_->reset();
    }

    // CRITICAL: Clear enemies, projectiles, and bonuses locally to sync with server reset
    // Note: Walls are NOT cleared as they persist on the server.
    if (registry_) {
        auto& enemies = registry_->get_components<Enemy>();
        std::vector<Entity> to_kill;
        for (size_t i = 0; i < enemies.size(); ++i) {
            to_kill.push_back(enemies.get_entity_at(i));
        }
        auto& projectiles = registry_->get_components<Projectile>();
        for (size_t i = 0; i < projectiles.size(); ++i) {
            to_kill.push_back(projectiles.get_entity_at(i));
        }
        auto& bonuses = registry_->get_components<Bonus>();
        for (size_t i = 0; i < bonuses.size(); ++i) {
            to_kill.push_back(bonuses.get_entity_at(i));
        }

        for (Entity e : to_kill) {
            registry_->kill_entity(e);
        }
    }

    // Mark respawn as applied
    pending_respawn_apply_ = false;
}

void ClientGame::setup_map_system() {
    if (!graphics_plugin_) {
        std::cerr << "[ClientGame] Cannot setup map system: graphics plugin not available\n";
        return;
    }
    
    // Create systems (will be configured when game starts with specific map)
    parallax_system_ = std::make_unique<rtype::ParallaxBackgroundSystem>(
        *graphics_plugin_, screen_width_, screen_height_
    );
    
    chunk_manager_ = std::make_unique<rtype::ChunkManagerSystem>(
        *graphics_plugin_, screen_width_, screen_height_
    );
    
    map_scroll_x_ = 0.0;
//     std::cout << "[ClientGame] Map systems created (waiting for map selection)\n";
}

void ClientGame::load_map(const std::string& mapId) {
//     std::cout << "[ClientGame] load_map() called with mapId: " << mapId << std::endl;
    
    if (!parallax_system_ || !chunk_manager_) {
        std::cerr << "[ClientGame] Map systems not initialized\n";
        return;
    }
    
    // Load map configuration by ID
    rtype::MapConfig config = rtype::MapConfigLoader::loadMapById(mapId);
    
    // Initialize parallax layers
    if (parallax_system_->initLayers(config.parallaxLayers)) {
//         std::cout << "[ClientGame] Parallax initialized: " << config.parallaxLayers.size() << " layers\n";
    } else {
        std::cerr << "[ClientGame] Failed to initialize parallax layers\n";
    }

    // Reset chunk manager to clean up old wall entities before reinitializing
    // This fixes collision issues on game restart where old hitboxes would persist
    chunk_manager_->reset(*registry_);

    // Initialize chunk manager
    chunk_manager_->initWithConfig(config);
    
    // Load tile sheet
    if (chunk_manager_->loadTileSheet(config.tileSheetPath)) {
//         std::cout << "[ClientGame] Tile sheet loaded: " << config.tileSheetPath << "\n";
    } else {
        std::cerr << "[ClientGame] Failed to load tile sheet\n";
    }
    
    // Load map segments
    std::string segmentsDir = config.basePath + "/segments";
    auto segmentPaths = rtype::MapConfigLoader::getSegmentPaths(segmentsDir);
    if (!segmentPaths.empty()) {
        chunk_manager_->loadSegments(segmentPaths);
//         std::cout << "[ClientGame] Loaded " << segmentPaths.size() << " segments\n";
    } else {
        std::cerr << "[ClientGame] No segments found in " << segmentsDir << "\n";
    }
    
    // Disable old background entities (they would render on top of new parallax)
    auto& sprites = registry_->get_components<::Sprite>();
    if (sprites.has_entity(background1_)) {
        sprites[background1_].texture = engine::INVALID_HANDLE;
    }
    if (sprites.has_entity(background2_)) {
        sprites[background2_].texture = engine::INVALID_HANDLE;
    }
    
    map_scroll_x_ = 0.0;
    current_map_id_str_ = mapId;
//     std::cout << "[ClientGame] Map loaded: " << config.name << "\n";
}

void ClientGame::setup_network_callbacks() {
    network_client_->set_on_accepted([this](uint32_t player_id) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !entity_manager_)
            return;
        entity_manager_->set_local_player_id(player_id);
        status_overlay_->set_connection("Connected (Player " + std::to_string(player_id) + ")");
        status_overlay_->refresh();

        // Initialize lag compensation system
        prediction_system_ = std::make_unique<ClientPredictionSystem>(player_id);
//         std::cout << "[ClientGame] Lag compensation system initialized for player " << player_id << "\n";

        // Don't auto-join lobby anymore - user will choose from menu
    });

    network_client_->set_on_rejected([this](uint8_t reason, const std::string& message) {
        status_overlay_->set_connection("Rejected: " + message);
        status_overlay_->refresh();
    });

    network_client_->set_on_lobby_state([this](const protocol::ServerLobbyStatePayload& state,
                                               const std::vector<protocol::PlayerLobbyEntry>& players_info) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !entity_manager_)
            return;
        int current = static_cast<int>(state.current_player_count);
        int required = static_cast<int>(state.required_player_count);
        status_overlay_->set_lobby("Lobby " + std::to_string(state.lobby_id) + ": " +
                                   std::to_string(current) + "/" + std::to_string(required));
        status_overlay_->refresh();

        for (const auto& entry : players_info) {
            std::string name(entry.player_name);
            if (name.empty())
                name = "Player " + std::to_string(entry.player_id);
            entity_manager_->set_player_name(entry.player_id, name);
        }

        // Update RoomLobbyScreen with all players (names + skins)
        if (menu_manager_) {
            menu_manager_->on_lobby_state(state, players_info);
        }
    });

    network_client_->set_on_countdown([this](uint8_t seconds) {
        status_overlay_->set_session("Starting in " + std::to_string(seconds) + "s");
        status_overlay_->refresh();
    });

    network_client_->set_on_game_start([this](uint32_t session_id, uint16_t udp_port, uint16_t map_id, float scroll_speed, uint32_t seed) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !registry_ || !entity_manager_ || !screen_manager_)
            return;
        (void)udp_port;
        status_overlay_->set_session("In game (session " + std::to_string(session_id) + ")");
        status_overlay_->refresh();
        entity_manager_->clear_all();
        screen_manager_->set_screen(GameScreen::PLAYING);
        server_scroll_speed_ = scroll_speed;

        // Emit scene change event for gameplay music
        registry_->get_event_bus().publish(ecs::SceneChangeEvent{
            ecs::SceneChangeEvent::SceneType::GAMEPLAY,
            static_cast<int>(map_id)
        });

        // Reset score at start of new game
        last_known_score_ = 0;

        // Convert numeric map ID to string ID
        // Map visual assets based on level
        std::string mapIdStr;
        switch (map_id) {
            case 0:   // Debug: Quick Test
            case 99:  // Debug: Instant Boss
                mapIdStr = "nebula_outpost";
                break;
            case 1:   // Level 1: Mars Assault
                mapIdStr = "mars_outpost";
                break;
            case 2:   // Level 2: Nebula Station
                mapIdStr = "nebula_outpost";
                break;
            case 3:   // Level 3: Uranus Station
                mapIdStr = "urasnus_outpost";
                break;
            case 4:   // Level 4: Jupiter Orbit
                mapIdStr = "jupiter_outpost";
                break;
            default:
                mapIdStr = "nebula_outpost";
                break;
        }

        // Load the selected map
        load_map(mapIdStr);
        if (chunk_manager_) {
            // Pass registry to update velocities of existing wall entities
            chunk_manager_->setScrollSpeed(server_scroll_speed_, registry_.get());
            
            // Set the map seed for procedural generation (ensure this happens AFTER map load/init)
            // load_map() resets the generator with config seed, so we must override it here
            chunk_manager_->setProceduralSeed(seed);
        }

        // Apply map-specific theme (background color)
        apply_map_theme(map_id);

        // Load checkpoints for the level
        load_level_checkpoints(map_id);

        auto& texts = registry_->get_components<UIText>();
        Entity status_entity = screen_manager_->get_status_entity();
        if (texts.has_entity(status_entity))
            texts[status_entity].active = false;

        // Reset GameState to PLAYING for the new game
        auto& gameStates = registry_->get_components<GameState>();
        for (size_t i = 0; i < gameStates.size(); ++i) {
            Entity entity = gameStates.get_entity_at(i);
            gameStates[entity].currentState = GameStateType::PLAYING;
            gameStates[entity].finalScore = 0;
        }

        auto& wave_controllers = registry_->get_components<WaveController>();
        if (wave_controllers.has_entity(wave_tracker_)) {
            WaveController& ctrl = wave_controllers[wave_tracker_];
            ctrl.currentWaveNumber = 1;
            ctrl.currentWaveIndex = 0;
            ctrl.allWavesCompleted = false;
            ctrl.totalScrollDistance = 0.0f;
        }
    });

    network_client_->set_on_player_name_updated([this](const protocol::ServerPlayerNameUpdatedPayload& payload) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !entity_manager_)
            return;
        std::string name(payload.new_name, strnlen(payload.new_name, sizeof(payload.new_name)));
        uint32_t playerId = payload.player_id;
        entity_manager_->set_player_name(playerId, name);

        if (menu_manager_) {
            menu_manager_->on_player_name_updated(payload);
        }
    });

    network_client_->set_on_player_skin_updated([this](const protocol::ServerPlayerSkinUpdatedPayload& payload) {
        if (menu_manager_) {
            menu_manager_->on_player_skin_updated(payload);
        }
    });

    network_client_->set_on_entity_spawn([this](const protocol::ServerEntitySpawnPayload& spawn) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !entity_manager_)
            return;
        uint32_t server_id = ntohl(spawn.entity_id);
        uint16_t health = ntohs(spawn.health);
        entity_manager_->spawn_or_update_entity(server_id, spawn.entity_type,
                                                spawn.spawn_x, spawn.spawn_y, health, spawn.subtype);
    });

    network_client_->set_on_entity_destroy([this](const protocol::ServerEntityDestroyPayload& destroy) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !entity_manager_)
            return;
        uint32_t server_id = ntohl(destroy.entity_id);
        entity_manager_->remove_entity(server_id);
    });

    network_client_->set_on_wave_start([this](const protocol::ServerWaveStartPayload& wave) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !registry_)
            return;
        auto& wave_controllers = registry_->get_components<WaveController>();
        if (!wave_controllers.has_entity(wave_tracker_))
            return;
        WaveController& ctrl = wave_controllers[wave_tracker_];
        int old_wave = ctrl.currentWaveNumber;
        ctrl.currentWaveNumber = static_cast<int>(ntohl(wave.wave_number));
        ctrl.totalWaveCount = ntohs(wave.total_waves);
        ctrl.totalScrollDistance = wave.scroll_distance;
        ctrl.allWavesCompleted = false;
        std::cout << "[CLIENT] 🌊 Wave START: " << old_wave << " → " << ctrl.currentWaveNumber << "\n";
    });

    network_client_->set_on_wave_complete([this](const protocol::ServerWaveCompletePayload& wave) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !registry_)
            return;
        auto& wave_controllers = registry_->get_components<WaveController>();
        if (!wave_controllers.has_entity(wave_tracker_))
            return;
        WaveController& ctrl = wave_controllers[wave_tracker_];
        int old_wave = ctrl.currentWaveNumber;
        ctrl.currentWaveNumber = static_cast<int>(ntohl(wave.wave_number));
        if (wave.all_waves_complete)
            ctrl.currentWaveNumber = static_cast<int>(ctrl.totalWaveCount);
        ctrl.allWavesCompleted = wave.all_waves_complete != 0;
        std::cout << "[CLIENT] ✅ Wave COMPLETE: " << old_wave << " → " << ctrl.currentWaveNumber << "\n";
    });

    network_client_->set_on_score_update([this](const protocol::ServerScoreUpdatePayload& score) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !entity_manager_ || !registry_)
            return;
        // Use entity_id from payload to find the correct entity (like level-up does)
        uint32_t server_entity_id = score.entity_id;
        uint32_t player_id = score.player_id;

        std::cout << "[CLIENT] Received score update - player_id: " << player_id
                  << ", entity_id: " << server_entity_id
                  << ", score: " << score.new_total_score
                  << ", has_entity: " << entity_manager_->has_entity(server_entity_id) << std::endl;

        if (entity_manager_->has_entity(server_entity_id)) {
            Entity entity = entity_manager_->get_entity(server_entity_id);
            auto& scores = registry_->get_components<Score>();

            std::cout << "[CLIENT]   Local entity: " << entity
                      << ", has_score_component: " << scores.has_entity(entity) << std::endl;

            if (scores.has_entity(entity)) {
                scores[entity].value = score.new_total_score;
                std::cout << "[CLIENT] ✅ Score updated for player " << player_id
                          << " (server_entity " << server_entity_id
                          << ", local_entity " << entity << "): " << score.new_total_score << std::endl;

                // Save the score for game over screen only if this is the local player
                uint32_t local_server_id = entity_manager_->get_local_player_entity_id();
                if (server_entity_id == local_server_id) {
                    last_known_score_ = static_cast<int>(score.new_total_score);
                }
            } else {
                std::cout << "[CLIENT] ❌ Entity " << entity << " has no Score component!" << std::endl;
            }
        } else {
            std::cout << "[CLIENT] ❌ No entity found for server_entity_id " << server_entity_id << std::endl;
        }

        // Update HUDSystem scoreboard with player scores
        if (registry_->has_system<HUDSystem>()) {
            std::string player_name = entity_manager_->get_player_name(player_id);
            registry_->get_system<HUDSystem>().update_player_score(player_id, player_name, score.new_total_score);
        }
    });

    network_client_->set_on_player_level_up([this](const protocol::ServerPlayerLevelUpPayload& level_up) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !entity_manager_)
            return;
        // Update the player's sprite with the new skin
        entity_manager_->update_player_skin(level_up.entity_id, level_up.new_skin_id);

        // Check if this is the local player for HUD feedback
        uint32_t local_server_id = entity_manager_->get_local_player_entity_id();
        if (level_up.entity_id == local_server_id) {
            std::cout << "[ClientGame] Local player leveled up to level "
                      << static_cast<int>(level_up.new_level) << "!\n";
            // TODO: Trigger level-up visual effect in HUD
        }
    });

    network_client_->set_on_powerup_collected([this](const protocol::ServerPowerupCollectedPayload& powerup) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !entity_manager_ || !registry_)
            return;

        // Find the player entity
        if (!entity_manager_->has_entity(powerup.player_id))
            return;

        Entity playerEntity = entity_manager_->get_entity(powerup.player_id);

        // Handle SHIELD powerup - add visual circle effect
        if (powerup.powerup_type == protocol::PowerupType::SHIELD) {
            // Add Shield component if not present
            auto& shields = registry_->get_components<Shield>();
            if (!shields.has_entity(playerEntity)) {
                registry_->add_component(playerEntity, Shield{true});

                // Add visual effect (purple circle around player)
                auto& colliders = registry_->get_components<Collider>();
                float shieldRadius = 40.0f;  // Default radius
                if (colliders.has_entity(playerEntity)) {
                    const auto& col = colliders[playerEntity];
                    shieldRadius = std::max(col.width, col.height) / 2.0f + 15.0f;
                }

                registry_->add_component(playerEntity, CircleEffect{
                    shieldRadius,
                    engine::Color::ShieldViolet,
                    0.0f, 0.0f,
                    true,
                    CircleEffect::DEFAULT_LAYER
                });
                std::cout << "[ClientGame] Shield visual effect added for player " << powerup.player_id << std::endl;
            }
            return;
        }

        // Handle WEAPON_UPGRADE type (bonus weapon / companion turret)
        if (powerup.powerup_type == protocol::PowerupType::WEAPON_UPGRADE) {
            // Publish event to CompanionSystem (ECS architecture)
            registry_->get_event_bus().publish(ecs::CompanionSpawnEvent{playerEntity, powerup.player_id});
        }
    });

    network_client_->set_on_shield_broken([this](uint32_t player_id) {
        std::cout << "[ClientGame] Shield broken callback received for player_id=" << player_id << std::endl;

        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !entity_manager_ || !registry_) {
            std::cout << "[ClientGame] Shield broken: early return (shutdown or null pointers)" << std::endl;
            return;
        }

        // Find the player entity
        if (!entity_manager_->has_entity(player_id)) {
            std::cout << "[ClientGame] Shield broken: player_id " << player_id << " not found in entity_manager" << std::endl;
            return;
        }

        Entity playerEntity = entity_manager_->get_entity(player_id);
        std::cout << "[ClientGame] Shield broken: found entity " << playerEntity << " for player_id " << player_id << std::endl;

        // Remove Shield component
        auto& shields = registry_->get_components<Shield>();
        if (shields.has_entity(playerEntity)) {
            registry_->remove_component<Shield>(playerEntity);
            std::cout << "[ClientGame] Shield component removed from entity " << playerEntity << std::endl;
        } else {
            std::cout << "[ClientGame] No Shield component on entity " << playerEntity << std::endl;
        }

        // Remove CircleEffect (visual purple circle)
        if (registry_->has_component_registered<CircleEffect>()) {
            auto& circles = registry_->get_components<CircleEffect>();
            if (circles.has_entity(playerEntity)) {
                registry_->remove_component<CircleEffect>(playerEntity);
                std::cout << "[ClientGame] CircleEffect removed from entity " << playerEntity << std::endl;
            } else {
                std::cout << "[ClientGame] No CircleEffect on entity " << playerEntity << std::endl;
            }
        } else {
            std::cout << "[ClientGame] CircleEffect component not registered!" << std::endl;
        }

        std::cout << "[ClientGame] Shield broken processing complete for player " << player_id << std::endl;
    });

    network_client_->set_on_projectile_spawn([this](const protocol::ServerProjectileSpawnPayload& proj) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !entity_manager_ || !registry_)
            return;
        uint32_t proj_id = ntohl(proj.projectile_id);
        uint32_t owner_id = ntohl(proj.owner_id);

        // Determine projectile type based on owner's entity type
        protocol::EntityType proj_type = protocol::EntityType::PROJECTILE_PLAYER;

        // Check owner's entity type to see if it's an enemy
        protocol::EntityType owner_type;
        if (entity_manager_->get_entity_type(owner_id, owner_type)) {
            // If owner is an enemy, create PROJECTILE_ENEMY
            if (owner_type == protocol::EntityType::ENEMY_BASIC ||
                owner_type == protocol::EntityType::ENEMY_FAST ||
                owner_type == protocol::EntityType::ENEMY_TANK ||
                owner_type == protocol::EntityType::ENEMY_BOSS) {
                proj_type = protocol::EntityType::PROJECTILE_ENEMY;
            }
        }

        Entity entity = entity_manager_->spawn_or_update_entity(proj_id, proj_type,
                                                                proj.spawn_x, proj.spawn_y, 1, 0);

        auto& velocities = registry_->get_components<Velocity>();
        float vel_x = static_cast<float>(proj.velocity_x);
        float vel_y = static_cast<float>(proj.velocity_y);
        if (velocities.has_entity(entity)) {
            velocities[entity].x = vel_x;
            velocities[entity].y = vel_y;
        } else {
            registry_->add_component(entity, Velocity{vel_x, vel_y});
        }
    });

    network_client_->set_on_explosion([this](const protocol::ServerExplosionPayload& explosion) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !entity_manager_)
            return;
        float x = explosion.position_x;
        float y = explosion.position_y;
        float scale = explosion.effect_scale <= 0.0f ? 1.0f : explosion.effect_scale;
        entity_manager_->spawn_explosion(x, y, scale);
    });

    network_client_->set_on_snapshot([this](const protocol::ServerSnapshotPayload& header,
                                            const std::vector<protocol::EntityState>& entities) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !registry_ || !entity_manager_)
            return;
        auto& positions = registry_->get_components<Position>();
        auto& velocities = registry_->get_components<Velocity>();
        auto& healths = registry_->get_components<Health>();
        auto& last_states = registry_->get_components<LastServerState>();
        std::unordered_set<uint32_t> updated_ids;
        updated_ids.reserve(entities.size());

        // Store snapshot in interpolation system for remote entities
        uint32_t server_tick = ntohl(header.server_tick);
        uint32_t local_player_id = entity_manager_->get_local_player_entity_id();
        if (interpolation_system_) {
            interpolation_system_->on_snapshot_received(server_tick, entities, local_player_id);
        }

        // Synchronize scroll position from server for visual tile rendering
        // Wall collisions are handled server-side only, but tiles need to render at correct position
        // Skip scroll updates during level transitions to prevent chunk loading desync
        if (!level_transition_in_progress_) {
            double server_scroll = static_cast<double>(header.scroll_x);
            if (chunk_manager_) {
                // Force chunk manager scroll to match server exactly
                // This ensures chunk loading/unloading decisions use the same scroll as rendering
                chunk_manager_->setScrollX(server_scroll);
            }
            map_scroll_x_ = server_scroll;
        }

        for (const auto& state : entities) {
            uint32_t server_id = ntohl(state.entity_id);
            updated_ids.insert(server_id);

            if (!entity_manager_->has_entity(server_id))
                continue;

            Entity entity = entity_manager_->get_entity(server_id);

            // Vérifier si c'est le joueur local
            bool is_local = (server_id == entity_manager_->get_local_player_entity_id());

            // Sauvegarder l'état serveur pour extrapolation du joueur local
            if (is_local) {
                if (!last_states.has_entity(entity)) {
                    LastServerState state_component;
                    state_component.x = state.position_x;
                    state_component.y = state.position_y;
                    state_component.timestamp = current_time_;
                    registry_->add_component(entity, state_component);
                } else {
                    LastServerState& last_state = last_states[entity];
                    last_state.x = state.position_x;
                    last_state.y = state.position_y;
                    last_state.timestamp = current_time_;
                }

                // SERVER RECONCILIATION for local player
                if (prediction_system_ && positions.has_entity(entity)) {
                    // Acknowledge inputs up to the last processed sequence
                    uint32_t last_ack = ntohl(state.last_ack_sequence);
                    if (last_ack > 0) {
                        prediction_system_->acknowledge_input(last_ack);

                        // Check prediction error
                        Position& local_pos = positions[entity];
                        float error_x = std::abs(local_pos.x - state.position_x);
                        float error_y = std::abs(local_pos.y - state.position_y);
                        float error_distance = std::sqrt(error_x * error_x + error_y * error_y);

                        constexpr float ERROR_THRESHOLD = 10.0f;  // pixels (increased for smoother gameplay)
                        constexpr float LARGE_ERROR_THRESHOLD = 50.0f;  // pixels (teleport if too far)

                        if (error_distance > ERROR_THRESHOLD) {
                            // Record correction for debug overlay
                            if (debug_network_overlay_) {
                                debug_network_overlay_->record_correction();
                            }

                            // If error is very large (>50px), teleport immediately (likely a collision or major desync)
                            if (error_distance > LARGE_ERROR_THRESHOLD) {
                                // std::cout << "[RECONCILIATION] Large correction (teleport): " << error_distance
                                //           << " pixels (seq: " << last_ack << ")\n";
                                local_pos.x = state.position_x;
                                local_pos.y = state.position_y;
                            } else {
                                // Small-medium error: smooth correction over time (lerp)
                                // Apply 30% of the correction per frame for smooth adjustment
                                constexpr float CORRECTION_SPEED = 0.3f;
                                float correction_x = (state.position_x - local_pos.x) * CORRECTION_SPEED;
                                float correction_y = (state.position_y - local_pos.y) * CORRECTION_SPEED;

                                local_pos.x += correction_x;
                                local_pos.y += correction_y;

                                // Only log significant corrections
                                if (error_distance > 15.0f) {
                                    // std::cout << "[RECONCILIATION] Smooth correction: " << error_distance
                                    //           << " pixels (applying " << (CORRECTION_SPEED * 100) << "%)\n";
                                }
                            }
                        }

                        // Update debug overlay positions
                        if (debug_network_overlay_) {
                            debug_network_overlay_->set_server_position(state.position_x, state.position_y);
                            debug_network_overlay_->set_predicted_position(local_pos.x, local_pos.y);
                        }
                    }
                }
            }

            // Mettre à jour la position (pour les entités distantes, utiliser interpolation)
            if (!is_local) {
                // Try to get interpolated position first
                float interp_x, interp_y;
                bool has_interpolation = interpolation_system_ &&
                    interpolation_system_->get_interpolated_position(server_id, server_tick, interp_x, interp_y);

                if (has_interpolation) {
                    // Use interpolated position for smooth movement
                    if (positions.has_entity(entity)) {
                        positions[entity].x = interp_x;
                        positions[entity].y = interp_y;
                    } else {
                        registry_->add_component(entity, Position{interp_x, interp_y});
                    }
                } else {
                    // Fallback: use direct server position if no interpolation data yet
                    if (positions.has_entity(entity)) {
                        positions[entity].x = state.position_x;
                        positions[entity].y = state.position_y;
                    } else {
                        registry_->add_component(entity, Position{state.position_x, state.position_y});
                    }
                }
            }

            // Mettre à jour la vélocité SEULEMENT pour les autres joueurs
            // Le joueur local garde sa vélocité prédite pour éviter le stuttering
            if (!is_local) {
                float vel_x = static_cast<float>(state.velocity_x) / 10.0f;
                float vel_y = static_cast<float>(state.velocity_y) / 10.0f;
                if (velocities.has_entity(entity)) {
                    velocities[entity].x = vel_x;
                    velocities[entity].y = vel_y;
                } else {
                    registry_->add_component(entity, Velocity{vel_x, vel_y});
                }
            }

            uint16_t hp = ntohs(state.health);
            if (healths.has_entity(entity)) {
                healths[entity].current = static_cast<int>(hp);
                healths[entity].max = std::max(healths[entity].max, static_cast<int>(hp));
            } else {
                Health comp;
                comp.current = static_cast<int>(hp);
                comp.max = static_cast<int>(hp);
                registry_->add_component(entity, comp);
            }
        }

        entity_manager_->process_snapshot_update(updated_ids);
    });

    network_client_->set_on_game_over([this](const protocol::ServerGameOverPayload& result) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !registry_ || !screen_manager_)
            return;
        bool victory = result.result == protocol::GameResult::VICTORY;
        status_overlay_->set_session(victory ? "Victory" : "Defeat");
        status_overlay_->refresh();

        // Get final score from local player
        // Try to get from entity first (if still alive), otherwise use cached value
        int final_score = last_known_score_;
        auto& scores = registry_->get_components<Score>();
        auto& localPlayers = registry_->get_components<LocalPlayer>();

        for (size_t i = 0; i < localPlayers.size(); ++i) {
            Entity playerEntity = localPlayers.get_entity_at(i);
            if (scores.has_entity(playerEntity)) {
                final_score = scores[playerEntity].value;
                // Update cached value
                last_known_score_ = final_score;
                break;
            }
        }

        // Set GameState to trigger HUD hiding
        auto& gameStates = registry_->get_components<GameState>();
        for (size_t i = 0; i < gameStates.size(); ++i) {
            Entity entity = gameStates.get_entity_at(i);
            gameStates[entity].currentState = victory ? GameStateType::VICTORY : GameStateType::GAME_OVER;
            gameStates[entity].finalScore = final_score;
        }

        // Emit scene change event for victory/game over music
        registry_->get_event_bus().publish(ecs::SceneChangeEvent{
            victory ? ecs::SceneChangeEvent::SceneType::VICTORY
                    : ecs::SceneChangeEvent::SceneType::GAME_OVER,
            0
        });

        screen_manager_->show_result(victory, final_score);
    });

    network_client_->set_on_leaderboard([this](const protocol::ServerLeaderboardPayload& header,
                                               const std::vector<protocol::LeaderboardEntry>& entries) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !registry_ || !screen_manager_)
            return;
        std::cout << "[ClientGame] Received leaderboard with " << static_cast<int>(header.entry_count)
                  << " entries" << std::endl;

        // Convert to LeaderboardEntryData and publish event
        std::vector<ecs::LeaderboardEntryData> displayEntries;
        for (const auto& entry : entries) {
            ecs::LeaderboardEntryData data;
            data.player_id = entry.player_id;
            data.player_name = std::string(entry.player_name, strnlen(entry.player_name, sizeof(entry.player_name)));
            data.score = entry.score;
            data.rank = entry.rank;
            displayEntries.push_back(data);
        }

        registry_->get_event_bus().publish(ecs::LeaderboardReceivedEvent{displayEntries});

        // Also update ScreenManager for result screen display
        std::vector<ResultLeaderboardEntry> screenEntries;
        for (const auto& entry : entries) {
            ResultLeaderboardEntry e;
            e.player_id = entry.player_id;
            e.player_name = std::string(entry.player_name, strnlen(entry.player_name, sizeof(entry.player_name)));
            e.score = entry.score;
            e.rank = entry.rank;
            screenEntries.push_back(e);
        }
        screen_manager_->set_leaderboard(screenEntries);
    });

    network_client_->set_on_disconnected([this]() {
        // Only set running flag, don't touch registry from background thread
        running_ = false;
    });

    // Player respawn callback
    network_client_->set_on_player_respawn([this](const protocol::ServerPlayerRespawnPayload& payload) {
        // Guard against callbacks during shutdown
        if (is_shutting_down_ || !entity_manager_ || !registry_)
            return;

        uint32_t player_id = ntohl(payload.player_id);
        float respawn_x = payload.respawn_x;
        float respawn_y = payload.respawn_y;
        uint16_t invuln_duration_ms = ntohs(payload.invulnerability_duration);
        float invuln_duration = invuln_duration_ms / 1000.0f;
        uint8_t lives = payload.lives_remaining;

//         std::cout << "[ClientGame] Player " << player_id << " respawning at ("
//                   << respawn_x << ", " << respawn_y << ") with "
//                   << static_cast<int>(lives) << " lives and "
//                   << invuln_duration << "s invulnerability\n";

        // Update HUD lives display
        if (registry_->has_system<HUDSystem>()) {
            registry_->get_system<HUDSystem>().update_lives(*registry_, lives);
        }

        // If this is our local player, prepare for respawn
        if (player_id == network_client_->get_player_id()) {
            std::cout << "[ClientGame] Local player respawned! Will apply changes when screen is black.\n";

            // Trigger visual reset (fade to black)
            fade_trigger_ = true;

            // Mark respawn as pending - will be applied when screen is fully black
            pending_respawn_apply_ = true;
        }
    });

    // Level transition callback
    network_client_->set_on_level_transition([this](const protocol::ServerLevelTransitionPayload& payload) {
        // Store pending transition data - will be applied when screen is fully black
        pending_level_id_ = payload.next_level_id;
        pending_level_transition_apply_ = true;

        // Trigger visual reset (fade to black)
        fade_trigger_ = true;

        // Enable transition lock to prevent scroll desync during map reload
        level_transition_in_progress_ = true;
        level_transition_timer_ = 0.0f;
        level_ready_received_ = false;  // Wait for server to signal level is ready

        std::cout << "[ClientGame] Level transition to Level " << pending_level_id_ << " (will apply when screen is black)\n";
    });

    // Level ready callback - server signals level is fully loaded
    network_client_->set_on_level_ready([this](const protocol::ServerLevelReadyPayload& payload) {
        std::cout << "[ClientGame] Level " << payload.level_id << " ready - ending fade\n";
        level_ready_received_ = true;
    });
}

void ClientGame::run() {
    // Re-register chat callback to ensure it points to this instance's chat overlay
    // (RoomLobbyScreen may have overwritten it during room phase)
    network_client_->set_on_chat_message([this](uint32_t sender_id, const std::string& sender_name, const std::string& message) {
        if (chat_overlay_) {
            chat_overlay_->add_message(sender_id, sender_name, message);
        }
    });

    // Track menu state to detect transition from menu to game
    bool was_in_menu = true;

    auto last_frame = std::chrono::steady_clock::now();
    auto last_input_send = last_frame;
    auto last_ping = last_frame;
    auto last_overlay_update = last_frame;

    while (running_ && graphics_plugin_->is_window_open()) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last_frame).count();
        last_frame = now;

        // Mettre à jour le temps écoulé pour l'extrapolation
        current_time_ += dt;

        // Release transition lock when server signals level is ready (or after 10s timeout as fallback)
        if (level_transition_in_progress_) {
            level_transition_timer_ += dt;
            if (level_ready_received_ || level_transition_timer_ >= 10.0f) {
                level_transition_in_progress_ = false;
            }
        }

        network_plugin_->update(dt);
        network_client_->update();

        // Only quit game with Escape if console overlay is not visible
        // (Console uses Escape to close, chat uses F1)
        if (input_handler_->is_escape_pressed() && !console_overlay_->is_visible()) {
            running_ = false;
            break;
        }

        // Toggle debug visualization with H key (colliders + spawn points)
        // Toggle debug visualization with H key (colliders + spawn points)
        // DISABLED: User request to deactivate hitbox debug menu
        /*
        if (input_handler_->is_hitbox_toggle_pressed()) {
            bool new_state = false;

            // Toggle collider debug
            if (registry_->has_system<ColliderDebugSystem>()) {
                auto& debug_system = registry_->get_system<ColliderDebugSystem>();
                new_state = !debug_system.is_enabled();
                debug_system.set_enabled(new_state);
            }
        }
        */

        // Toggle admin console with Tab key
        static bool tab_was_pressed = false;
        bool tab_pressed = input_plugin_->is_key_pressed(engine::Key::Tab);
        if (tab_pressed && !tab_was_pressed && !chat_overlay_->is_visible())
            console_overlay_->toggle();
        tab_was_pressed = tab_pressed;
        if (console_overlay_->is_visible())
            console_overlay_->update(graphics_plugin_, input_plugin_);

        // Chat handling is done later, after in_menu is determined
        // This prevents double-updating when RoomLobbyScreen has its own chat
        if (input_handler_->is_network_debug_toggle_pressed()) {
            if (debug_network_overlay_) {
                bool new_state = !debug_network_overlay_->is_enabled();
                debug_network_overlay_->set_enabled(new_state);
//                 std::cout << "[ClientGame] Network debug overlay "
//                           << (new_state ? "enabled" : "disabled") << "\n";
            }
        }

        // Toggle scoreboard visibility with Tab key (hold to show)
        if (registry_->has_system<HUDSystem>()) {
            bool scoreboard_pressed = input_handler_->is_scoreboard_pressed();
            registry_->get_system<HUDSystem>().set_scoreboard_visible(scoreboard_pressed);
        }

        if (network_client_->is_in_game() &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_input_send).count() >= 15) {

            // Disable game inputs when chat or console is open
            uint16_t input_flags = 0;
            bool overlay_blocking_input = (chat_overlay_ && chat_overlay_->is_visible()) ||
                                          (console_overlay_ && console_overlay_->is_visible());

            if (!overlay_blocking_input) {
                input_flags = input_handler_->gather_input();

                // Client-side shoot sound with cooldown
                bool is_shooting = (input_flags & static_cast<uint16_t>(protocol::InputFlags::INPUT_SHOOT)) != 0;
                shoot_sound_cooldown_ -= dt;
                if (is_shooting && shoot_sound_cooldown_ <= 0.0f) {
                    // Emit shoot sound event
                    registry_->get_event_bus().publish(ecs::ShotFiredEvent{0, 0});
                    shoot_sound_cooldown_ = 0.15f; // 150ms cooldown between sounds
                }

                // NOUVEAU: Appliquer la vélocité localement AVANT d'envoyer au serveur
                apply_input_to_local_player(input_flags);
            }

            // Send input to server (send 0 if overlay is open to stop movement)
            network_client_->send_input(input_flags, client_tick_++);

            // Store input in prediction system for reconciliation
            if (prediction_system_) {
                uint32_t sequence = network_client_->get_last_input_sequence();
                uint32_t timestamp = static_cast<uint32_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
                );
                prediction_system_->on_input_sent(sequence, input_flags, timestamp);
            }

            last_input_send = now;
        }

        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_ping).count() >= 5) {
            network_client_->send_ping();
            last_ping = now;
        }

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_overlay_update).count() >= 500) {
            status_overlay_->set_ping(network_plugin_->get_server_ping());
            status_overlay_->refresh();
            last_overlay_update = now;
        }

        // Check current screen
        GameScreen current_screen = screen_manager_->get_current_screen();
        bool in_menu = (current_screen == GameScreen::CONNECTION ||
                       current_screen == GameScreen::MAIN_MENU ||
                       current_screen == GameScreen::CREATE_ROOM ||
                       current_screen == GameScreen::BROWSE_ROOMS ||
                       current_screen == GameScreen::ROOM_LOBBY ||
                       current_screen == GameScreen::SETTINGS ||
                       current_screen == GameScreen::GLOBAL_LEADERBOARD);
        bool in_result = (current_screen == GameScreen::VICTORY ||
                         current_screen == GameScreen::DEFEAT);

        // Re-register chat callback when transitioning from menu to game
        // This ensures ClientGame's ChatOverlay receives messages, not RoomLobbyScreen's
        if (was_in_menu && !in_menu) {
            network_client_->set_on_chat_message([this](uint32_t sender_id, const std::string& sender_name, const std::string& message) {
                if (chat_overlay_) {
                    chat_overlay_->add_message(sender_id, sender_name, message);
                }
            });
        }
        was_in_menu = in_menu;

        // Toggle chat with T key (only to OPEN), F1 to close
        // ONLY handle chat in ClientGame when NOT in menu (RoomLobbyScreen has its own chat)
        if (!in_menu && chat_overlay_) {
            static bool t_was_pressed = false;
            static bool f1_was_pressed = false;
            bool t_pressed = input_plugin_->is_key_pressed(engine::Key::T);
            bool f1_pressed = input_plugin_->is_key_pressed(engine::Key::F1);

            // Open chat with T (only when not already open)
            if (t_pressed && !t_was_pressed && !console_overlay_->is_visible() && !chat_overlay_->is_visible()) {
                chat_overlay_->set_visible(true);
            }
            t_was_pressed = t_pressed;

            // Close chat with F1
            if (f1_pressed && !f1_was_pressed && chat_overlay_->is_visible()) {
                chat_overlay_->set_visible(false);
            }
            f1_was_pressed = f1_pressed;

            if (chat_overlay_->is_visible())
                chat_overlay_->update(graphics_plugin_, input_plugin_);
        }
        entity_manager_->update_projectiles(dt);
        entity_manager_->update_name_tags();

        // Update bullet animations (cycle through 3 frames)
        auto& bulletAnimations = registry_->get_components<BulletAnimation>();
        auto& sprites = registry_->get_components<::Sprite>();
        
        for (size_t i = 0; i < bulletAnimations.size(); i++) {
            Entity entity = bulletAnimations.get_entity_at(i);

            if (!bulletAnimations.has_entity(entity))
                continue;

            auto& bulletAnim = bulletAnimations[entity];
            bulletAnim.timer += dt;

            // Switch frame every 0.1 seconds
            if (bulletAnim.timer >= bulletAnim.frameDuration) {
                bulletAnim.timer -= bulletAnim.frameDuration;
                bulletAnim.currentFrame = (bulletAnim.currentFrame + 1) % 3;  // Cycle 0->1->2->0
                
                // Update sprite source_rect
                if (sprites.has_entity(entity)) {
                    auto& sprite = sprites[entity];
                    // Frame 0 at x=0, Frame 1 at x=16, Frame 2 at x=32 (each 16x16 in 48x16 image)
                    sprite.source_rect.x = bulletAnim.currentFrame * 16.0f;
                    sprite.source_rect.y = 0.0f;
                    sprite.source_rect.width = 16.0f;
                    sprite.source_rect.height = 16.0f;
                }
            }
        }
        
        // Shot animations are now handled by MuzzleFlashSystem (ECS architecture)

        auto& explosionAnimations = registry_->get_components<ExplosionAnimation>();
        for (size_t i = 0; i < explosionAnimations.size(); ++i) {
            Entity entity = explosionAnimations.get_entity_at(i);
            if (!explosionAnimations.has_entity(entity))
                continue;

            auto& explosionAnim = explosionAnimations[entity];
            explosionAnim.timer += dt;

            bool advance_frame = false;
            if (explosionAnim.frameDuration > 0.0f) {
                if (explosionAnim.timer >= explosionAnim.frameDuration) {
                    explosionAnim.timer -= explosionAnim.frameDuration;
                    advance_frame = true;
                }
            } else {
                advance_frame = true;
            }

            if (advance_frame) {
                explosionAnim.currentFrame++;

                if (explosionAnim.currentFrame >= explosionAnim.totalFrames) {
                    registry_->kill_entity(entity);
                    continue;
                }

                if (sprites.has_entity(entity)) {
                    auto& sprite = sprites[entity];
                    int framesPerRow = std::max(1, explosionAnim.framesPerRow);
                    int column = explosionAnim.currentFrame % framesPerRow;
                    int row = explosionAnim.currentFrame / framesPerRow;
                    sprite.source_rect.x = static_cast<float>(column * explosionAnim.frameWidth);
                    sprite.source_rect.y = static_cast<float>(row * explosionAnim.frameHeight);
                    sprite.source_rect.width = static_cast<float>(explosionAnim.frameWidth);
                    sprite.source_rect.height = static_cast<float>(explosionAnim.frameHeight);
                }
            }
        }

        if (in_menu) {
            // Menu screen - only update and draw menu
            menu_manager_->update(graphics_plugin_, input_plugin_);

            // Sync menu screen with screen manager
            if (menu_manager_->get_current_screen() != current_screen) {
                screen_manager_->set_screen(menu_manager_->get_current_screen());
            }

            menu_manager_->draw(graphics_plugin_);
        } else if (in_result) {
            // Result screen (victory/defeat) - update button and render
            screen_manager_->update_result_screen(graphics_plugin_, input_plugin_);
            registry_->run_systems(dt);
        } else {
            // Game screen - run game systems

            // Update map scrolling for smooth visual rendering
            // Note: chunk_manager uses confirmedScrollX (from server snapshots) for chunk loading decisions
            // but renderScrollX (extrapolated locally) for smooth visual display
            float scrollDelta = server_scroll_speed_ * dt;
            map_scroll_x_ += scrollDelta;

            // Update parallax and chunk states
            if (parallax_system_) {
                parallax_system_->updateScroll(scrollDelta);
            }
            if (chunk_manager_) {
                // Advance render scroll for smooth visual interpolation between server updates
                chunk_manager_->advanceRenderScroll(scrollDelta);
                // Update loads/unloads chunks based on confirmedScrollX (set by server snapshots)
                chunk_manager_->update(*registry_, dt);
            }
            
            if (registry_->has_system<LocalPredictionSystem>()) {
                auto& prediction = registry_->get_system<LocalPredictionSystem>();
                prediction.set_current_time(current_time_);
            }
            
            // Clear screen first (RenderSystem will skip clear since we did it)
            graphics_plugin_->clear(engine::Color{20, 20, 30, 255});
            
            // Render parallax background (behind everything)
            if (parallax_system_ && parallax_system_->isInitialized()) {
                parallax_system_->render();
            }
            
            // Render tile chunks (uses internal scroll position for consistency)
            if (chunk_manager_ && chunk_manager_->isInitialized()) {
                chunk_manager_->render();
            }
            
            // Tell RenderSystem to skip clear (we already cleared + rendered background)
            if (registry_->has_system<RenderSystem>()) {
                auto& render_sys = registry_->get_system<RenderSystem>();
                render_sys.set_skip_clear(true);
            }

            // Render HUD background image BEFORE entities (so entities render on top)
            if (!hud_loaded_) {
                hud_texture_ = graphics_plugin_->load_texture("assets/sprite/ui-hud.png");
                hud_loaded_ = true;
            }

            if (hud_loaded_ && hud_texture_ != engine::INVALID_HANDLE) {
                engine::Sprite hud_sprite;
                hud_sprite.texture_handle = hud_texture_;
                hud_sprite.size = {static_cast<float>(screen_width_), static_cast<float>(screen_height_)};
                hud_sprite.origin = {0.0f, 0.0f};
                hud_sprite.tint = {255, 255, 255, 230};  // 230 alpha for slight transparency

                graphics_plugin_->draw_sprite(hud_sprite, {0.0f, 0.0f});
            }

            // Run ECS systems (RenderSystem will NOT clear since skip_clear is set)
            // HUDSystem will render text on top of the HUD background image
            registry_->run_systems(dt);
        }

        // Render debug network overlay (if enabled)
        if (debug_network_overlay_ && prediction_system_) {
            // Update metrics
            float rtt = static_cast<float>(network_plugin_->get_server_ping());
            size_t buffer_size = prediction_system_->get_pending_inputs().size();
            debug_network_overlay_->update_metrics(rtt, 0, buffer_size);  // corrections_per_second updated elsewhere

            // Render overlay
            debug_network_overlay_->render(*graphics_plugin_);
        }

        if (in_result)
            screen_manager_->draw_result_screen(graphics_plugin_);

        // Equalize the fade logic here
        // --- Visual Reset Effect (Fade to Black then back) ---
        // States: 0 = none, 1 = fading in (to black), 2 = waiting for level ready, 3 = fading out (to visible)

        if (fade_trigger_) {
            fade_state_ = 1;  // Start fading IN (to black)
            fade_alpha_ = 0.0f;
            fade_trigger_ = false;
        }

        if (fade_state_ > 0) {
            // Draw fullscreen black rectangle
            engine::Rectangle func_rect{0.0f, 0.0f, static_cast<float>(screen_width_), static_cast<float>(screen_height_)};
            uint8_t alpha_byte = static_cast<uint8_t>(fade_alpha_ * 255.0f);
            graphics_plugin_->draw_rectangle(func_rect, engine::Color{0, 0, 0, alpha_byte});

            // Draw players on top of the fade overlay (so they remain visible)
            if (registry_ && fade_alpha_ > 0.5f) {  // Only when screen is mostly dark
                auto& local_players = registry_->get_components<LocalPlayer>();
                auto& positions = registry_->get_components<Position>();
                auto& sprites = registry_->get_components<::Sprite>();

                for (size_t i = 0; i < local_players.size(); ++i) {
                    Entity player_entity = local_players.get_entity_at(i);
                    if (positions.has_entity(player_entity) && sprites.has_entity(player_entity)) {
                        const auto& pos = positions[player_entity];
                        const auto& spr = sprites[player_entity];

                        // Convert ECS Sprite to engine::Sprite
                        engine::Sprite player_sprite;
                        player_sprite.texture_handle = spr.texture;
                        player_sprite.size = {spr.width, spr.height};
                        player_sprite.origin = {spr.origin_x, spr.origin_y};
                        player_sprite.rotation = spr.rotation;
                        player_sprite.tint = {255, 255, 255, 255};
                        player_sprite.source_rect = {spr.source_rect.x, spr.source_rect.y,
                                                     spr.source_rect.width, spr.source_rect.height};

                        graphics_plugin_->draw_sprite(player_sprite, {pos.x, pos.y});
                    }
                }
            }

            // State machine for fade transitions
            if (fade_state_ == 1) {
                // Fading IN (to black)
                fade_alpha_ += dt * 1.0f;  // Fade to black over 1 second
                if (fade_alpha_ >= 1.0f) {
                    fade_alpha_ = 1.0f;
                    fade_state_ = 2;  // Now waiting for level ready

                    // Screen is now fully black - apply pending changes
                    bool was_level_transition = pending_level_transition_apply_;
                    apply_pending_level_transition();
                    apply_pending_respawn();

                    // For respawn only (no level transition), immediately start fading out
                    if (!was_level_transition) {
                        fade_state_ = 3;
                    }
                }
            } else if (fade_state_ == 2) {
                // Waiting for level to be ready (only for level transitions)
                if (level_ready_received_) {
                    fade_state_ = 3;  // Start fading out
                }
            } else if (fade_state_ == 3) {
                // Fading OUT (to visible)
                fade_alpha_ -= dt * 0.5f;  // Fade out over 2 seconds
                if (fade_alpha_ <= 0.0f) {
                    fade_alpha_ = 0.0f;
                    fade_state_ = 0;  // Done
                }
            }
        }

        if (console_overlay_)
            console_overlay_->draw(graphics_plugin_);
        // Only draw ClientGame's chat overlay when NOT in menu
        // (RoomLobbyScreen has its own chat overlay that it draws)
        if (chat_overlay_ && !in_menu) {
            chat_overlay_->draw(graphics_plugin_);
            chat_overlay_->draw_notification_badge(graphics_plugin_);
        }
        graphics_plugin_->display();
        input_plugin_->update();  // Update at END of frame for proper just_pressed detection
    }
}

void ClientGame::shutdown() {
    // Prevent double shutdown
    if (is_shutting_down_.exchange(true)) return;

    running_ = false;

    // 1. Stop network communication first
    // This stops the background thread and ensures no more callbacks are called
    if (network_client_) {
        network_client_->disconnect();
    }

    // 2. Destroy UI and high-level managers that depend on Registry/Plugins
    menu_manager_.reset();
    input_handler_.reset();
    status_overlay_.reset();
    console_overlay_.reset();
    debug_network_overlay_.reset();
    
    // 3. Clear ECS entities while managers/systems are still partially alive
    if (entity_manager_) {
        entity_manager_->clear_all();
        entity_manager_.reset();
    }

    // 4. Destroy remaining gameplay systems
    prediction_system_.reset();
    interpolation_system_.reset();
    parallax_system_.reset();
    chunk_manager_.reset();
    screen_manager_.reset();
    texture_manager_.reset();

    // 5. Destroy the Registry (this destroys all ECS Systems like AudioSystem, RenderSystem)
    // This MUST happen before plugin shutdown because systems hold plugin references
    registry_.reset();

    // 6. Finally safely shutdown and release the plugins
    if (network_client_) network_client_.reset();
    
    // Shutdown plugins in reverse order of initialization
    if (network_plugin_) network_plugin_->shutdown();
    if (audio_plugin_) audio_plugin_->shutdown();
    if (input_plugin_) input_plugin_->shutdown();
    if (graphics_plugin_) graphics_plugin_->shutdown();

    std::cout << "[ClientGame] Client terminated safely.\n";
}

void ClientGame::apply_input_to_local_player(uint16_t input_flags) {
    uint32_t local_id = entity_manager_->get_local_player_entity_id();
    if (!entity_manager_->has_entity(local_id))
        return;

    Entity player = entity_manager_->get_entity(local_id);
    if (!registry_)
        return;

    float dir_x = 0.0f;
    float dir_y = 0.0f;

    if (input_flags & protocol::INPUT_LEFT)  dir_x -= 1.0f;
    if (input_flags & protocol::INPUT_RIGHT) dir_x += 1.0f;
    if (input_flags & protocol::INPUT_UP)    dir_y -= 1.0f;
    if (input_flags & protocol::INPUT_DOWN)  dir_y += 1.0f;

    registry_->get_event_bus().publish(ecs::PlayerMoveEvent{player, dir_x, dir_y});
}

void ClientGame::handle_console_command(const std::string& command) {
    if (command == "clear") {
        console_overlay_->clear();
        console_overlay_->add_success("Console cleared");
        return;
    }

    if (command == "auth" || command.substr(0, 5) == "auth ") {
        std::string password;
        if (command.length() > 5)
            password = command.substr(5);
        else if (!admin_password_.empty())
            password = admin_password_;
        else {
            console_overlay_->add_error("Usage: auth <password>");
            return;
        }
        network_client_->send_admin_auth(password);
        console_overlay_->add_info("Authenticating...");
        return;
    }
    if (!admin_authenticated_) {
        console_overlay_->add_error("Not authenticated. Use: auth <password>");
        return;
    }
    network_client_->send_admin_command(command);
}

}
