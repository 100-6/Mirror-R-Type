#include "BagarioServer.hpp"
#include "BagarioSession.hpp"
#include "BagarioNetworkHandler.hpp"
#include "BagarioPacketSender.hpp"
#include "plugin_manager/PluginPaths.hpp"

#include <iostream>
#include <thread>
#include <chrono>

namespace bagario::server {

BagarioServer::BagarioServer(uint16_t tcp_port, uint16_t udp_port, bool listen_on_all_interfaces)
    : m_tcp_port(tcp_port)
    , m_udp_port(udp_port)
    , m_listen_on_all_interfaces(listen_on_all_interfaces) {}

BagarioServer::~BagarioServer() {
    stop();
}

bool BagarioServer::start() {
    std::cout << "[BagarioServer] Starting server on TCP:" << m_tcp_port
              << " UDP:" << m_udp_port << std::endl;
    m_network = m_plugin_manager.load_plugin<engine::INetworkPlugin>(
        engine::PluginPaths::get_plugin_path(engine::PluginPaths::ENET_NETWORK),
        "create_network_plugin"
    );
    if (!m_network) {
        std::cerr << "[BagarioServer] Failed to load ENet network plugin" << std::endl;
        return false;
    }
    if (!m_network->initialize()) {
        std::cerr << "[BagarioServer] Failed to initialize network plugin" << std::endl;
        return false;
    }
    if (!m_network->start_server(m_tcp_port, m_udp_port, m_listen_on_all_interfaces)) {
        std::cerr << "[BagarioServer] Failed to start network server" << std::endl;
        return false;
    }
    m_network->set_on_client_connected([this](uint32_t client_id) {
        on_client_connected(client_id);
    });
    m_network->set_on_client_disconnected([this](uint32_t client_id) {
        on_client_disconnected(client_id);
    });
    m_network->set_on_packet_received([this](uint32_t client_id, const engine::NetworkPacket& packet) {
        on_packet_received(client_id, packet);
    });
    m_network_handler = std::make_unique<BagarioNetworkHandler>(m_network);
    m_packet_sender = std::make_unique<BagarioPacketSender>(m_network);
    m_session = std::make_unique<BagarioSession>();
    NetworkCallbacks callbacks;
    callbacks.on_connect = [this](uint32_t client_id, const protocol::ClientConnectPayload& p) {
        handle_client_connect(client_id, p);
    };
    callbacks.on_disconnect = [this](uint32_t client_id, const protocol::ClientDisconnectPayload& p) {
        handle_client_disconnect(client_id, p);
    };
    callbacks.on_ping = [this](uint32_t client_id, const protocol::ClientPingPayload& p) {
        handle_client_ping(client_id, p);
    };
    callbacks.on_input = [this](uint32_t client_id, const protocol::ClientInputPayload& p) {
        handle_client_input(client_id, p);
    };
    callbacks.on_split = [this](uint32_t client_id, const protocol::ClientSplitPayload& p) {
        handle_client_split(client_id, p);
    };
    callbacks.on_eject_mass = [this](uint32_t client_id, const protocol::ClientEjectMassPayload& p) {
        handle_client_eject_mass(client_id, p);
    };
    m_network_handler->set_callbacks(callbacks);
    SessionCallbacks session_callbacks;
    session_callbacks.on_entity_spawn = [this](const protocol::ServerEntitySpawnPayload& payload) {
        m_packet_sender->broadcast_entity_spawn(payload);
    };
    session_callbacks.on_entity_destroy = [this](const protocol::ServerEntityDestroyPayload& payload) {
        m_packet_sender->broadcast_entity_destroy(payload);
    };
    session_callbacks.on_player_eliminated = [this](uint32_t player_id, uint32_t killer_id) {
        std::lock_guard<std::mutex> lock(m_players_mutex);
        auto it = m_player_to_client.find(player_id);
        if (it != m_player_to_client.end()) {
            protocol::ServerPlayerEatenPayload payload;
            payload.player_id = player_id;
            payload.killer_id = killer_id;
            payload.final_mass = m_session->get_player_total_mass(player_id);
            m_packet_sender->send_player_eaten(it->second, payload);
        }
    };
    m_session->set_callbacks(session_callbacks);
    m_session->init();
    auto now = std::chrono::steady_clock::now();
    m_last_snapshot_time = now;
    m_last_leaderboard_time = now;
    m_last_tick_time = now;
    m_running = true;
    std::cout << "[BagarioServer] Server started successfully" << std::endl;
    return true;
}

void BagarioServer::stop() {
    if (!m_running)
        return;
    std::cout << "[BagarioServer] Stopping server..." << std::endl;
    m_running = false;

    if (m_session)
        m_session->shutdown();
    if (m_network)
        m_network->stop_server();
    m_session.reset();
    m_network_handler.reset();
    m_packet_sender.reset();
    std::cout << "[BagarioServer] Server stopped" << std::endl;
}

void BagarioServer::run() {
    if (!m_running)
    return;
    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - m_last_tick_time).count();
    m_last_tick_time = now;

    m_network->update(dt);
    m_network_handler->process_packets();
    m_session->update(dt);
    auto snapshot_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_last_snapshot_time
    ).count();
    if (snapshot_elapsed >= config::SNAPSHOT_INTERVAL_MS) {
        broadcast_snapshot();
        m_last_snapshot_time = now;
    }
    auto leaderboard_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_last_leaderboard_time
    ).count();
    if (leaderboard_elapsed >= 1000 / config::LEADERBOARD_UPDATE_RATE) {
        broadcast_leaderboard();
        m_last_leaderboard_time = now;
    }
    auto tick_duration = std::chrono::steady_clock::now() - now;
    auto target_duration = std::chrono::milliseconds(config::TICK_INTERVAL_MS);
    if (tick_duration < target_duration)
        std::this_thread::sleep_for(target_duration - tick_duration);
}

