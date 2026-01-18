# CreateRoom Module - Architecture Documentation

## 📁 Modular Structure

```
createroom/
├── CreateRoomConfig.hpp              # Configuration & constants
├── CreateRoomInput.hpp/cpp           # Input handling (circular clicks)
├── CreateRoomRenderer.hpp/cpp        # Low-level rendering (textures, shapes)
├── CreateRoomInitializer.hpp/cpp     # UI element initialization
├── CreateRoomUpdater.hpp/cpp         # Update logic
└── CreateRoomDrawer.hpp/cpp          # High-level drawing logic
```

## 🎯 Module Responsibilities

### **CreateRoomConfig.hpp**
- **Role**: Defines all layout and styling constants
- **Content**:
  - Dimensions (sizes, spacing)
  - Colors (background, borders, glow)
  - Asset paths (textures)
- **Usage**: Imported by all other modules

### **CreateRoomInput.hpp/cpp**
- **Role**: Handles all user input logic
- **Main Functions**:
  - `handle_difficulty_click()` - Circular click detection for difficulties
  - `handle_gamemode_click()` - Circular click detection for game modes
  - `is_point_in_circle()` - Circular collision utility
- **Dependencies**: IInputPlugin, protocol::Payloads

### **CreateRoomRenderer.hpp/cpp**
- **Role**: Low-level rendering of graphical elements
- **TexturePack Class**:
  - Loads and stores all textures
  - `load()` - Lazy texture loading
- **Renderer Class**:
  - `draw_background()` - Background with gradients
  - `draw_stepper()` - Step progress indicator
  - `draw_map_selection()` - Map preview rendering (rectangular)
  - `draw_difficulty_selection()` - Difficulty icon rendering (circular)
  - `draw_gamemode_selection()` - Game mode icon rendering (circular)
  - `draw_circular_image()` - Utility for drawing image + circular effects
- **Dependencies**: IGraphicsPlugin, protocol::Payloads

### **CreateRoomInitializer.hpp/cpp**
- **Role**: Initializes all UI elements for different steps
- **Main Functions**:
  - `init_room_info_step()` - Creates labels and text fields (room name, password)
  - `init_map_selection_step()` - Creates map selection buttons
  - `init_difficulty_step()` - No buttons (clickable circular images)
  - `init_game_mode_step()` - No buttons (clickable circular images)
  - `init_navigation_buttons()` - Creates Previous/Next/Create buttons
- **Dependencies**: UIButton, UILabel, UITextField

### **CreateRoomUpdater.hpp/cpp**
- **Role**: Handles update logic for each step
- **Main Functions**:
  - `update_room_info_step()` - Updates text fields
  - `update_map_selection_step()` - Updates map buttons (selection)
  - `update_difficulty_step()` - Delegates to InputHandler for circular clicks
  - `update_game_mode_step()` - Delegates to InputHandler for circular clicks
  - `update_navigation_buttons()` - Updates navigation buttons
  - `is_any_field_focused()` - Utility to check text field focus
- **Dependencies**: IGraphicsPlugin, IInputPlugin, CreateRoomInput

### **CreateRoomDrawer.hpp/cpp**
- **Role**: Orchestrates rendering for each step (high level)
- **Main Functions**:
  - `draw_room_info_step()` - Draws labels + text fields
  - `draw_map_selection_step()` - Draws map images + buttons (via Renderer)
  - `draw_difficulty_step()` - Draws circular difficulty images (via Renderer)
  - `draw_game_mode_step()` - Draws circular game mode images (via Renderer)
  - `draw_navigation_buttons()` - Draws buttons with dynamic text
- **Dependencies**: Renderer, UIButton, UILabel, UITextField

## 🔄 Data Flow

### Initialization
```
CreateRoomScreen::initialize()
  └─> Initializer::init_room_info_step()
  └─> Initializer::init_map_selection_step()
  └─> Initializer::init_difficulty_step()
  └─> Initializer::init_game_mode_step()
  └─> Initializer::init_navigation_buttons()
```

### Update
```
CreateRoomScreen::update()
  └─> Updater::is_any_field_focused()
  └─> Updater::update_*_step()
      └─> InputHandler::handle_*_click() (for difficulty & gamemode)
  └─> Updater::update_navigation_buttons()
```

### Draw
```
CreateRoomScreen::draw()
  └─> TexturePack::load() (lazy loading)
  └─> Renderer::draw_background()
  └─> Renderer::draw_stepper()
  └─> Drawer::draw_*_step()
      └─> Renderer::draw_*_selection() (for maps, difficulty, gamemode)
  └─> Drawer::draw_navigation_buttons()
```

## ✨ Architecture Benefits

1. **Separation of Concerns**: Each module has a unique and well-defined role
2. **Reusability**: Renderers can be reused in other screens
3. **Testability**: Each module can be tested independently
4. **Maintainability**: Changes are localized to specific files
5. **Readability**: Short and focused code (< 150 lines per file)
6. **Extensibility**: Easy to add new steps or features

## 📊 Comparison

### Before (monolithic)
- **1 file**: CreateRoomScreen.cpp (620+ lines)
- Everything mixed: init, update, draw, rendering
- Difficult to navigate and maintain

### After (modular)
- **7 files** well organized
- CreateRoomScreen.cpp: **275 lines** (56% reduction!)
- Each module: **50-150 lines**
- Clear, focused, easy-to-maintain code

## 🚀 How to Extend

### Adding a new step
1. Add enum in `CreateRoomScreen.hpp`
2. Create `init_new_step()` in `Initializer`
3. Create `update_new_step()` in `Updater`
4. Create `draw_new_step()` in `Drawer`
5. Update switch cases in `CreateRoomScreen.cpp`

### Adding a new UI element
1. Add constants in `Config.hpp`
2. Modify `Initializer` to create the element
3. Modify `Updater` for update logic
4. Modify `Drawer` for rendering
