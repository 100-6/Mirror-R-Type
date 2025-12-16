/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Multiplayer Network Client entry point
*/

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <array>
#include <chrono>
#include <sstream>
#include <string>
#include <algorithm>
#include <limits>

#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "ecs/CoreComponents.hpp"
#include "ecs/systems/SpriteAnimationSystem.hpp"
#include "systems/ScrollingSystem.hpp"
#include "ecs/systems/RenderSystem.hpp"
#include "systems/HUDSystem.hpp"
#include "plugin_manager/PluginManager.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include "plugin_manager/IInputPlugin.hpp"
#include "plugin_manager/IAudioPlugin.hpp"
#include "plugin_manager/INetworkPlugin.hpp"
#include "NetworkClient.hpp"
#include "protocol/NetworkConfig.hpp"
#include "protocol/Payloads.hpp"
#include "GameDimensions.hpp"

#ifdef _WIN32
    #include <winsock2.h>
    #define NETWORK_PLUGIN_PATH "plugins/" "asio_network.dll"
#else
    #include <arpa/inet.h>
    #define NETWORK_PLUGIN_PATH "plugins/asio_network.so"
#endif

using namespace rtype;

namespace {
constexpr float PROJECTILE_DESPAWN_MARGIN = 250.0f;
constexpr uint8_t ENTITY_STALE_THRESHOLD = 6; // ~0.1s before removing missing entities
}

struct TexturePack {
    engine::TextureHandle background = engine::INVALID_HANDLE;
    std::array<engine::TextureHandle, 4> playerFrames{};
    engine::TextureHandle enemy = engine::INVALID_HANDLE;
    engine::TextureHandle projectile = engine::INVALID_HANDLE;
    engine::TextureHandle wall = engine::INVALID_HANDLE;
};

struct DisplayMetrics {
    float playerWidth = 0.0f;
    float playerHeight = 0.0f;
    float enemyWidth = 0.0f;
    float enemyHeight = 0.0f;
    float projectileWidth = 0.0f;
    float projectileHeight = 0.0f;
    float wallWidth = 0.0f;
    float wallHeight = 0.0f;
};

struct RemoteWorldState {
    std::unordered_map<uint32_t, Entity> serverToLocal;
    std::unordered_map<uint32_t, protocol::EntityType> serverTypes;
    std::unordered_map<uint32_t, uint8_t> staleCounters;
    std::unordered_set<uint32_t> locallyIntegrated;
    std::unordered_set<uint32_t> snapshotUpdated;
};

struct StatusOverlay {
    std::string connection = "Disconnected";
    std::string lobby = "Lobby: -";
    std::string session = "Waiting";
    int pingMs = -1;
};

struct EntityDimensions {
    float width;
    float height;
};

static EntityDimensions get_collider_dimensions(protocol::EntityType type)
{
    using namespace rtype::shared::dimensions;

    if (type == protocol::EntityType::BONUS_HEALTH)
        type = protocol::EntityType::POWERUP_HEALTH;
    else if (type == protocol::EntityType::BONUS_SHIELD)
        type = protocol::EntityType::POWERUP_SHIELD;
    else if (type == protocol::EntityType::BONUS_SPEED)
        type = protocol::EntityType::POWERUP_SPEED;

    switch (type) {
        case protocol::EntityType::PLAYER:
            return {PLAYER_WIDTH, PLAYER_HEIGHT};
        case protocol::EntityType::ENEMY_BASIC:
            return {ENEMY_BASIC_WIDTH, ENEMY_BASIC_HEIGHT};
        case protocol::EntityType::ENEMY_FAST:
            return {ENEMY_FAST_WIDTH, ENEMY_FAST_HEIGHT};
        case protocol::EntityType::ENEMY_TANK:
            return {ENEMY_TANK_WIDTH, ENEMY_TANK_HEIGHT};
        case protocol::EntityType::ENEMY_BOSS:
            return {ENEMY_BOSS_WIDTH, ENEMY_BOSS_HEIGHT};
        case protocol::EntityType::PROJECTILE_PLAYER:
        case protocol::EntityType::PROJECTILE_ENEMY:
            return {PROJECTILE_WIDTH, PROJECTILE_HEIGHT};
        case protocol::EntityType::WALL:
            return {WALL_WIDTH, WALL_HEIGHT};
        case protocol::EntityType::POWERUP_HEALTH:
        case protocol::EntityType::POWERUP_SHIELD:
        case protocol::EntityType::POWERUP_SPEED:
        case protocol::EntityType::POWERUP_SCORE:
            return {BONUS_SIZE * 2.0f, BONUS_SIZE * 2.0f};
        default:
            return {rtype::shared::dimensions::ENEMY_BASIC_WIDTH,
                    rtype::shared::dimensions::ENEMY_BASIC_HEIGHT};
    }
}

