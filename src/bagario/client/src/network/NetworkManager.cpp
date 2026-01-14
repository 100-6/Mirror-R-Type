#include "network/NetworkManager.hpp"
#include "plugin_manager/PluginPaths.hpp"
#include <iostream>
#include <chrono>

namespace bagario::client {

NetworkManager::NetworkManager() = default;

NetworkManager::~NetworkManager() {
    shutdown();
}

bool NetworkManager::initialize() {
    try {
        // Load ENet network plugin
        m_network = m_plugin_manager.load_plugin<engine::INetworkPlugin>(
            engine::PluginPaths::get_plugin_path(engine::PluginPaths::ENET_NETWORK),
            "create_network_plugin"
        );

        if (!m_network) {
            std::cerr << "[NetworkManager] Failed to load network plugin\n";
            return false;
        }

        // Create handler and sender
        m_handler = std::make_unique<BagarioClientNetworkHandler>(m_network);
        m_sender = std::make_unique<BagarioClientPacketSender>(m_network);

        // Setup internal callbacks
        setup_internal_callbacks();

        std::cout << "[NetworkManager] Initialized successfully\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[NetworkManager] Initialization error: " << e.what() << "\n";
        return false;
    }
}

void NetworkManager::shutdown() {
    if (m_state != ConnectionState::DISCONNECTED) {
        disconnect();
    }

    m_handler.reset();
    m_sender.reset();
    m_network = nullptr;
    // PluginManager handles plugin destruction
}

bool NetworkManager::connect(const std::string& host, uint16_t tcp_port, uint16_t udp_port) {
    if (!m_network) {
        m_connection_error = "Network not initialized";
        return false;
    }

    if (m_state != ConnectionState::DISCONNECTED) {
        disconnect();
    }

    m_server_host = host;
    m_tcp_port = tcp_port;
    m_udp_port = udp_port;
    m_connection_error.clear();

    // Set connection callbacks
    m_network->set_on_connected([this]() {
        std::cout << "[NetworkManager] TCP connected to server\n";
        // Now connect UDP
        if (m_network->connect_udp(m_server_host, m_udp_port)) {
            m_state = ConnectionState::CONNECTED;
            std::cout << "[NetworkManager] UDP connected, ready to join\n";
        } else {
            m_connection_error = "Failed to connect UDP";
            m_state = ConnectionState::DISCONNECTED;
            m_network->disconnect();
        }
    });

    m_network->set_on_disconnected([this]() {
        handle_disconnected();
    });

    // Start TCP connection
    m_state = ConnectionState::CONNECTING;
    if (!m_network->connect_tcp(host, tcp_port)) {
        m_connection_error = "Failed to connect TCP";
        m_state = ConnectionState::DISCONNECTED;
        return false;
    }

    std::cout << "[NetworkManager] Connecting to " << host << ":" << tcp_port << "/" << udp_port << "\n";
    return true;
}

void NetworkManager::disconnect() {
    if (m_state == ConnectionState::DISCONNECTED) {
        return;
    }

    if (m_network && m_player_id != 0 && m_sender) {
        m_sender->send_disconnect(m_player_id, protocol::DisconnectReason::USER_QUIT);
    }

    if (m_network) {
        m_network->disconnect();
    }

    m_state = ConnectionState::DISCONNECTED;
    m_player_id = 0;
    m_input_sequence = 0;
    m_ping_ms = -1;
    m_connection_error.clear();
    m_pending_skin_data.clear();

    std::cout << "[NetworkManager] Disconnected\n";
}

void NetworkManager::request_join(const std::string& player_name, const PlayerSkin& skin) {
    if (m_state != ConnectionState::CONNECTED || !m_sender) {
        std::cerr << "[NetworkManager] Cannot join: not connected\n";
        return;
    }

    // Store skin data to send after we receive SERVER_ACCEPT
    m_pending_skin_data = skin.serialize();

    // Send connection request
    m_sender->send_connect(player_name);
    std::cout << "[NetworkManager] Join request sent for player: " << player_name << "\n";
}

void NetworkManager::update(float dt) {
    if (!m_network) {
        return;
    }

    // Update network plugin
    m_network->update(dt);

    // Process incoming packets
    if (m_handler) {
        m_handler->process_packets();
    }

    // Periodic ping
    if (m_state == ConnectionState::IN_GAME) {
        m_ping_timer += dt;
        if (m_ping_timer >= PING_INTERVAL) {
            send_ping();
            m_ping_timer = 0.0f;
        }
    }
}

void NetworkManager::send_input(float target_x, float target_y) {
    if (m_state != ConnectionState::IN_GAME || !m_sender) {
        return;
    }

    m_sender->send_input(m_player_id, target_x, target_y, ++m_input_sequence);
}

void NetworkManager::send_split() {
    if (m_state != ConnectionState::IN_GAME || !m_sender) {
        return;
    }

    m_sender->send_split(m_player_id);
}

void NetworkManager::send_eject_mass(float dir_x, float dir_y) {
    if (m_state != ConnectionState::IN_GAME || !m_sender) {
        return;
    }

    m_sender->send_eject_mass(m_player_id, dir_x, dir_y);
}

void NetworkManager::send_skin(const PlayerSkin& skin) {
    if (m_state != ConnectionState::IN_GAME || !m_sender) {
        return;
    }

    auto skin_data = skin.serialize();
    m_sender->send_skin(m_player_id, skin_data);
}

void NetworkManager::send_ping() {
    if (m_state != ConnectionState::IN_GAME || !m_sender) {
        return;
    }

    m_last_ping_time = get_current_timestamp_ms();
    m_sender->send_ping(m_player_id, m_last_ping_time);
}

void NetworkManager::set_callbacks(const ClientNetworkCallbacks& callbacks) {
    m_external_callbacks = callbacks;
}

void NetworkManager::setup_internal_callbacks() {
    ClientNetworkCallbacks internal_callbacks;

    internal_callbacks.on_accept = [this](const protocol::ServerAcceptPayload& payload) {
        handle_accept(payload);
        if (m_external_callbacks.on_accept) {
            m_external_callbacks.on_accept(payload);
        }
    };

    internal_callbacks.on_reject = [this](const protocol::ServerRejectPayload& payload) {
        handle_reject(payload);
        if (m_external_callbacks.on_reject) {
            m_external_callbacks.on_reject(payload);
        }
    };

    internal_callbacks.on_pong = [this](const protocol::ServerPongPayload& payload) {
        handle_pong(payload);
        if (m_external_callbacks.on_pong) {
            m_external_callbacks.on_pong(payload);
        }
    };

    // Pass through other callbacks
    internal_callbacks.on_snapshot = [this](const protocol::ServerSnapshotPayload& header,
                                            const std::vector<protocol::EntityState>& entities) {
        if (m_external_callbacks.on_snapshot) {
            m_external_callbacks.on_snapshot(header, entities);
        }
    };

    internal_callbacks.on_entity_spawn = [this](const protocol::ServerEntitySpawnPayload& payload) {
        if (m_external_callbacks.on_entity_spawn) {
            m_external_callbacks.on_entity_spawn(payload);
        }
    };

    internal_callbacks.on_entity_destroy = [this](const protocol::ServerEntityDestroyPayload& payload) {
        if (m_external_callbacks.on_entity_destroy) {
            m_external_callbacks.on_entity_destroy(payload);
        }
    };

    internal_callbacks.on_player_eaten = [this](const protocol::ServerPlayerEatenPayload& payload) {
        if (m_external_callbacks.on_player_eaten) {
            m_external_callbacks.on_player_eaten(payload);
        }
    };

    internal_callbacks.on_leaderboard = [this](const protocol::ServerLeaderboardPayload& header,
                                               const std::vector<protocol::LeaderboardEntry>& entries) {
        if (m_external_callbacks.on_leaderboard) {
            m_external_callbacks.on_leaderboard(header, entries);
        }
    };

    internal_callbacks.on_player_skin = [this](uint32_t player_id, const std::vector<uint8_t>& skin_data) {
        if (m_external_callbacks.on_player_skin) {
            m_external_callbacks.on_player_skin(player_id, skin_data);
        }
    };

    internal_callbacks.on_disconnected = [this]() {
        handle_disconnected();
        if (m_external_callbacks.on_disconnected) {
            m_external_callbacks.on_disconnected();
        }
    };

    m_handler->set_callbacks(internal_callbacks);
}

void NetworkManager::handle_accept(const protocol::ServerAcceptPayload& payload) {
    m_player_id = payload.assigned_player_id;
    m_map_width = payload.map_width;
    m_map_height = payload.map_height;
    m_state = ConnectionState::IN_GAME;

    std::cout << "[NetworkManager] Joined game! Player ID: " << m_player_id
              << ", Map: " << m_map_width << "x" << m_map_height << "\n";

    // Send pending skin data now that we have our player_id
    if (!m_pending_skin_data.empty() && m_sender) {
        m_sender->send_skin(m_player_id, m_pending_skin_data);
        m_pending_skin_data.clear();
        std::cout << "[NetworkManager] Skin data sent\n";
    }
}

void NetworkManager::handle_reject(const protocol::ServerRejectPayload& payload) {
    m_connection_error = std::string(payload.reason_message);
    m_state = ConnectionState::DISCONNECTED;

    std::cerr << "[NetworkManager] Connection rejected: " << m_connection_error << "\n";

    if (m_network) {
        m_network->disconnect();
    }
}

void NetworkManager::handle_pong(const protocol::ServerPongPayload& payload) {
    uint32_t now = get_current_timestamp_ms();
    if (payload.client_timestamp == m_last_ping_time) {
        m_ping_ms = static_cast<int>(now - m_last_ping_time);
    }
}

void NetworkManager::handle_disconnected() {
    if (m_state == ConnectionState::DISCONNECTED) {
        return;
    }

    m_state = ConnectionState::DISCONNECTED;
    m_player_id = 0;
    m_connection_error = "Disconnected from server";

    std::cout << "[NetworkManager] Connection lost\n";
}

uint32_t NetworkManager::get_current_timestamp_ms() const {
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    return static_cast<uint32_t>(ms.count());
}

}  // namespace bagario::client
