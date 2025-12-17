#include "systems/ScrollingSystem.hpp"
#include <iostream>

void ScrollingSystem::init(Registry& registry)
{
    std::cout << "ScrollingSystem: Initialisation (scroll speed: "
              << scrollSpeed_ << " px/s)" << std::endl;
}

void ScrollingSystem::shutdown()
{
    std::cout << "ScrollingSystem: Arrêt" << std::endl;
}

void ScrollingSystem::update(Registry& registry, float dt)
{
    // Get component containers
    auto& positions = registry.get_components<Position>();
    auto& scrollables = registry.get_components<Scrollable>();
    auto& backgrounds = registry.get_components<Background>();

    // Calculate scroll offset for this frame
    float scrollOffset = scrollSpeed_ * dt;

    // Update all scrollable entities
    for (size_t i = 0; i < scrollables.size(); i++) {
        Entity entity = scrollables.get_entity_at(i);

        // Check if entity has Position component
        if (!positions.has_entity(entity)) {
            continue;
        }

        Position& pos = positions[entity];
        const Scrollable& scrollable = scrollables[entity];

        // Apply scrolling offset
        pos.x += scrollOffset * scrollable.speedMultiplier;

        // Check if this is a background entity that needs wrapping
        if (backgrounds.has_entity(entity) && scrollable.wrap) {
            // Wrap background for infinite scrolling
            if (scrollSpeed_ < 0) {
                // Scrolling left: wrap when goes off left edge
                if (pos.x <= -screenWidth_) {
                    pos.x += screenWidth_ * 2.0f;
                }
            } else if (scrollSpeed_ > 0) {
                // Scrolling right: wrap when goes off right edge
                if (pos.x >= screenWidth_) {
                    pos.x -= screenWidth_ * 2.0f;
                }
            }
        } else if (scrollable.destroyOffscreen) {
            // Destroy entities that scroll off screen (useful for obstacles/enemies)
            bool offscreen = false;

            if (scrollSpeed_ < 0 && pos.x < -200.0f) {
                // Scrolled off left edge with some margin
                offscreen = true;
            } else if (scrollSpeed_ > 0 && pos.x > screenWidth_ + 200.0f) {
                // Scrolled off right edge with some margin
                offscreen = true;
            }

            if (offscreen) {
                // Mark entity for destruction
                auto& toDestroys = registry.get_components<ToDestroy>();
                if (!toDestroys.has_entity(entity)) {
                    registry.add_component(entity, ToDestroy{});
                }
            }
        }
    }
}