static EntityDimensions get_visual_dimensions(protocol::EntityType type,
                                              const DisplayMetrics& metrics)
{
    switch (type) {
        case protocol::EntityType::PLAYER:
            return {metrics.playerWidth, metrics.playerHeight};
        case protocol::EntityType::ENEMY_FAST:
            return {metrics.enemyWidth * 0.8f, metrics.enemyHeight * 0.8f};
        case protocol::EntityType::ENEMY_TANK:
            return {metrics.enemyWidth * 1.3f, metrics.enemyHeight * 1.3f};
        case protocol::EntityType::ENEMY_BOSS:
            return {metrics.enemyWidth * 1.8f, metrics.enemyHeight * 1.8f};
        case protocol::EntityType::WALL:
            return {metrics.wallWidth, metrics.wallHeight};
        case protocol::EntityType::PROJECTILE_PLAYER:
        case protocol::EntityType::PROJECTILE_ENEMY:
            return {metrics.projectileWidth, metrics.projectileHeight};
        case protocol::EntityType::POWERUP_HEALTH:
        case protocol::EntityType::POWERUP_SHIELD:
        case protocol::EntityType::POWERUP_SPEED:
        case protocol::EntityType::POWERUP_SCORE:
            return {metrics.projectileWidth * 1.4f, metrics.projectileHeight * 1.4f};
        default:
            return {metrics.enemyWidth, metrics.enemyHeight};
    }
}

static Sprite build_sprite(protocol::EntityType type,
                           const TexturePack& textures,
                           const DisplayMetrics& metrics,
                           bool isLocalPlayer = false,
                           uint8_t subtype = 0)
{
    Sprite sprite{};
    sprite.texture = textures.enemy;
    auto visual = get_visual_dimensions(type, metrics);
    sprite.width = visual.width;
    sprite.height = visual.height;
    sprite.tint = engine::Color::White;
    sprite.layer = 5;

    switch (type) {
        case protocol::EntityType::PLAYER:
            sprite.texture = textures.playerFrames[0];
            sprite.width = visual.width;
            sprite.height = visual.height;
            sprite.layer = 10;
            sprite.tint = isLocalPlayer ? engine::Color::Cyan : engine::Color::White;
            break;
        case protocol::EntityType::ENEMY_FAST:
            sprite.texture = textures.enemy;
            sprite.width = visual.width;
            sprite.height = visual.height;
            sprite.tint = engine::Color{255, 180, 0, 255};
            break;
        case protocol::EntityType::ENEMY_TANK:
            sprite.texture = textures.enemy;
            sprite.width = visual.width;
            sprite.height = visual.height;
            sprite.tint = engine::Color{200, 80, 80, 255};
            break;
        case protocol::EntityType::ENEMY_BOSS:
            sprite.texture = textures.enemy;
            sprite.width = visual.width;
            sprite.height = visual.height;
            sprite.layer = 6;
            sprite.tint = engine::Color{180, 0, 255, 255};
            break;
        case protocol::EntityType::WALL:
            sprite.texture = textures.wall;
            sprite.width = visual.width;
            sprite.height = visual.height;
            sprite.layer = 2;
            sprite.tint = engine::Color{180, 180, 255, 255};
            break;
        case protocol::EntityType::PROJECTILE_PLAYER:
        case protocol::EntityType::PROJECTILE_ENEMY:
            sprite.texture = textures.projectile;
            sprite.width = visual.width;
            sprite.height = visual.height;
            sprite.layer = 20;
            sprite.tint = type == protocol::EntityType::PROJECTILE_PLAYER
                ? engine::Color::Cyan
                : engine::Color::Red;
            break;
        case protocol::EntityType::POWERUP_HEALTH:
        case protocol::EntityType::POWERUP_SHIELD:
        case protocol::EntityType::POWERUP_SPEED:
        case protocol::EntityType::POWERUP_SCORE:
            sprite.texture = textures.projectile;
            sprite.width = visual.width;
            sprite.height = visual.height;
            sprite.layer = 4;
            sprite.tint = engine::Color{120, 255, 120, 255};
            break;
        default:
            break;
    }

    if (type == protocol::EntityType::ENEMY_BASIC && subtype != 0) {
        sprite.tint = engine::Color{200, 200, 255, 255};
    }

    return sprite;
}

static SpriteAnimation make_player_animation(const TexturePack& textures)
{
    SpriteAnimation anim{};
    anim.frames.insert(anim.frames.end(), textures.playerFrames.begin(), textures.playerFrames.end());
    anim.frameTime = 0.1f;
    anim.loop = true;
    anim.playing = true;
    return anim;
}

