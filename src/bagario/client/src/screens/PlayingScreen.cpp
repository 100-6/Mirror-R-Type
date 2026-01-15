#include "screens/PlayingScreen.hpp"
#include "BagarioConfig.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace bagario {

PlayingScreen::PlayingScreen(LocalGameState& game_state, int screen_width, int screen_height)
    : BaseScreen(game_state, screen_width, screen_height)
{
}

PlayingScreen::~PlayingScreen() = default;

void PlayingScreen::initialize() {
    client_game_state_ = std::make_unique<client::ClientGameState>();
    camera_ = std::make_unique<client::Camera>(
        static_cast<float>(screen_width_),
        static_cast<float>(screen_height_)
    );
}

void PlayingScreen::set_network_manager(client::NetworkManager* network) {
    network_ = network;
    if (network_) {
        setup_network_callbacks();
    }
}

void PlayingScreen::setup_network_callbacks() {
    if (!network_) return;

    client::ClientNetworkCallbacks callbacks;

    callbacks.on_accept = [this](const protocol::ServerAcceptPayload& payload) {
        std::cout << "[PlayingScreen] Connected! Player ID: " << payload.assigned_player_id << "\n";
        client_game_state_->set_local_player_id(payload.assigned_player_id);
        client_game_state_->set_map_size(payload.map_width, payload.map_height);
        camera_->set_map_bounds(payload.map_width, payload.map_height);
        is_connecting_ = false;
    };

    callbacks.on_reject = [this](const protocol::ServerRejectPayload& payload) {
        std::cerr << "[PlayingScreen] Connection rejected: " << payload.reason_message << "\n";
        connection_error_ = payload.reason_message;
        connection_failed_ = true;
        is_connecting_ = false;
    };

    callbacks.on_snapshot = [this](const protocol::ServerSnapshotPayload& header,
                                   const std::vector<protocol::EntityState>& entities) {
        client_game_state_->update_from_snapshot(header, entities);
    };

    callbacks.on_entity_spawn = [this](const protocol::ServerEntitySpawnPayload& payload) {
        client_game_state_->handle_entity_spawn(payload);
    };

    callbacks.on_entity_destroy = [this](const protocol::ServerEntityDestroyPayload& payload) {
        client_game_state_->handle_entity_destroy(payload);
    };

    callbacks.on_player_eaten = [this](const protocol::ServerPlayerEatenPayload& payload) {
        if (payload.player_id == client_game_state_->get_local_player_id()) {
            std::cout << "[PlayingScreen] You were eaten! Final mass: " << payload.final_mass << "\n";
            // Could show death screen here
        }
    };

    callbacks.on_leaderboard = [this](const protocol::ServerLeaderboardPayload& header,
                                      const std::vector<protocol::LeaderboardEntry>& entries) {
        client_game_state_->update_leaderboard(header, entries);
    };

    callbacks.on_player_skin = [this](uint32_t player_id, const std::vector<uint8_t>& skin_data) {
        client_game_state_->update_player_skin(player_id, skin_data);
    };

    callbacks.on_disconnected = [this]() {
        std::cout << "[PlayingScreen] Disconnected from server\n";
        connection_error_ = "Disconnected from server";
        connection_failed_ = true;
    };

    network_->set_callbacks(callbacks);
}

void PlayingScreen::on_enter() {
    std::cout << "[PlayingScreen] Entering game screen\n";

    // Reset state
    is_connecting_ = true;
    connection_failed_ = false;
    join_requested_ = false;  // Reset join request flag for new connection
    connection_error_.clear();
    first_update_ = true;
    input_send_timer_ = 0.0f;

    if (client_game_state_) {
        client_game_state_->clear();
    }

    // Connect to server
    if (network_) {
        if (network_->connect(game_state_.server_ip, 5002, 5003)) {
            // Connection initiated, wait for on_connected callback
            // Then request join will be sent
        } else {
            connection_error_ = "Failed to connect to server";
            connection_failed_ = true;
            is_connecting_ = false;
        }
    } else {
        connection_error_ = "Network not available";
        connection_failed_ = true;
        is_connecting_ = false;
    }
}

void PlayingScreen::on_exit() {
    std::cout << "[PlayingScreen] Exiting game screen\n";

    if (network_) {
        network_->disconnect();
    }

    if (client_game_state_) {
        client_game_state_->clear();
    }
}

void PlayingScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    float dt = get_delta_time();

    // Handle connection state
    if (network_) {
        // If we're connected (TCP established) but haven't joined yet, request join
        // Use join_requested_ flag to ensure we only send ONE join request
        if (network_->get_state() == client::ConnectionState::CONNECTED && is_connecting_ && !join_requested_) {
            network_->request_join(game_state_.username, game_state_.skin);
            join_requested_ = true;  // Prevent sending multiple join requests
        }

        network_->update(dt);
    }

    // Handle return to menu on connection failure or Escape
    if (connection_failed_) {
        // Wait for any key to return to menu
        if (input->is_key_just_pressed(engine::Key::Escape) ||
            input->is_key_just_pressed(engine::Key::Enter) ||
            input->is_key_just_pressed(engine::Key::Space)) {
            if (on_screen_change_) {
                on_screen_change_(GameScreen::WELCOME);
            }
        }
        return;
    }

    // Only process game input when fully connected
    if (network_ && network_->is_connected()) {
        handle_mouse_input(input);
        handle_keyboard_input(input);

        // Update interpolation
        if (client_game_state_) {
            client_game_state_->update_interpolation(dt);
        }

        // Update camera to follow player
        if (client_game_state_ && camera_) {
            float center_x, center_y;
            uint32_t player_id = client_game_state_->get_local_player_id();
            if (client_game_state_->get_player_center(player_id, center_x, center_y)) {
                float total_mass = client_game_state_->get_player_total_mass(player_id);
                camera_->follow(center_x, center_y, total_mass);
            }
            camera_->update(dt);
        }
    }
}

void PlayingScreen::handle_mouse_input(engine::IInputPlugin* input) {
    if (!network_ || !network_->is_connected() || !camera_) return;

    float dt = get_delta_time();
    input_send_timer_ += dt;

    if (input_send_timer_ >= INPUT_SEND_INTERVAL) {
        input_send_timer_ = 0.0f;

        // Get mouse position and convert to world coordinates
        auto mouse_pos = input->get_mouse_position();
        auto world_pos = camera_->screen_to_world(mouse_pos.x, mouse_pos.y);

        network_->send_input(world_pos.x, world_pos.y);
    }
}

void PlayingScreen::handle_keyboard_input(engine::IInputPlugin* input) {
    if (!network_ || !network_->is_connected()) return;

    // Escape - return to menu
    if (input->is_key_just_pressed(engine::Key::Escape)) {
        if (on_screen_change_) {
            on_screen_change_(GameScreen::WELCOME);
        }
        return;
    }

    // Space - split
    if (input->is_key_just_pressed(engine::Key::Space)) {
        network_->send_split();
    }

    // W - eject mass
    if (input->is_key_just_pressed(engine::Key::W)) {
        if (camera_ && client_game_state_) {
            // Get direction from player to mouse
            float center_x, center_y;
            uint32_t player_id = client_game_state_->get_local_player_id();
            if (client_game_state_->get_player_center(player_id, center_x, center_y)) {
                auto mouse_pos = input->get_mouse_position();
                auto world_pos = camera_->screen_to_world(mouse_pos.x, mouse_pos.y);

                float dx = world_pos.x - center_x;
                float dy = world_pos.y - center_y;
                float len = std::sqrt(dx * dx + dy * dy);

                if (len > 0.001f) {
                    network_->send_eject_mass(dx / len, dy / len);
                }
            }
        }
    }
}

void PlayingScreen::draw(engine::IGraphicsPlugin* graphics) {
    // Show connection status screen if not fully connected
    if (!network_ || !network_->is_connected()) {
        draw_connection_status(graphics);
        return;
    }

    // Draw game world
    draw_background(graphics);
    draw_grid(graphics);
    draw_entities(graphics);
    draw_player_names(graphics);

    // Draw UI overlay
    draw_ui_overlay(graphics);
}

void PlayingScreen::draw_background(engine::IGraphicsPlugin* graphics) {
    // Dark background
    graphics->clear(engine::Color{20, 25, 30, 255});
}

