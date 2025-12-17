#include "ClientGame.hpp"
#include "ecs/systems/SpriteAnimationSystem.hpp"
#include "systems/ScrollingSystem.hpp"
#include "ecs/systems/RenderSystem.hpp"
#include "systems/HUDSystem.hpp"
#include "protocol/NetworkConfig.hpp"
#include <iostream>
#include <chrono>

#ifdef _WIN32
    #include <winsock2.h>
    #define NETWORK_PLUGIN_PATH "plugins/asio_network.dll"
#else
    #include <arpa/inet.h>
    #define NETWORK_PLUGIN_PATH "plugins/asio_network.so"
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
    , client_tick_(0) {
}

ClientGame::~ClientGame() {
    shutdown();
}

bool ClientGame::initialize(const std::string& host, uint16_t tcp_port, const std::string& player_name) {
    host_ = host;
    tcp_port_ = tcp_port;
    player_name_ = player_name;

    std::cout << "=== R-Type Network Client ===\n";
    std::cout << "Server: " << host_ << ":" << tcp_port_ << '\n';

    if (!load_plugins())
        return false;

    registry_ = std::make_unique<Registry>();
    setup_registry();
    setup_systems();

    texture_manager_ = std::make_unique<TextureManager>(*graphics_plugin_);
    if (!load_textures())
        return false;

    setup_background();

    screen_manager_ = std::make_unique<ScreenManager>(
        *registry_, screen_width_, screen_height_,
        texture_manager_->get_background(),
        texture_manager_->get_menu_background()
    );
    screen_manager_->initialize();

    entity_manager_ = std::make_unique<EntityManager>(
        *registry_, *texture_manager_, screen_width_, screen_height_
    );

    status_overlay_ = std::make_unique<StatusOverlay>(
        *registry_, screen_manager_->get_status_entity()
    );

    input_handler_ = std::make_unique<InputHandler>(*input_plugin_);

    network_client_ = std::make_unique<rtype::client::NetworkClient>(*network_plugin_);
    setup_network_callbacks();

    status_overlay_->set_connection("Connecting to " + host_ + ":" + std::to_string(tcp_port_));
    status_overlay_->refresh();

    if (!network_client_->connect(host_, tcp_port_)) {
        std::cerr << "[ClientGame] Failed to connect to server\n";
        return false;
    }

    network_client_->send_connect(player_name_);
    running_ = true;

    return true;
}