size_t BagarioServer::get_player_count() const {
    std::lock_guard<std::mutex> lock(m_players_mutex);

    return m_players.size();
}

void BagarioServer::on_client_connected(uint32_t client_id) {
    std::cout << "[BagarioServer] Client " << client_id << " connected" << std::endl;
}

void BagarioServer::on_client_disconnected(uint32_t client_id) {
    std::cout << "[BagarioServer] Client " << client_id << " disconnected" << std::endl;
    std::lock_guard<std::mutex> lock(m_players_mutex);
    auto it = m_players.find(client_id);

    if (it != m_players.end()) {
        uint32_t player_id = it->second.player_id;
        m_session->remove_player(player_id);
        m_player_to_client.erase(player_id);
        m_players.erase(it);
        std::cout << "[BagarioServer] Player " << player_id << " removed" << std::endl;
    }
}

void BagarioServer::on_packet_received(uint32_t client_id, const engine::NetworkPacket& packet) {
    // Handled by network handler in process_packets()
}

void BagarioServer::handle_client_connect(uint32_t client_id, const protocol::ClientConnectPayload& payload) {
    std::cout << "[BagarioServer] Client " << client_id << " requesting connection as '"
              << payload.player_name << "'" << std::endl;
    {
        std::lock_guard<std::mutex> lock(m_players_mutex);
        if (m_players.size() >= config::MAX_PLAYERS) {
            protocol::ServerRejectPayload reject;
            reject.reason_code = protocol::RejectReason::SERVER_FULL;
            reject.set_message("Server is full");
            m_packet_sender->send_reject(client_id, reject);
            return;
        }
    }
    uint32_t player_id = generate_player_id();
    uint32_t color = generate_color();
    std::string name(payload.player_name);
    {
        std::lock_guard<std::mutex> lock(m_players_mutex);
        PlayerInfo info;
        info.client_id = client_id;
        info.player_id = player_id;
        info.name = name;
        info.color = color;
        info.in_game = true;
        info.last_activity = std::chrono::steady_clock::now();
        m_players[client_id] = info;
        m_player_to_client[player_id] = client_id;
    }
    protocol::ServerAcceptPayload accept;
    accept.assigned_player_id = player_id;
    m_packet_sender->send_accept(client_id, accept);
    m_session->add_player(player_id, name, color);
    std::cout << "[BagarioServer] Player " << player_id << " (" << name << ") joined" << std::endl;
}

void BagarioServer::handle_client_disconnect(uint32_t client_id, const protocol::ClientDisconnectPayload& payload) {
    on_client_disconnected(client_id);
}

void BagarioServer::handle_client_ping(uint32_t client_id, const protocol::ClientPingPayload& payload) {
    protocol::ServerPongPayload pong;
    pong.client_timestamp = payload.client_timestamp;
    pong.server_timestamp = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
    m_packet_sender->send_pong(client_id, pong);
}

void BagarioServer::handle_client_input(uint32_t client_id, const protocol::ClientInputPayload& payload) {
    std::lock_guard<std::mutex> lock(m_players_mutex);
    auto it = m_players.find(client_id);

    if (it == m_players.end())
        return;
    it->second.last_activity = std::chrono::steady_clock::now();
    m_session->set_player_target(it->second.player_id, payload.target_x, payload.target_y);
}

void BagarioServer::handle_client_split(uint32_t client_id, const protocol::ClientSplitPayload& payload) {
    std::lock_guard<std::mutex> lock(m_players_mutex);
    auto it = m_players.find(client_id);

    if (it == m_players.end())
        return;
    m_session->player_split(it->second.player_id);
}

void BagarioServer::handle_client_eject_mass(uint32_t client_id, const protocol::ClientEjectMassPayload& payload) {
    std::lock_guard<std::mutex> lock(m_players_mutex);
    auto it = m_players.find(client_id);

    if (it == m_players.end())
        return;
    m_session->player_eject_mass(it->second.player_id, payload.direction_x, payload.direction_y);
}

uint32_t BagarioServer::generate_player_id() {
    return m_next_player_id++;
}

uint32_t BagarioServer::generate_color() {
    std::uniform_int_distribution<uint32_t> dist(0x40, 0xFF);
    uint8_t r = static_cast<uint8_t>(dist(m_rng));
    uint8_t g = static_cast<uint8_t>(dist(m_rng));
    uint8_t b = static_cast<uint8_t>(dist(m_rng));

    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

void BagarioServer::broadcast_snapshot() {
    auto entities = m_session->get_snapshot();
    protocol::ServerSnapshotPayload header;
    header.server_tick = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()
    );
    header.entity_count = static_cast<uint16_t>(entities.size());

    m_packet_sender->broadcast_snapshot(header, entities);
}

void BagarioServer::broadcast_leaderboard() {
    auto entries = m_session->get_leaderboard();
    protocol::ServerLeaderboardPayload header;
    header.entry_count = static_cast<uint8_t>(std::min(entries.size(),
                                              static_cast<size_t>(config::LEADERBOARD_SIZE)));

    m_packet_sender->broadcast_leaderboard(header, entries);
}

}
