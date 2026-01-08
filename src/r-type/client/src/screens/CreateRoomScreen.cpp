#include "screens/CreateRoomScreen.hpp"
#include "screens/createroom/CreateRoomInitializer.hpp"
#include "screens/createroom/CreateRoomUpdater.hpp"
#include "screens/createroom/CreateRoomDrawer.hpp"
#include "screens/createroom/CreateRoomRenderer.hpp"
#include "NetworkClient.hpp"
#include "ScreenManager.hpp"
#include <iostream>

namespace rtype::client {

// ============================================================================
// STATIC HELPERS
// ============================================================================

const char* CreateRoomScreen::get_map_name(MapId id) {
    switch (id) {
        case MapId::NEBULA_OUTPOST: return "Nebula Outpost";
        case MapId::ASTEROID_BELT: return "Asteroid Belt";
        case MapId::BYDO_MOTHERSHIP: return "Bydo Mothership";
        default: return "Unknown";
    }
}

const char* CreateRoomScreen::get_step_title() const {
    switch (current_step_) {
        case Step::ROOM_INFO: return "Room Information";
        case Step::MAP_SELECTION: return "Select Map";
        case Step::DIFFICULTY: return "Choose Difficulty";
        case Step::GAME_MODE: return "Game Mode";
        default: return "";
    }
}

// ============================================================================
// CONSTRUCTOR
// ============================================================================

CreateRoomScreen::CreateRoomScreen(NetworkClient& network_client, int screen_width, int screen_height)
    : BaseScreen(network_client, screen_width, screen_height) {
}

// ============================================================================
// CONFIGURATION GETTERS
// ============================================================================

uint8_t CreateRoomScreen::get_configured_max_players() const {
    switch (game_mode_) {
        case protocol::GameMode::DUO: return 2;
        case protocol::GameMode::TRIO: return 3;
        case protocol::GameMode::SQUAD: return 4;
        default: return 4;
    }
}

// ============================================================================
// NAVIGATION & ROOM CREATION
// ============================================================================

void CreateRoomScreen::next_step() {
    if (current_step_ == Step::GAME_MODE) {
        create_room();
    } else {
        current_step_ = static_cast<Step>(static_cast<int>(current_step_) + 1);
    }
}

void CreateRoomScreen::previous_step() {
    if (current_step_ == Step::ROOM_INFO) {
        if (on_screen_change_) {
            on_screen_change_(GameScreen::MAIN_MENU);
        }
    } else {
        current_step_ = static_cast<Step>(static_cast<int>(current_step_) - 1);
    }
}

void CreateRoomScreen::create_room() {
    std::string room_name = fields_[0]->get_text();
    std::string password = fields_[1]->get_text();

    if (room_name.empty()) {
        room_name = "Room";
    }

    uint8_t max_players = get_configured_max_players();
    uint16_t map = get_configured_map_id();

    std::cout << "[CreateRoomScreen] Creating room on map: " << get_map_name(map_id_) << "\n";

    network_client_.send_create_room(room_name, password, game_mode_, difficulty_, map, max_players);

    if (on_room_created_) {
        on_room_created_(game_mode_, max_players);
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void CreateRoomScreen::initialize() {
    // Clear all UI elements
    labels_.clear();
    fields_.clear();
    buttons_.clear();
    mode_buttons_.clear();
    map_buttons_.clear();
    difficulty_buttons_.clear();
    nav_buttons_.clear();

    // Reset to defaults
    current_step_ = Step::ROOM_INFO;
    game_mode_ = protocol::GameMode::SQUAD;
    difficulty_ = protocol::Difficulty::NORMAL;
    map_id_ = MapId::NEBULA_OUTPOST;
    textures_.loaded = false;

    // Initialize each step using modular initializers
    createroom::Initializer::init_room_info_step(labels_, fields_, screen_width_);
    createroom::Initializer::init_map_selection_step(map_buttons_, map_id_, screen_width_);
    createroom::Initializer::init_difficulty_step();
    createroom::Initializer::init_game_mode_step();
    createroom::Initializer::init_navigation_buttons(
        nav_buttons_,
        [this]() { previous_step(); },
        [this]() { next_step(); },
        screen_width_,
        screen_height_
    );
}

void CreateRoomScreen::initialize_room_info_step() {
    createroom::Initializer::init_room_info_step(labels_, fields_, screen_width_);
}

void CreateRoomScreen::initialize_map_selection_step() {
    createroom::Initializer::init_map_selection_step(map_buttons_, map_id_, screen_width_);
}

void CreateRoomScreen::initialize_difficulty_step() {
    createroom::Initializer::init_difficulty_step();
}

void CreateRoomScreen::initialize_game_mode_step() {
    createroom::Initializer::init_game_mode_step();
}

void CreateRoomScreen::initialize_navigation_buttons() {
    createroom::Initializer::init_navigation_buttons(
        nav_buttons_,
        [this]() { previous_step(); },
        [this]() { next_step(); },
        screen_width_,
        screen_height_
    );
}

// ============================================================================
// UPDATE
// ============================================================================

void CreateRoomScreen::update(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    // Edit mode controls for navigation arrows
    if (edit_mode_) {
        std::cout << "[EDIT MODE] Spacing: " << button_spacing_ << " | Y-Offset: " << button_y_offset_
                  << " | Radius: " << button_radius_ << "\n";
        std::cout << "[EDIT MODE] Controls: Arrow Keys (spacing), W/S (vertical), +/- (radius), P (print), E (exit)\n";

        bool changed = false;

        // Arrow Left/Right - Adjust horizontal spacing
        if (input->is_key_pressed(engine::Key::Left)) {
            button_spacing_ -= move_speed_;
            changed = true;
        }
        if (input->is_key_pressed(engine::Key::Right)) {
            button_spacing_ += move_speed_;
            changed = true;
        }

        // W/S - Adjust vertical position
        if (input->is_key_pressed(engine::Key::W)) {
            button_y_offset_ += move_speed_;
            changed = true;
        }
        if (input->is_key_pressed(engine::Key::S)) {
            button_y_offset_ -= move_speed_;
            changed = true;
        }

        // +/- - Adjust button radius (use Add/Subtract for numpad)
        if (input->is_key_pressed(engine::Key::Equal) || input->is_key_pressed(engine::Key::Add)) {
            button_radius_ += 1.0f;
            changed = true;
            std::cout << "[EDIT MODE] Radius increased to " << button_radius_ << "\n";
        }
        if (input->is_key_pressed(engine::Key::Hyphen) || input->is_key_pressed(engine::Key::Subtract)) {
            if (button_radius_ > 10.0f) {  // Minimum size
                button_radius_ -= 1.0f;
                changed = true;
                std::cout << "[EDIT MODE] Radius decreased to " << button_radius_ << "\n";
            } else {
                std::cout << "[EDIT MODE] Radius at minimum (10.0f)\n";
            }
        }

        // P - Print current values
        if (input->is_key_pressed(engine::Key::P)) {
            std::cout << "\n=== NAVIGATION ARROW POSITIONS ===\n";
            std::cout << "button_spacing = " << button_spacing_ << "f;\n";
            std::cout << "button_y_offset = " << button_y_offset_ << "f;\n";
            std::cout << "button_radius = " << button_radius_ << "f;\n";
            std::cout << "==================================\n\n";
        }

        // E - Exit edit mode
        if (input->is_key_pressed(engine::Key::E)) {
            edit_mode_ = false;
            std::cout << "[EDIT MODE] Disabled. Final values:\n";
            std::cout << "  button_spacing = " << button_spacing_ << "f\n";
            std::cout << "  button_y_offset = " << button_y_offset_ << "f\n";
            std::cout << "  button_radius = " << button_radius_ << "f\n";
        }

        if (changed) {
            std::cout << "[EDIT MODE] Updated: spacing=" << button_spacing_
                     << " y_offset=" << button_y_offset_
                     << " radius=" << button_radius_ << "\n";
        }

        return;  // Skip normal updates in edit mode
    }

    // Update text fields only on ROOM_INFO step
    if (current_step_ == Step::ROOM_INFO) {
        update_room_info_step(graphics, input);
    }

    // Check if any text field is focused
    if (!createroom::Updater::is_any_field_focused(fields_)) {
        // Update step-specific content
        switch (current_step_) {
            case Step::MAP_SELECTION:
                update_map_selection_step(graphics, input);
                break;
            case Step::DIFFICULTY:
                update_difficulty_step(graphics, input);
                break;
            case Step::GAME_MODE:
                update_game_mode_step(graphics, input);
                break;
            default:
                break;
        }

        // Update navigation buttons
        createroom::Updater::update_navigation_buttons(nav_buttons_, graphics, input);
    }
}

void CreateRoomScreen::update_room_info_step(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    createroom::Updater::update_room_info_step(fields_, graphics, input);
}

void CreateRoomScreen::update_map_selection_step(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    createroom::Updater::update_map_selection_step(map_buttons_, map_id_, graphics, input);
}

void CreateRoomScreen::update_difficulty_step(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    createroom::Updater::update_difficulty_step(difficulty_, screen_width_, input);
}

void CreateRoomScreen::update_game_mode_step(engine::IGraphicsPlugin* graphics, engine::IInputPlugin* input) {
    createroom::Updater::update_game_mode_step(game_mode_, screen_width_, input);
}

// ============================================================================
// DRAW
// ============================================================================

void CreateRoomScreen::draw(engine::IGraphicsPlugin* graphics) {
    // Load textures on first draw call
    textures_.load(graphics);

    // Draw background
    draw_background(graphics);

    // Draw stepper indicator
    draw_stepper(graphics);

    // Draw content based on current step
    switch (current_step_) {
        case Step::ROOM_INFO:
            draw_room_info_step(graphics);
            break;
        case Step::MAP_SELECTION:
            draw_map_selection_step(graphics);
            break;
        case Step::DIFFICULTY:
            draw_difficulty_step(graphics);
            break;
        case Step::GAME_MODE:
            draw_game_mode_step(graphics);
            break;
    }

    // Draw navigation buttons
    draw_navigation_buttons(graphics);
}

void CreateRoomScreen::draw_background(engine::IGraphicsPlugin* graphics) {
    createroom::Renderer::draw_background(graphics, textures_, screen_width_, screen_height_);
}

void CreateRoomScreen::draw_stepper(engine::IGraphicsPlugin* graphics) {
    createroom::Renderer::draw_stepper(
        graphics,
        screen_width_,
        static_cast<int>(current_step_),
        get_step_title(),
        TOTAL_STEPS
    );
}

void CreateRoomScreen::draw_room_info_step(engine::IGraphicsPlugin* graphics) {
    createroom::Drawer::draw_room_info_step(labels_, fields_, graphics);
}

void CreateRoomScreen::draw_map_selection_step(engine::IGraphicsPlugin* graphics) {
    createroom::Drawer::draw_map_selection_step(textures_, map_buttons_, map_id_, screen_width_, graphics);
}

void CreateRoomScreen::draw_difficulty_step(engine::IGraphicsPlugin* graphics) {
    createroom::Drawer::draw_difficulty_step(textures_, difficulty_, screen_width_, graphics);
}

void CreateRoomScreen::draw_game_mode_step(engine::IGraphicsPlugin* graphics) {
    createroom::Drawer::draw_game_mode_step(textures_, game_mode_, screen_width_, graphics);
}

void CreateRoomScreen::draw_navigation_buttons(engine::IGraphicsPlugin* graphics) {
    createroom::Drawer::draw_navigation_buttons(nav_buttons_, static_cast<int>(current_step_), TOTAL_STEPS, graphics, screen_width_, screen_height_, button_radius_, button_spacing_, button_y_offset_, edit_mode_);
}

}  // namespace rtype::client