static uint16_t gather_input(engine::IInputPlugin& inputPlugin)
{
    uint16_t flags = 0;
    auto pressed = [&](engine::Key key) {
        return inputPlugin.is_key_pressed(key);
    };

    if (pressed(engine::Key::W) || pressed(engine::Key::Up))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_UP);
    if (pressed(engine::Key::S) || pressed(engine::Key::Down))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_DOWN);
    if (pressed(engine::Key::A) || pressed(engine::Key::Left))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_LEFT);
    if (pressed(engine::Key::D) || pressed(engine::Key::Right))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_RIGHT);
    if (pressed(engine::Key::Space))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_SHOOT);
    if (pressed(engine::Key::LShift) || pressed(engine::Key::RShift))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_CHARGE);
    if (pressed(engine::Key::LControl) || pressed(engine::Key::RControl))
        flags |= static_cast<uint16_t>(protocol::InputFlags::INPUT_SPECIAL);

    return flags;
}

static void update_overlay_text(Registry& registry, Entity uiEntity, const StatusOverlay& status)
{
    auto& texts = registry.get_components<UIText>();
    if (!texts.has_entity(uiEntity))
        return;

    std::ostringstream oss;
    oss << status.connection;
    if (!status.lobby.empty())
        oss << " | " << status.lobby;
    if (!status.session.empty())
        oss << " | " << status.session;
    if (status.pingMs >= 0)
        oss << " | Ping: " << status.pingMs << "ms";

    UIText& text = texts[uiEntity];
    text.text = oss.str();
}

