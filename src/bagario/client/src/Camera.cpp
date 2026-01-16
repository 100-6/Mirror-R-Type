#include "Camera.hpp"
#include <algorithm>
#include <cmath>

namespace bagario::client {

Camera::Camera(float screen_width, float screen_height)
    : m_screen_width(screen_width)
    , m_screen_height(screen_height)
    , m_view_width(screen_width)
    , m_view_height(screen_height)
{
    // Start centered on map
    m_center_x = m_map_width / 2.0f;
    m_center_y = m_map_height / 2.0f;
    m_target_x = m_center_x;
    m_target_y = m_center_y;
}

void Camera::follow(float target_x, float target_y, float target_mass) {
    m_target_x = target_x;
    m_target_y = target_y;
    m_target_zoom = calculate_zoom_for_mass(target_mass);
}

void Camera::update(float dt) {
    // Smooth position interpolation
    float pos_factor = 1.0f - std::exp(-POSITION_LERP_SPEED * dt);
    m_center_x += (m_target_x - m_center_x) * pos_factor;
    m_center_y += (m_target_y - m_center_y) * pos_factor;

    // Smooth zoom interpolation
    float zoom_factor = 1.0f - std::exp(-ZOOM_LERP_SPEED * dt);
    m_zoom += (m_target_zoom - m_zoom) * zoom_factor;

    // Update view size based on zoom
    // Higher zoom = smaller view (zoomed in)
    // Lower zoom = larger view (zoomed out)
    m_view_width = m_screen_width / m_zoom;
    m_view_height = m_screen_height / m_zoom;

    // Clamp camera to map bounds
    clamp_to_map_bounds();
}

void Camera::set_map_bounds(float width, float height) {
    m_map_width = width;
    m_map_height = height;
}

engine::Vector2f Camera::screen_to_world(float screen_x, float screen_y) const {
    // Screen coordinates: (0,0) top-left, (width,height) bottom-right
    // Convert to normalized coordinates (-0.5 to 0.5)
    float norm_x = (screen_x / m_screen_width) - 0.5f;
    float norm_y = (screen_y / m_screen_height) - 0.5f;

    // Scale by view size and add camera center
    float world_x = m_center_x + norm_x * m_view_width;
    float world_y = m_center_y + norm_y * m_view_height;

    return {world_x, world_y};
}

engine::Vector2f Camera::world_to_screen(float world_x, float world_y) const {
    // Get position relative to camera center
    float rel_x = world_x - m_center_x;
    float rel_y = world_y - m_center_y;

    // Normalize by view size
    float norm_x = rel_x / m_view_width;
    float norm_y = rel_y / m_view_height;

    // Convert to screen coordinates
    float screen_x = (norm_x + 0.5f) * m_screen_width;
    float screen_y = (norm_y + 0.5f) * m_screen_height;

    return {screen_x, screen_y};
}

bool Camera::is_visible(float world_x, float world_y, float radius) const {
    // Check if circle intersects with view rectangle
    float half_width = m_view_width / 2.0f + radius;
    float half_height = m_view_height / 2.0f + radius;

    float dx = std::abs(world_x - m_center_x);
    float dy = std::abs(world_y - m_center_y);

    return (dx < half_width) && (dy < half_height);
}

void Camera::clamp_to_map_bounds() {
    float half_view_w = m_view_width / 2.0f;
    float half_view_h = m_view_height / 2.0f;

    // Clamp so view stays within map
    // If view is larger than map, center on map
    if (m_view_width >= m_map_width) {
        m_center_x = m_map_width / 2.0f;
    } else {
        m_center_x = std::clamp(m_center_x, half_view_w, m_map_width - half_view_w);
    }

    if (m_view_height >= m_map_height) {
        m_center_y = m_map_height / 2.0f;
    } else {
        m_center_y = std::clamp(m_center_y, half_view_h, m_map_height - half_view_h);
    }
}

float Camera::calculate_zoom_for_mass(float mass) const {
    // Zoom out as mass increases
    // Formula: zoom = BASE_ZOOM / sqrt(mass / MASS_ZOOM_FACTOR)
    // At MASS_ZOOM_FACTOR mass, zoom = 1.0
    // At 4x MASS_ZOOM_FACTOR mass, zoom = 0.5 (zoomed out)

    if (mass <= 0.0f) {
        return BASE_ZOOM;
    }

    float zoom = BASE_ZOOM / std::sqrt(mass / MASS_ZOOM_FACTOR);
    return std::clamp(zoom, MIN_ZOOM, MAX_ZOOM);
}

}  // namespace bagario::client