void PlayingScreen::draw_grid(engine::IGraphicsPlugin* graphics) {
    if (!camera_ || !client_game_state_) return;

    const float grid_spacing = 50.0f;
    engine::Color grid_color{40, 45, 55, 100};

    // Get visible area
    float left = camera_->get_view_left();
    float right = camera_->get_view_right();
    float top = camera_->get_view_top();
    float bottom = camera_->get_view_bottom();

    // Clamp to map bounds
    left = std::max(0.0f, left);
    right = std::min(client_game_state_->get_map_width(), right);
    top = std::max(0.0f, top);
    bottom = std::min(client_game_state_->get_map_height(), bottom);

    // Draw vertical lines
    float start_x = std::floor(left / grid_spacing) * grid_spacing;
    for (float x = start_x; x <= right; x += grid_spacing) {
        auto p1 = camera_->world_to_screen(x, top);
        auto p2 = camera_->world_to_screen(x, bottom);
        graphics->draw_line(p1, p2, grid_color, 1.0f);
    }

    // Draw horizontal lines
    float start_y = std::floor(top / grid_spacing) * grid_spacing;
    for (float y = start_y; y <= bottom; y += grid_spacing) {
        auto p1 = camera_->world_to_screen(left, y);
        auto p2 = camera_->world_to_screen(right, y);
        graphics->draw_line(p1, p2, grid_color, 1.0f);
    }
}

void PlayingScreen::draw_entities(engine::IGraphicsPlugin* graphics) {
    if (!client_game_state_ || !camera_) return;

    // Sort entities by mass for proper layering (smallest first)
    std::vector<const client::CachedEntity*> sorted_entities;
    for (const auto& [id, entity] : client_game_state_->get_entities()) {
        sorted_entities.push_back(&entity);
    }

    std::sort(sorted_entities.begin(), sorted_entities.end(),
        [](const client::CachedEntity* a, const client::CachedEntity* b) {
            return a->mass < b->mass;
        });

    // Draw all entities
    for (const auto* entity : sorted_entities) {
        draw_entity(graphics, *entity);
    }
}

void PlayingScreen::draw_entity(engine::IGraphicsPlugin* graphics, const client::CachedEntity& entity) {
    float x = entity.get_interpolated_x();
    float y = entity.get_interpolated_y();
    float radius = entity.get_radius();

    // Culling - skip if not visible
    if (!camera_->is_visible(x, y, radius)) {
        return;
    }

    switch (entity.type) {
        case protocol::EntityType::PLAYER_CELL:
            draw_player_cell(graphics, entity);
            break;
        case protocol::EntityType::FOOD:
            draw_food(graphics, entity);
            break;
        case protocol::EntityType::VIRUS:
            draw_virus(graphics, entity);
            break;
        case protocol::EntityType::EJECTED_MASS:
            draw_ejected_mass(graphics, entity);
            break;
    }
}

void PlayingScreen::draw_player_cell(engine::IGraphicsPlugin* graphics, const client::CachedEntity& entity) {
    float x = entity.get_interpolated_x();
    float y = entity.get_interpolated_y();
    float radius = entity.get_radius();

    auto screen_pos = camera_->world_to_screen(x, y);
    float screen_radius = radius * camera_->get_zoom();

    // Get color (from skin or default)
    engine::Color color;
    if (entity.has_skin) {
        color = entity.skin.primary;
    } else {
        color = uint32_to_color(entity.color);
    }

    // Draw cell body
    graphics->draw_circle(screen_pos, screen_radius, color);

    // Draw outline (darker color)
    engine::Color outline_color = darken_color(color, 0.7f);
    float outline_thickness = std::max(2.0f, screen_radius * 0.05f);

    // Draw outline as slightly larger circle behind
    graphics->draw_circle(screen_pos, screen_radius + outline_thickness / 2, outline_color);
    graphics->draw_circle(screen_pos, screen_radius - outline_thickness / 2, color);
}

void PlayingScreen::draw_food(engine::IGraphicsPlugin* graphics, const client::CachedEntity& entity) {
    float x = entity.get_interpolated_x();
    float y = entity.get_interpolated_y();
    float radius = entity.get_radius();

    auto screen_pos = camera_->world_to_screen(x, y);
    float screen_radius = radius * camera_->get_zoom();

    // Food is small, use a minimum size for visibility
    screen_radius = std::max(3.0f, screen_radius);

    engine::Color color = uint32_to_color(entity.color);
    graphics->draw_circle(screen_pos, screen_radius, color);
}

void PlayingScreen::draw_virus(engine::IGraphicsPlugin* graphics, const client::CachedEntity& entity) {
    float x = entity.get_interpolated_x();
    float y = entity.get_interpolated_y();
    float radius = entity.get_radius();

    auto screen_pos = camera_->world_to_screen(x, y);
    float screen_radius = radius * camera_->get_zoom();

    // Green virus color
    engine::Color virus_color{0, 200, 0, 255};
    engine::Color virus_outline{0, 150, 0, 255};

    // Draw main body
    graphics->draw_circle(screen_pos, screen_radius, virus_color);
    graphics->draw_circle(screen_pos, screen_radius * 0.8f, virus_outline);
}

