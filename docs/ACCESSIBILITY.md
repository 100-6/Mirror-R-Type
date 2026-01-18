# Accessibility System Documentation

## Overview

The Mirror-R-Type engine includes a built-in accessibility system designed to support players with color vision deficiencies (colorblindness). This feature is implemented as a graphical **Post-Processing** effect, ensuring that the correction is applied globally to the entire game (sprites, UI, particles, background) without requiring changes to individual assets or game logic.

## Supported Modes

The system currently supports the three major types of color blindness:

1.  **Protanopia** (Red-blind): Simulates/Corrects for missing red cones.
2.  **Deuteranopia** (Green-blind): Simulates/Corrects for missing green cones.
3.  **Tritanopia** (Blue-blind): Simulates/Corrects for missing blue cones.

## Technical Architecture

The implementation uses a **Render-to-Texture** approach combined with **GLSL Shaders**.

### 1. The Rendering Pipeline

Instead of drawing directly to the screen's backbuffer, the rendering pipeline is intercepted in the Graphics Plugin layer:

1.  **Initialization**: A `RenderTexture` (Framebuffer) is created matching the window dimensions.
2.  **Drawing Phase**:
    *   `IGraphicsPlugin::clear()` redirects rendering target to the `RenderTexture`.
    *   All standard draw calls (`draw_sprite`, `draw_rect`, etc.) occur on this texture.
3.  **Display Phase**:
    *   `IGraphicsPlugin::display()` binds the `RenderTexture` as the source.
    *   The **Colorblind Shader** is activated.
    *   The texture is drawn as a full-screen quad onto the actual screen.
    *   The shader modifies pixel colors in real-time during this pass.

### 2. Shader Logic

The shader utilizes a color matrix transformation approach. It takes the original RGB color and applies a transformation based on the selected mode.

**Shader Source (Simplified):**
```glsl
uniform int mode; // 0:None, 1:Protanopia, 2:Deuteranopia, 3:Tritanopia

void main() {
    vec4 color = texture(texture0, fragTexCoord);

    if (mode == 1) { // Protanopia
        // Mix Red and Green channels to simulate red deficiency
        float r = 0.567 * color.r + 0.433 * color.g;
        float g = 0.558 * color.r + 0.442 * color.g;
        float b = 0.242 * color.g + 0.758 * color.b;
        color.rgb = vec3(r, g, b);
    }
    // ... logic for other modes
}
```

## Integration

### Engine Core
*   **`CommonTypes.hpp`**: Defines the `enum class ColorBlindMode`.
*   **`IGraphicsPlugin.hpp`**: Exposes the interface `virtual void set_colorblind_mode(ColorBlindMode mode) = 0;`.

### Plugins
Both major graphics backends implement this interface:
*   **Raylib (`RaylibGraphicsPlugin`)**: Uses `LoadShaderFromMemory`, `BeginTextureMode`, and `BeginShaderMode`.

### Client UI
The setting is managed in `SettingsScreen.cpp` (Display Tab). It toggles the mode, which immediately calls the graphics plugin to update the shader uniform.

## How to Add New Modes

1.  **Define**: Add the new mode to `engine::ColorBlindMode` in `CommonTypes.hpp`.
2.  **Update UI**: Add the case to `SettingsScreen::update_colorblind_label()`.
3.  **Update Shaders**:
    *   Modify `RAYLIB_COLORBLIND_SHADER` in `RaylibGraphicsPlugin.cpp`.
    *   Add the mathematical logic for the new color transformation.

## Troubleshooting

*   **No Effect**: Ensure the correct Graphics Plugin is loaded. Check console logs for "Shader loaded successfully".
*   **Performance**: The shader is lightweight (simple vector multiplication), but on extremely low-end hardware, disabling the mode (Mode 0) skips the shader pass entirely.