int main(int argc, char* argv[])
{
    const int SCREEN_WIDTH = 1920;
    const int SCREEN_HEIGHT = 1080;

    std::string host = "127.0.0.1";
    uint16_t tcpPort = protocol::config::DEFAULT_TCP_PORT;
    std::string playerName = "Pilot";

    if (argc > 1)
        host = argv[1];
    if (argc > 2) {
        try {
            tcpPort = static_cast<uint16_t>(std::stoi(argv[2]));
        } catch (const std::exception& e) {
            std::cerr << "Invalid TCP port: " << e.what() << '\n';
            return 1;
        }
    }
    if (argc > 3)
        playerName = argv[3];

    std::cout << "=== R-Type Network Client ===\n";
    std::cout << "Server: " << host << ":" << tcpPort << '\n';

    engine::PluginManager pluginManager;
    engine::IGraphicsPlugin* graphicsPlugin = nullptr;
    engine::IInputPlugin* inputPlugin = nullptr;
    engine::IAudioPlugin* audioPlugin = nullptr;
    engine::INetworkPlugin* networkPlugin = nullptr;

    try {
        graphicsPlugin = pluginManager.load_plugin<engine::IGraphicsPlugin>(
            "plugins/raylib_graphics.so", "create_graphics_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "Failed to load graphics plugin: " << e.what() << '\n';
        return 1;
    }

    if (!graphicsPlugin || !graphicsPlugin->create_window(SCREEN_WIDTH, SCREEN_HEIGHT, "R-Type - Multiplayer")) {
        std::cerr << "Unable to initialize graphics plugin" << '\n';
        return 1;
    }
    graphicsPlugin->set_vsync(true);

    try {
        inputPlugin = pluginManager.load_plugin<engine::IInputPlugin>(
            "plugins/raylib_input.so", "create_input_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "Failed to load input plugin: " << e.what() << '\n';
        graphicsPlugin->shutdown();
        return 1;
    }

    try {
        audioPlugin = pluginManager.load_plugin<engine::IAudioPlugin>(
            "plugins/miniaudio_audio.so", "create_audio_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "Audio plugin unavailable: " << e.what() << '\n';
    }

    try {
        networkPlugin = pluginManager.load_plugin<engine::INetworkPlugin>(
            NETWORK_PLUGIN_PATH, "create_network_plugin");
    } catch (const engine::PluginException& e) {
        std::cerr << "Failed to load network plugin: " << e.what() << '\n';
        graphicsPlugin->shutdown();
        if (audioPlugin)
            audioPlugin->shutdown();
        return 1;
    }

    if (!networkPlugin->initialize()) {
        std::cerr << "Network plugin initialization failed" << '\n';
        graphicsPlugin->shutdown();
        if (audioPlugin)
            audioPlugin->shutdown();
        return 1;
    }

    Registry registry;
    registry.register_component<Position>();
    registry.register_component<Velocity>();
    registry.register_component<Sprite>();
    registry.register_component<SpriteAnimation>();
    registry.register_component<Collider>();
    registry.register_component<Health>();
    registry.register_component<Score>();
    registry.register_component<Controllable>();
    registry.register_component<Background>();
    registry.register_component<Scrollable>();
    registry.register_component<NetworkId>();
    registry.register_component<UIText>();
    registry.register_component<GameState>();
    registry.register_component<WaveController>();
    registry.register_component<Shield>();
    registry.register_component<SpeedBoost>();
    registry.register_component<CircleEffect>();
    registry.register_component<TextEffect>();
    registry.register_component<Bonus>();
    registry.register_component<Invulnerability>();

    registry.register_system<ScrollingSystem>(-100.0f, static_cast<float>(SCREEN_WIDTH));
    registry.register_system<SpriteAnimationSystem>();
    registry.register_system<RenderSystem>(*graphicsPlugin);
    registry.register_system<HUDSystem>(*graphicsPlugin, SCREEN_WIDTH, SCREEN_HEIGHT);

    TexturePack textures;
    textures.background = graphicsPlugin->load_texture("assets/sprite/symmetry.png");
    textures.playerFrames[0] = graphicsPlugin->load_texture("assets/sprite/ship1.png");
    textures.playerFrames[1] = graphicsPlugin->load_texture("assets/sprite/ship2.png");
    textures.playerFrames[2] = graphicsPlugin->load_texture("assets/sprite/ship3.png");
    textures.playerFrames[3] = graphicsPlugin->load_texture("assets/sprite/ship4.png");
    textures.enemy = graphicsPlugin->load_texture("assets/sprite/enemy.png");
    textures.projectile = graphicsPlugin->load_texture("assets/sprite/bullet.png");
    textures.wall = graphicsPlugin->load_texture("assets/sprite/lock.png");

    if (textures.background == engine::INVALID_HANDLE ||
        textures.playerFrames[0] == engine::INVALID_HANDLE ||
        textures.enemy == engine::INVALID_HANDLE ||
        textures.projectile == engine::INVALID_HANDLE) {
        std::cerr << "Failed to load textures" << '\n';
        graphicsPlugin->shutdown();
        if (audioPlugin)
            audioPlugin->shutdown();
        return 1;
    }

    DisplayMetrics metrics;
    engine::Vector2f playerSize = graphicsPlugin->get_texture_size(textures.playerFrames[0]);
    engine::Vector2f enemySize = graphicsPlugin->get_texture_size(textures.enemy);
    engine::Vector2f projectileSize = graphicsPlugin->get_texture_size(textures.projectile);
    engine::Vector2f wallSize = graphicsPlugin->get_texture_size(textures.wall);

    metrics.playerWidth = playerSize.x * 0.20f;
    metrics.playerHeight = playerSize.y * 0.20f;
    metrics.enemyWidth = enemySize.x;
    metrics.enemyHeight = enemySize.y;
    metrics.projectileWidth = projectileSize.x * 0.80f;
    metrics.projectileHeight = projectileSize.y * 0.80f;
    metrics.wallWidth = 50.0f;
    metrics.wallHeight = 100.0f;

    Entity background1 = registry.spawn_entity();
    registry.add_component(background1, Position{0.0f, 0.0f});
    registry.add_component(background1, Background{});
    registry.add_component(background1, Scrollable{1.0f, true, false});
    registry.add_component(background1, Sprite{textures.background,
        static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT),
        0.0f, engine::Color::White, 0.0f, 0.0f, -100});

    Entity background2 = registry.spawn_entity();
    registry.add_component(background2, Position{static_cast<float>(SCREEN_WIDTH), 0.0f});
    registry.add_component(background2, Background{});
    registry.add_component(background2, Scrollable{1.0f, true, false});
    registry.add_component(background2, Sprite{textures.background,
        static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT),
        0.0f, engine::Color::White, 0.0f, 0.0f, -100});

    Entity waveTracker = registry.spawn_entity();
    WaveController waveCtrl;
    waveCtrl.totalWaveCount = 0;
    waveCtrl.currentWaveNumber = 0;
    waveCtrl.currentWaveIndex = 0;
    waveCtrl.totalScrollDistance = 0.0f;
    waveCtrl.allWavesCompleted = false;
    registry.add_component(waveTracker, waveCtrl);

    Entity statusEntity = registry.spawn_entity();
    registry.add_component(statusEntity, Position{30.0f, 30.0f});
    registry.add_component(statusEntity, UIText{"Connecting...", engine::Color::White,
        engine::Color{0, 0, 0, 180}, 22, true, 2.0f, 2.0f, true, 102});

    RemoteWorldState remoteWorld;
    StatusOverlay overlay;
    uint32_t localPlayerEntityId = std::numeric_limits<uint32_t>::max();
    std::unordered_map<uint32_t, std::string> playerNames;
    std::unordered_map<uint32_t, Entity> playerNameTags;

    auto refreshOverlay = [&registry, statusEntity](const StatusOverlay& info) {
        update_overlay_text(registry, statusEntity, info);
    };

    auto get_player_label = [&](uint32_t serverId) {
        auto it = playerNames.find(serverId);
        if (it != playerNames.end() && !it->second.empty())
            return it->second;
        return std::string("Player ") + std::to_string(serverId);
    };

    auto ensure_player_name_tag = [&](uint32_t serverId, float x, float y) {
        auto& texts = registry.get_components<UIText>();
        auto& positions = registry.get_components<Position>();
        auto it = playerNameTags.find(serverId);
        Entity textEntity;
        if (it == playerNameTags.end()) {
            textEntity = registry.spawn_entity();
            playerNameTags[serverId] = textEntity;
            registry.add_component(textEntity, Position{x, y - 30.0f});
            registry.add_component(textEntity, UIText{
                get_player_label(serverId),
                engine::Color::White,
                engine::Color{0, 0, 0, 180},
                20,
                true,
                1.5f,
                1.5f,
                true,
                105
            });
        } else {
            textEntity = it->second;
            if (texts.has_entity(textEntity)) {
                texts[textEntity].text = get_player_label(serverId);
            }
        }
    };

    auto remove_remote_entity = [&](uint32_t serverId) {
        auto it = remoteWorld.serverToLocal.find(serverId);
        if (it == remoteWorld.serverToLocal.end())
            return;
        registry.kill_entity(it->second);
        remoteWorld.serverTypes.erase(serverId);
        remoteWorld.staleCounters.erase(serverId);
        remoteWorld.locallyIntegrated.erase(serverId);
        remoteWorld.snapshotUpdated.erase(serverId);
        remoteWorld.serverToLocal.erase(it);
        if (localPlayerEntityId == serverId)
            localPlayerEntityId = std::numeric_limits<uint32_t>::max();
        auto tagIt = playerNameTags.find(serverId);
        if (tagIt != playerNameTags.end()) {
            registry.kill_entity(tagIt->second);
            playerNameTags.erase(tagIt);
        }
    };

    auto clear_remote_world = [&]() {
        for (const auto& pair : remoteWorld.serverToLocal)
            registry.kill_entity(pair.second);
        remoteWorld.serverToLocal.clear();
        remoteWorld.serverTypes.clear();
        remoteWorld.staleCounters.clear();
        remoteWorld.locallyIntegrated.clear();
        remoteWorld.snapshotUpdated.clear();
        localPlayerEntityId = std::numeric_limits<uint32_t>::max();
        for (const auto& tagPair : playerNameTags)
            registry.kill_entity(tagPair.second);
        playerNameTags.clear();
    };

    uint32_t localPlayerId = 0;
    SpriteAnimation playerAnimation = make_player_animation(textures);

    auto spawn_or_update_entity = [&](uint32_t serverId,
                                      protocol::EntityType type,
                                      float x, float y,
                                      uint16_t health,
                                      uint8_t subtype) {
        Entity entity;
        const bool isNew = remoteWorld.serverToLocal.find(serverId) == remoteWorld.serverToLocal.end();
        if (isNew) {
            entity = registry.spawn_entity();
            remoteWorld.serverToLocal[serverId] = entity;
            registry.add_component(entity, NetworkId{serverId});
        } else {
            entity = remoteWorld.serverToLocal[serverId];
        }

        remoteWorld.serverTypes[serverId] = type;
        remoteWorld.staleCounters[serverId] = 0;
        if (type == protocol::EntityType::PROJECTILE_PLAYER || type == protocol::EntityType::PROJECTILE_ENEMY) {
            remoteWorld.locallyIntegrated.insert(serverId);
        } else {
            remoteWorld.locallyIntegrated.erase(serverId);
        }

        bool localSubtypeMatch = false;
        if (type == protocol::EntityType::PLAYER && localPlayerId != 0) {
            uint8_t localIdByte = static_cast<uint8_t>(localPlayerId & 0xFFu);
            if (subtype == localIdByte) {
                localPlayerEntityId = serverId;
                localSubtypeMatch = true;
            }
        }
        bool highlightAsLocal = (serverId == localPlayerEntityId) || localSubtypeMatch;

        auto& positions = registry.get_components<Position>();
        if (positions.has_entity(entity)) {
            positions[entity].x = x;
            positions[entity].y = y;
        } else {
            registry.add_component(entity, Position{x, y});
        }

        Sprite sprite = build_sprite(type, textures, metrics, highlightAsLocal, subtype);
        auto& sprites = registry.get_components<Sprite>();
        if (sprites.has_entity(entity)) {
            Sprite& existing = sprites[entity];
            existing = sprite;
        } else {
            registry.add_component(entity, sprite);
        }

        auto& colliders = registry.get_components<Collider>();
        EntityDimensions colliderDims = get_collider_dimensions(type);
        if (colliders.has_entity(entity)) {
            colliders[entity].width = colliderDims.width;
            colliders[entity].height = colliderDims.height;
        } else {
            registry.add_component(entity, Collider{colliderDims.width, colliderDims.height});
        }

        auto& healths = registry.get_components<Health>();
        auto& scores = registry.get_components<Score>();
        auto& controllables = registry.get_components<Controllable>();
        int hp = static_cast<int>(health);
        if (healths.has_entity(entity)) {
            healths[entity].current = hp;
            healths[entity].max = std::max(healths[entity].max, hp);
        } else {
            Health comp;
            comp.current = hp;
            comp.max = hp;
            registry.add_component(entity, comp);
        }

        if (!scores.has_entity(entity)) {
            registry.add_component(entity, Score{0});
        }

        if (highlightAsLocal) {
            if (!controllables.has_entity(entity)) {
                registry.add_component(entity, Controllable{300.0f});
            }
        } else if (controllables.has_entity(entity) && serverId != localPlayerEntityId) {
            registry.remove_component<Controllable>(entity);
        }

        if (type == protocol::EntityType::PLAYER) {
            auto& animations = registry.get_components<SpriteAnimation>();
            if (!animations.has_entity(entity)) {
                registry.add_component(entity, playerAnimation);
            }
            ensure_player_name_tag(serverId, x, y);
        }

        return entity;
    };

    rtype::client::NetworkClient client(*networkPlugin);

    overlay.connection = "Connecting to " + host + ":" + std::to_string(tcpPort);
    refreshOverlay(overlay);

    client.set_on_accepted([&](uint32_t playerId) {
        localPlayerId = playerId;
        overlay.connection = "Connected (Player " + std::to_string(playerId) + ")";
        refreshOverlay(overlay);
        client.send_join_lobby(protocol::GameMode::DUO, protocol::Difficulty::NORMAL);
    });

    client.set_on_rejected([&](uint8_t reason, const std::string& message) {
        overlay.connection = "Rejected: " + message;
        refreshOverlay(overlay);
    });

    client.set_on_lobby_state([&](const protocol::ServerLobbyStatePayload& state,
                                  const std::vector<protocol::PlayerLobbyEntry>& playersInfo) {
        int current = static_cast<int>(state.current_player_count);
        int required = static_cast<int>(state.required_player_count);
        overlay.lobby = "Lobby " + std::to_string(state.lobby_id) + ": " +
                        std::to_string(current) + "/" + std::to_string(required);
        refreshOverlay(overlay);

        auto& texts = registry.get_components<UIText>();
        for (const auto& entry : playersInfo) {
            std::string name(entry.player_name);
            if (name.empty())
                name = "Player " + std::to_string(entry.player_id);
            playerNames[entry.player_id] = name;
            auto tagIt = playerNameTags.find(entry.player_id);
            if (tagIt != playerNameTags.end() && texts.has_entity(tagIt->second)) {
                texts[tagIt->second].text = name;
            }
        }
    });

    client.set_on_countdown([&](uint8_t seconds) {
        overlay.session = "Starting in " + std::to_string(seconds) + "s";
        refreshOverlay(overlay);
    });

    client.set_on_game_start([&](uint32_t sessionId, uint16_t udpPort) {
        (void)udpPort;
        overlay.session = "In game (session " + std::to_string(sessionId) + ")";
        refreshOverlay(overlay);
        clear_remote_world();
        auto& waveControllers = registry.get_components<WaveController>();
        if (waveControllers.has_entity(waveTracker)) {
            WaveController& ctrl = waveControllers[waveTracker];
            ctrl.currentWaveNumber = 0;
            ctrl.currentWaveIndex = 0;
            ctrl.allWavesCompleted = false;
            ctrl.totalScrollDistance = 0.0f;
        }
    });

    client.set_on_entity_spawn([&](const protocol::ServerEntitySpawnPayload& spawn) {
        uint32_t serverId = ntohl(spawn.entity_id);
        uint16_t health = ntohs(spawn.health);
        spawn_or_update_entity(serverId, spawn.entity_type, spawn.spawn_x, spawn.spawn_y, health, spawn.subtype);
    });

    client.set_on_entity_destroy([&](const protocol::ServerEntityDestroyPayload& destroy) {
        uint32_t serverId = ntohl(destroy.entity_id);
        remove_remote_entity(serverId);
    });

    client.set_on_wave_start([&](const protocol::ServerWaveStartPayload& wave) {
        auto& waveControllers = registry.get_components<WaveController>();
        if (!waveControllers.has_entity(waveTracker))
            return;
        WaveController& ctrl = waveControllers[waveTracker];
        ctrl.currentWaveNumber = static_cast<int>(ntohl(wave.wave_number));
        ctrl.currentWaveIndex = ctrl.currentWaveNumber;
        ctrl.totalWaveCount = ntohs(wave.total_waves);
        ctrl.totalScrollDistance = wave.scroll_distance;
        ctrl.allWavesCompleted = false;
    });

    client.set_on_wave_complete([&](const protocol::ServerWaveCompletePayload& wave) {
        auto& waveControllers = registry.get_components<WaveController>();
        if (!waveControllers.has_entity(waveTracker))
            return;
        WaveController& ctrl = waveControllers[waveTracker];
        ctrl.currentWaveNumber = static_cast<int>(ntohl(wave.wave_number));
        if (wave.all_waves_complete)
            ctrl.currentWaveNumber = static_cast<int>(ctrl.totalWaveCount);
        ctrl.allWavesCompleted = wave.all_waves_complete != 0;
    });

    client.set_on_projectile_spawn([&](const protocol::ServerProjectileSpawnPayload& proj) {
        uint32_t projId = ntohl(proj.projectile_id);
        uint32_t ownerId = ntohl(proj.owner_id);
        protocol::EntityType projType = protocol::EntityType::PROJECTILE_PLAYER;
        auto ownerIt = remoteWorld.serverTypes.find(ownerId);
        if (ownerIt != remoteWorld.serverTypes.end() && ownerIt->second != protocol::EntityType::PLAYER)
            projType = protocol::EntityType::PROJECTILE_ENEMY;

        Entity entity = spawn_or_update_entity(projId, projType, proj.spawn_x, proj.spawn_y, 1, 0);
        auto& velocities = registry.get_components<Velocity>();
        float velX = static_cast<float>(proj.velocity_x);
        float velY = static_cast<float>(proj.velocity_y);
        if (velocities.has_entity(entity)) {
            velocities[entity].x = velX;
            velocities[entity].y = velY;
        } else {
            registry.add_component(entity, Velocity{velX, velY});
        }
    });

    client.set_on_snapshot([&](const protocol::ServerSnapshotPayload& header,
                               const std::vector<protocol::EntityState>& entities) {
        (void)header;
        auto& positions = registry.get_components<Position>();
        auto& velocities = registry.get_components<Velocity>();
        auto& healths = registry.get_components<Health>();
        std::unordered_set<uint32_t> updatedIds;
        updatedIds.reserve(entities.size());

        for (const auto& state : entities) {
            uint32_t serverId = ntohl(state.entity_id);
            updatedIds.insert(serverId);

            auto it = remoteWorld.serverToLocal.find(serverId);
            if (it == remoteWorld.serverToLocal.end())
                continue;

            Entity entity = it->second;
            remoteWorld.staleCounters[serverId] = 0;
            remoteWorld.snapshotUpdated.insert(serverId);
            if (positions.has_entity(entity)) {
                positions[entity].x = state.position_x;
                positions[entity].y = state.position_y;
            } else {
                registry.add_component(entity, Position{state.position_x, state.position_y});
            }

            float velX = static_cast<float>(state.velocity_x) / 10.0f;
            float velY = static_cast<float>(state.velocity_y) / 10.0f;
            if (velocities.has_entity(entity)) {
                velocities[entity].x = velX;
                velocities[entity].y = velY;
            } else {
                registry.add_component(entity, Velocity{velX, velY});
            }

            uint16_t hp = ntohs(state.health);
            if (healths.has_entity(entity)) {
                healths[entity].current = static_cast<int>(hp);
                healths[entity].max = std::max(healths[entity].max, static_cast<int>(hp));
            } else {
                Health comp;
                comp.current = static_cast<int>(hp);
                comp.max = static_cast<int>(hp);
                registry.add_component(entity, comp);
            }

            auto typeIt = remoteWorld.serverTypes.find(serverId);
            protocol::EntityType entityType = typeIt != remoteWorld.serverTypes.end()
                ? typeIt->second
                : protocol::EntityType::PLAYER;
            if (hp == 0 && entityType != protocol::EntityType::PLAYER &&
                entityType != protocol::EntityType::WALL &&
                entityType != protocol::EntityType::POWERUP_HEALTH &&
                entityType != protocol::EntityType::POWERUP_SHIELD &&
                entityType != protocol::EntityType::POWERUP_SPEED &&
                entityType != protocol::EntityType::POWERUP_SCORE &&
                entityType != protocol::EntityType::BONUS_HEALTH &&
                entityType != protocol::EntityType::BONUS_SHIELD &&
                entityType != protocol::EntityType::BONUS_SPEED &&
                entityType != protocol::EntityType::PROJECTILE_PLAYER &&
                entityType != protocol::EntityType::PROJECTILE_ENEMY) {
                remove_remote_entity(serverId);
            }
        }

        std::vector<uint32_t> staleRemovals;
        for (const auto& pair : remoteWorld.serverToLocal) {
            uint32_t serverId = pair.first;
            if (updatedIds.count(serverId))
                continue;
            auto counterIt = remoteWorld.staleCounters.find(serverId);
            if (counterIt == remoteWorld.staleCounters.end()) {
                counterIt = remoteWorld.staleCounters.emplace(serverId, 0).first;
            }
            uint8_t& counter = counterIt->second;
            if (counter < 250)
                counter++;
            if (counter > ENTITY_STALE_THRESHOLD) {
                staleRemovals.push_back(serverId);
            }
        }

        for (uint32_t id : staleRemovals) {
            remove_remote_entity(id);
        }
    });

    client.set_on_game_over([&](const protocol::ServerGameOverPayload& result) {
        overlay.session = result.result == protocol::GameResult::VICTORY ? "Victory" : "Defeat";
        refreshOverlay(overlay);
    });

    bool running = true;
    client.set_on_disconnected([&]() {
        overlay.connection = "Disconnected";
        refreshOverlay(overlay);
        running = false;
    });

    if (!client.connect(host, tcpPort)) {
        std::cerr << "Failed to connect to server" << '\n';
        networkPlugin->shutdown();
        graphicsPlugin->shutdown();
        if (audioPlugin)
            audioPlugin->shutdown();
        return 1;
    }

    client.send_connect(playerName);

    auto lastFrame = std::chrono::steady_clock::now();
    auto lastInputSend = lastFrame;
    auto lastPing = lastFrame;
    auto lastOverlayUpdate = lastFrame;
    uint32_t clientTick = 0;

    while (running && graphicsPlugin->is_window_open()) {
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - lastFrame).count();
        lastFrame = now;

        networkPlugin->update(dt);
        client.update();

        bool escapePressed = inputPlugin->is_key_pressed(engine::Key::Escape);

        if (client.is_in_game() &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastInputSend).count() >= 30) {
            uint16_t inputFlags = gather_input(*inputPlugin);
            client.send_input(inputFlags, clientTick++);
            lastInputSend = now;
        }

        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastPing).count() >= 5) {
            client.send_ping();
            lastPing = now;
        }

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastOverlayUpdate).count() >= 500) {
            overlay.pingMs = networkPlugin->get_server_ping();
            refreshOverlay(overlay);
            lastOverlayUpdate = now;
        }

        if (!remoteWorld.locallyIntegrated.empty()) {
            std::vector<uint32_t> despawnList;
            despawnList.reserve(remoteWorld.locallyIntegrated.size());
            auto& positions = registry.get_components<Position>();
            auto& velocities = registry.get_components<Velocity>();

            for (uint32_t serverId : remoteWorld.locallyIntegrated) {
                if (remoteWorld.snapshotUpdated.count(serverId))
                    continue;
                auto it = remoteWorld.serverToLocal.find(serverId);
                if (it == remoteWorld.serverToLocal.end())
                    continue;
                Entity entity = it->second;
                if (!positions.has_entity(entity) || !velocities.has_entity(entity))
                    continue;

                Position& pos = positions[entity];
                Velocity& vel = velocities[entity];
                pos.x += vel.x * dt;
                pos.y += vel.y * dt;

                if (pos.x < -PROJECTILE_DESPAWN_MARGIN ||
                    pos.x > SCREEN_WIDTH + PROJECTILE_DESPAWN_MARGIN ||
                    pos.y < -PROJECTILE_DESPAWN_MARGIN ||
                    pos.y > SCREEN_HEIGHT + PROJECTILE_DESPAWN_MARGIN) {
                    despawnList.push_back(serverId);
                }
            }

            for (uint32_t id : despawnList) {
                remove_remote_entity(id);
            }
        }

        remoteWorld.snapshotUpdated.clear();

        auto& positions = registry.get_components<Position>();
        auto& sprites = registry.get_components<Sprite>();
        auto& texts = registry.get_components<UIText>();
        std::vector<uint32_t> orphanTags;
        for (const auto& [serverId, textEntity] : playerNameTags) {
            auto entIt = remoteWorld.serverToLocal.find(serverId);
            if (entIt == remoteWorld.serverToLocal.end()) {
                orphanTags.push_back(serverId);
                continue;
            }
            Entity playerEntity = entIt->second;
            if (!positions.has_entity(playerEntity) || !positions.has_entity(textEntity) || !texts.has_entity(textEntity))
                continue;
            const Position& playerPos = positions[playerEntity];
            float spriteHeight = 0.0f;
            if (sprites.has_entity(playerEntity))
                spriteHeight = sprites[playerEntity].height;
            Position& tagPos = positions[textEntity];
            tagPos.x = playerPos.x;
            tagPos.y = playerPos.y - (spriteHeight * 0.2f + 30.0f);
        }
        for (uint32_t serverId : orphanTags) {
            auto it = playerNameTags.find(serverId);
            if (it != playerNameTags.end()) {
                registry.kill_entity(it->second);
                playerNameTags.erase(it);
            }
        }

        registry.run_systems(dt);

        graphicsPlugin->display();
        inputPlugin->update();

        if (escapePressed)
            running = false;
    }

    clear_remote_world();
    client.disconnect();

    networkPlugin->shutdown();
    if (audioPlugin)
        audioPlugin->shutdown();
    inputPlugin->shutdown();
    graphicsPlugin->shutdown();

    std::cout << "Client terminated." << std::endl;
    return 0;
}