void PlayingScreen::draw_ejected_mass(engine::IGraphicsPlugin* graphics, const client::CachedEntity& entity) {
    float x = entity.get_interpolated_x();
    float y = entity.get_interpolated_y();
    float radius = entity.get_radius();

    auto screen_pos = camera_->world_to_screen(x, y);
    float screen_radius = radius * camera_->get_zoom();

    engine::Color color = uint32_to_color(entity.color);
    graphics->draw_circle(screen_pos, screen_radius, color);
}

void PlayingScreen::draw_player_names(engine::IGraphicsPlugin* graphics) {
    // This would require storing player names - simplified for now
    // Names could be added to the snapshot or entity spawn data
}

void PlayingScreen::draw_ui_overlay(engine::IGraphicsPlugin* graphics) {
    draw_leaderboard(graphics);
    draw_minimap(graphics);
    draw_score(graphics);
}

void PlayingScreen::draw_leaderboard(engine::IGraphicsPlugin* graphics) {
    if (!client_game_state_) return;

    const auto& leaderboard = client_game_state_->get_leaderboard();
    if (leaderboard.entries.empty()) return;

    // Draw leaderboard background
    float lb_x = static_cast<float>(screen_width_) - 220.0f;
    float lb_y = 10.0f;
    float lb_width = 210.0f;
    float lb_height = 30.0f + leaderboard.entries.size() * 25.0f;

    engine::Color bg_color{20, 20, 30, 200};
    graphics->draw_rectangle(engine::Rectangle{lb_x, lb_y, lb_width, lb_height}, bg_color);

    // Title
    graphics->draw_text("Leaderboard", {lb_x + 10, lb_y + 5}, engine::Color{255, 255, 255, 255},
                       engine::INVALID_HANDLE, 20);

    // Entries
    float entry_y = lb_y + 35.0f;
    int rank = 1;
    for (const auto& entry : leaderboard.entries) {
        std::string text = std::to_string(rank) + ". " + std::string(entry.player_name);

        // Highlight local player
        engine::Color text_color{200, 200, 200, 255};
        if (entry.player_id == client_game_state_->get_local_player_id()) {
            text_color = {76, 175, 80, 255};  // Green for local player
        }

        graphics->draw_text(text, {lb_x + 10, entry_y}, text_color, engine::INVALID_HANDLE, 16);

        // Mass on right side
        std::string mass_text = std::to_string(static_cast<int>(entry.total_mass));
        graphics->draw_text(mass_text, {lb_x + lb_width - 50, entry_y},
                           engine::Color{150, 150, 150, 255}, engine::INVALID_HANDLE, 16);

        entry_y += 25.0f;
        rank++;

        if (rank > 10) break;  // Max 10 entries
    }
}

void PlayingScreen::draw_minimap(engine::IGraphicsPlugin* graphics) {
    if (!client_game_state_ || !camera_) return;

    // Minimap in bottom-right corner
    float mm_size = 150.0f;
    float mm_x = static_cast<float>(screen_width_) - mm_size - 10.0f;
    float mm_y = static_cast<float>(screen_height_) - mm_size - 10.0f;

    // Background
    engine::Color bg_color{20, 20, 30, 180};
    graphics->draw_rectangle(engine::Rectangle{mm_x, mm_y, mm_size, mm_size}, bg_color);

    // Border
    engine::Color border_color{60, 60, 70, 255};
    graphics->draw_line({mm_x, mm_y}, {mm_x + mm_size, mm_y}, border_color, 1.0f);
    graphics->draw_line({mm_x + mm_size, mm_y}, {mm_x + mm_size, mm_y + mm_size}, border_color, 1.0f);
    graphics->draw_line({mm_x + mm_size, mm_y + mm_size}, {mm_x, mm_y + mm_size}, border_color, 1.0f);
    graphics->draw_line({mm_x, mm_y + mm_size}, {mm_x, mm_y}, border_color, 1.0f);

    // Scale factor
    float map_width = client_game_state_->get_map_width();
    float map_height = client_game_state_->get_map_height();
    float scale = mm_size / std::max(map_width, map_height);

    // Draw player cells on minimap
    for (const auto& [id, entity] : client_game_state_->get_entities()) {
        if (entity.type == protocol::EntityType::PLAYER_CELL) {
            float dot_x = mm_x + entity.get_interpolated_x() * scale;
            float dot_y = mm_y + entity.get_interpolated_y() * scale;
            float dot_radius = std::max(2.0f, std::sqrt(entity.mass) * 0.3f);

            engine::Color dot_color;
            if (entity.owner_id == client_game_state_->get_local_player_id()) {
                dot_color = {76, 175, 80, 255};  // Green for local player
            } else {
                dot_color = uint32_to_color(entity.color);
            }

            graphics->draw_circle({dot_x, dot_y}, dot_radius, dot_color);
        }
    }

    // Draw current viewport rectangle
    float view_x = mm_x + camera_->get_view_left() * scale;
    float view_y = mm_y + camera_->get_view_top() * scale;
    float view_w = (camera_->get_view_right() - camera_->get_view_left()) * scale;
    float view_h = (camera_->get_view_bottom() - camera_->get_view_top()) * scale;

    engine::Color view_color{255, 255, 255, 100};
    graphics->draw_line({view_x, view_y}, {view_x + view_w, view_y}, view_color, 1.0f);
    graphics->draw_line({view_x + view_w, view_y}, {view_x + view_w, view_y + view_h}, view_color, 1.0f);
    graphics->draw_line({view_x + view_w, view_y + view_h}, {view_x, view_y + view_h}, view_color, 1.0f);
    graphics->draw_line({view_x, view_y + view_h}, {view_x, view_y}, view_color, 1.0f);
}

