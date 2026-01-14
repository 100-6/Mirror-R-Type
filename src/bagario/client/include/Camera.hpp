#pragma once

#include "plugin_manager/CommonTypes.hpp"

namespace bagario::client {

/**
 * @brief Camera system for viewport management
 *
 * Handles:
 * - Smooth following of player cell
 * - Zoom based on player mass
 * - Screen-to-world coordinate conversion
 * - View culling
 */
class Camera {
public:
    /**
     * @brief Construct camera with screen dimensions
     * @param screen_width Window width in pixels
     * @param screen_height Window height in pixels
     */
    Camera(float screen_width, float screen_height);
    ~Camera() = default;

    /**
     * @brief Set target position to follow (usually player cell center)
     * @param target_x World X coordinate
     * @param target_y World Y coordinate
     * @param target_mass Total mass (affects zoom)
     */
    void follow(float target_x, float target_y, float target_mass);

    /**
     * @brief Update camera position with smooth interpolation
     * @param dt Delta time since last frame
     */
    void update(float dt);

    /**
     * @brief Set map bounds for camera clamping
     * @param width Map width
     * @param height Map height
     */
    void set_map_bounds(float width, float height);

    /**
     * @brief Convert screen coordinates to world coordinates
     * Used for mouse input handling
     * @param screen_x Screen X (0 = left)
     * @param screen_y Screen Y (0 = top)
     * @return World coordinates
     */
    engine::Vector2f screen_to_world(float screen_x, float screen_y) const;

    /**
     * @brief Convert world coordinates to screen coordinates
     * Used for rendering world-space UI elements
     * @param world_x World X
     * @param world_y World Y
     * @return Screen coordinates
     */
    engine::Vector2f world_to_screen(float world_x, float world_y) const;

    /**
     * @brief Check if a world-space circle is visible on screen
     * @param world_x Circle center X
     * @param world_y Circle center Y
     * @param radius Circle radius
     * @return true if any part of the circle is visible
     */
    bool is_visible(float world_x, float world_y, float radius) const;

    // ============== Getters ==============

    float get_center_x() const { return m_center_x; }
    float get_center_y() const { return m_center_y; }
    engine::Vector2f get_center() const { return {m_center_x, m_center_y}; }
    engine::Vector2f get_size() const { return {m_view_width, m_view_height}; }
    float get_zoom() const { return m_zoom; }

    float get_view_left() const { return m_center_x - m_view_width / 2.0f; }
    float get_view_right() const { return m_center_x + m_view_width / 2.0f; }
    float get_view_top() const { return m_center_y - m_view_height / 2.0f; }
    float get_view_bottom() const { return m_center_y + m_view_height / 2.0f; }

private:
    void clamp_to_map_bounds();
    float calculate_zoom_for_mass(float mass) const;

    // Screen dimensions
    float m_screen_width;
    float m_screen_height;

    // Map bounds
    float m_map_width = 5000.0f;
    float m_map_height = 5000.0f;

    // Current camera state
    float m_center_x = 0.0f;
    float m_center_y = 0.0f;
    float m_zoom = 1.0f;
    float m_view_width;
    float m_view_height;

    // Target for smooth follow
    float m_target_x = 0.0f;
    float m_target_y = 0.0f;
    float m_target_zoom = 1.0f;

    // Smoothing parameters
    static constexpr float POSITION_LERP_SPEED = 5.0f;
    static constexpr float ZOOM_LERP_SPEED = 3.0f;

    // Zoom parameters
    static constexpr float BASE_ZOOM = 1.0f;
    static constexpr float MIN_ZOOM = 0.3f;   // Maximum zoom out
    static constexpr float MAX_ZOOM = 1.5f;   // Maximum zoom in
    static constexpr float MASS_ZOOM_FACTOR = 50.0f;  // Mass at which zoom = 1.0
};

}  // namespace bagario::client