bool ClientGame::load_plugins() {
    try {
        graphics_plugin_ = plugin_manager_.load_plugin<engine::IGraphicsPlugin>(
            "plugins/raylib_graphics.so", "create_graphics_plugin");
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
            "plugins/raylib_input.so", "create_input_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "[ClientGame] Failed to load input plugin: " << e.what() << '\n';
        graphics_plugin_->shutdown();
        return false;
    }

    try {
        audio_plugin_ = plugin_manager_.load_plugin<engine::IAudioPlugin>(
            "plugins/miniaudio_audio.so", "create_audio_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "[ClientGame] Audio plugin unavailable: " << e.what() << '\n';
    }

    try {
        network_plugin_ = plugin_manager_.load_plugin<engine::INetworkPlugin>(
            NETWORK_PLUGIN_PATH, "create_network_plugin");
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
    registry_->register_component<Sprite>();
    registry_->register_component<SpriteAnimation>();
    registry_->register_component<Collider>();
    registry_->register_component<Health>();
    registry_->register_component<Score>();
    registry_->register_component<Controllable>();
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
}

void ClientGame::setup_systems() {
    registry_->register_system<ScrollingSystem>(-100.0f, static_cast<float>(screen_width_));
    registry_->register_system<SpriteAnimationSystem>();
    registry_->register_system<RenderSystem>(*graphics_plugin_);
    registry_->register_system<HUDSystem>(*graphics_plugin_, screen_width_, screen_height_);
}

void ClientGame::setup_background() {
    Entity background1 = registry_->spawn_entity();
    registry_->add_component(background1, Position{0.0f, 0.0f});
    registry_->add_component(background1, Background{});
    registry_->add_component(background1, Scrollable{1.0f, true, false});
    registry_->add_component(background1, Sprite{
        texture_manager_->get_background(),
        static_cast<float>(screen_width_),
        static_cast<float>(screen_height_),
        0.0f,
        engine::Color::White,
        0.0f,
        0.0f,
        -100
    });

    Entity background2 = registry_->spawn_entity();
    registry_->add_component(background2, Position{static_cast<float>(screen_width_), 0.0f});
    registry_->add_component(background2, Background{});
    registry_->add_component(background2, Scrollable{1.0f, true, false});
    registry_->add_component(background2, Sprite{
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
    wave_ctrl.currentWaveNumber = 0;
    wave_ctrl.currentWaveIndex = 0;
    wave_ctrl.totalScrollDistance = 0.0f;
    wave_ctrl.allWavesCompleted = false;
    registry_->add_component(wave_tracker_, wave_ctrl);
}

void ClientGame::setup_network_callbacks() {
    network_client_->set_on_accepted([this](uint32_t player_id) {
        entity_manager_->set_local_player_id(player_id);
        status_overlay_->set_connection("Connected (Player " + std::to_string(player_id) + ")");
        status_overlay_->refresh();
        network_client_->send_join_lobby(protocol::GameMode::DUO, protocol::Difficulty::NORMAL);
    });

    network_client_->set_on_rejected([this](uint8_t reason, const std::string& message) {
        status_overlay_->set_connection("Rejected: " + message);
        status_overlay_->refresh();
    });

    network_client_->set_on_lobby_state([this](const protocol::ServerLobbyStatePayload& state,
                                               const std::vector<protocol::PlayerLobbyEntry>& players_info) {
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
    });

    network_client_->set_on_countdown([this](uint8_t seconds) {
        status_overlay_->set_session("Starting in " + std::to_string(seconds) + "s");
        status_overlay_->refresh();
    });

    network_client_->set_on_game_start([this](uint32_t session_id, uint16_t udp_port) {
        (void)udp_port;
        status_overlay_->set_session("In game (session " + std::to_string(session_id) + ")");
        status_overlay_->refresh();
        entity_manager_->clear_all();
        screen_manager_->set_screen(GameScreen::PLAYING);

        auto& wave_controllers = registry_->get_components<WaveController>();
        if (wave_controllers.has_entity(wave_tracker_)) {
            WaveController& ctrl = wave_controllers[wave_tracker_];
            ctrl.currentWaveNumber = 0;
            ctrl.currentWaveIndex = 0;
            ctrl.allWavesCompleted = false;
            ctrl.totalScrollDistance = 0.0f;
        }
    });

    network_client_->set_on_entity_spawn([this](const protocol::ServerEntitySpawnPayload& spawn) {
        uint32_t server_id = ntohl(spawn.entity_id);
        uint16_t health = ntohs(spawn.health);
        entity_manager_->spawn_or_update_entity(server_id, spawn.entity_type,
                                                spawn.spawn_x, spawn.spawn_y, health, spawn.subtype);
    });

    network_client_->set_on_entity_destroy([this](const protocol::ServerEntityDestroyPayload& destroy) {
        uint32_t server_id = ntohl(destroy.entity_id);
        entity_manager_->remove_entity(server_id);
    });

    network_client_->set_on_wave_start([this](const protocol::ServerWaveStartPayload& wave) {
        auto& wave_controllers = registry_->get_components<WaveController>();
        if (!wave_controllers.has_entity(wave_tracker_))
            return;
        WaveController& ctrl = wave_controllers[wave_tracker_];
        ctrl.currentWaveNumber = static_cast<int>(ntohl(wave.wave_number));
        ctrl.currentWaveIndex = ctrl.currentWaveNumber;
        ctrl.totalWaveCount = ntohs(wave.total_waves);
        ctrl.totalScrollDistance = wave.scroll_distance;
        ctrl.allWavesCompleted = false;
    });

    network_client_->set_on_wave_complete([this](const protocol::ServerWaveCompletePayload& wave) {
        auto& wave_controllers = registry_->get_components<WaveController>();
        if (!wave_controllers.has_entity(wave_tracker_))
            return;
        WaveController& ctrl = wave_controllers[wave_tracker_];
        ctrl.currentWaveNumber = static_cast<int>(ntohl(wave.wave_number));
        if (wave.all_waves_complete)
            ctrl.currentWaveNumber = static_cast<int>(ctrl.totalWaveCount);
        ctrl.allWavesCompleted = wave.all_waves_complete != 0;
    });

    network_client_->set_on_projectile_spawn([this](const protocol::ServerProjectileSpawnPayload& proj) {
        uint32_t proj_id = ntohl(proj.projectile_id);
        uint32_t owner_id = ntohl(proj.owner_id);

        protocol::EntityType proj_type = protocol::EntityType::PROJECTILE_PLAYER;
        // Determine projectile type based on owner (simplified)

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

    network_client_->set_on_snapshot([this](const protocol::ServerSnapshotPayload& header,
                                            const std::vector<protocol::EntityState>& entities) {
        (void)header;
        auto& positions = registry_->get_components<Position>();
        auto& velocities = registry_->get_components<Velocity>();
        auto& healths = registry_->get_components<Health>();
        std::unordered_set<uint32_t> updated_ids;
        updated_ids.reserve(entities.size());

        for (const auto& state : entities) {
            uint32_t server_id = ntohl(state.entity_id);
            updated_ids.insert(server_id);

            if (!entity_manager_->has_entity(server_id))
                continue;

            Entity entity = entity_manager_->get_entity(server_id);

            if (positions.has_entity(entity)) {
                positions[entity].x = state.position_x;
                positions[entity].y = state.position_y;
            } else {
                registry_->add_component(entity, Position{state.position_x, state.position_y});
            }

            float vel_x = static_cast<float>(state.velocity_x) / 10.0f;
            float vel_y = static_cast<float>(state.velocity_y) / 10.0f;
            if (velocities.has_entity(entity)) {
                velocities[entity].x = vel_x;
                velocities[entity].y = vel_y;
            } else {
                registry_->add_component(entity, Velocity{vel_x, vel_y});
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
        bool victory = result.result == protocol::GameResult::VICTORY;
        status_overlay_->set_session(victory ? "Victory" : "Defeat");
        status_overlay_->refresh();
        screen_manager_->show_result(victory);
    });

    network_client_->set_on_disconnected([this]() {
        status_overlay_->set_connection("Disconnected");
        status_overlay_->refresh();
        running_ = false;
    });
}

void ClientGame::run() {
    auto last_frame = std::chrono::steady_clock::now();
    auto last_input_send = last_frame;
    auto last_ping = last_frame;
    auto last_overlay_update = last_frame;

    while (running_ && graphics_plugin_->is_window_open()) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last_frame).count();
        last_frame = now;

        network_plugin_->update(dt);
        network_client_->update();

        if (input_handler_->is_escape_pressed()) {
            running_ = false;
            break;
        }

        if (network_client_->is_in_game() &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_input_send).count() >= 15) {
            uint16_t input_flags = input_handler_->gather_input();
            network_client_->send_input(input_flags, client_tick_++);
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

        entity_manager_->update_projectiles(dt);
        entity_manager_->update_name_tags();

        registry_->run_systems(dt);

        graphics_plugin_->display();
        input_plugin_->update();
    }
}

void ClientGame::shutdown() {
    if (network_client_) {
        entity_manager_->clear_all();
        network_client_->disconnect();
    }

    if (network_plugin_)
        network_plugin_->shutdown();
    if (audio_plugin_)
        audio_plugin_->shutdown();
    if (input_plugin_)
        input_plugin_->shutdown();
    if (graphics_plugin_)
        graphics_plugin_->shutdown();

    std::cout << "[ClientGame] Client terminated.\n";
}

}