void PlayingScreen::draw_score(engine::IGraphicsPlugin* graphics) {
    if (!client_game_state_) return;

    uint32_t player_id = client_game_state_->get_local_player_id();
    float total_mass = client_game_state_->get_player_total_mass(player_id);

    std::string score_text = "Score: " + std::to_string(static_cast<int>(total_mass));

    // Draw score at top-left
    graphics->draw_text(score_text, {10, 10}, engine::Color{255, 255, 255, 255},
                       engine::INVALID_HANDLE, 24);

    // Draw ping
    if (network_) {
        int ping = network_->get_ping_ms();
        if (ping >= 0) {
            std::string ping_text = "Ping: " + std::to_string(ping) + "ms";
            graphics->draw_text(ping_text, {10, 40}, engine::Color{150, 150, 150, 255},
                               engine::INVALID_HANDLE, 16);
        }
    }
}

void PlayingScreen::draw_connection_status(engine::IGraphicsPlugin* graphics) {
    graphics->clear(engine::Color{20, 25, 30, 255});

    float center_x = static_cast<float>(screen_width_) / 2.0f;
    float center_y = static_cast<float>(screen_height_) / 2.0f;

    if (connection_failed_) {
        // Show error message
        graphics->draw_text("Connection Failed", {center_x - 100, center_y - 50},
                           engine::Color{244, 67, 54, 255}, engine::INVALID_HANDLE, 30);

        graphics->draw_text(connection_error_, {center_x - 150, center_y},
                           engine::Color{200, 200, 200, 255}, engine::INVALID_HANDLE, 20);

        graphics->draw_text("Press any key to return to menu", {center_x - 150, center_y + 50},
                           engine::Color{150, 150, 150, 255}, engine::INVALID_HANDLE, 16);
    } else if (is_connecting_) {
        // Show connecting message
        graphics->draw_text("Connecting...", {center_x - 80, center_y},
                           engine::Color{76, 175, 80, 255}, engine::INVALID_HANDLE, 30);
    }
}

engine::Color PlayingScreen::uint32_to_color(uint32_t color) const {
    // Server uses RGBA format (0xRRGGBBAA)
    return engine::Color{
        static_cast<unsigned char>((color >> 24) & 0xFF),  // R
        static_cast<unsigned char>((color >> 16) & 0xFF),  // G
        static_cast<unsigned char>((color >> 8) & 0xFF),   // B
        static_cast<unsigned char>(color & 0xFF)           // A
    };
}

engine::Color PlayingScreen::darken_color(const engine::Color& color, float factor) const {
    return engine::Color{
        static_cast<unsigned char>(color.r * factor),
        static_cast<unsigned char>(color.g * factor),
        static_cast<unsigned char>(color.b * factor),
        color.a
    };
}

float PlayingScreen::get_delta_time() {
    auto now = std::chrono::steady_clock::now();

    if (first_update_) {
        first_update_ = false;
        last_update_time_ = now;
        return 0.016f;  // Assume 60fps for first frame
    }

    float dt = std::chrono::duration<float>(now - last_update_time_).count();
    last_update_time_ = now;

    // Clamp to reasonable values
    return std::clamp(dt, 0.001f, 0.1f);
}

}  // namespace bagario
