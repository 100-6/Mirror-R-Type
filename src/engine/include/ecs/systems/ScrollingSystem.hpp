#ifndef SCROLLINGSYSTEM_HPP_
#define SCROLLINGSYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"
#include "ecs/Components.hpp"
#include "ecs/Registry.hpp"

/**
 * ScrollingSystem - Forces automatic scrolling of the map and props
 *
 * This system automatically moves entities tagged with the Scrollable component
 * to create a forced scrolling effect (e.g., for side-scrolling shooter games).
 *
 * Features:
 * - Configurable scroll speed
 * - Automatic wrapping for infinite scrolling backgrounds
 * - Works with any entity (backgrounds, walls, props, obstacles)
 * - Independent of velocity-based movement
 */
class ScrollingSystem : public ISystem {
    private:
        float scrollSpeed_;  // Pixels per second (negative = scroll left)
        float screenWidth_;  // Screen width for wrapping calculations

    public:
        /**
         * Constructor
         * @param speed Scrolling speed in pixels per second (negative for left scroll)
         * @param screenWidth Screen width for wrapping (default: 1920)
         */
        ScrollingSystem(float speed = -100.0f, float screenWidth = 1920.0f)
            : scrollSpeed_(speed), screenWidth_(screenWidth) {}

        virtual ~ScrollingSystem() = default;

        void init(Registry& registry) override;
        void shutdown() override;
        void update(Registry& registry, float dt) override;

        /**
         * Set the scrolling speed at runtime
         * @param speed New speed in pixels per second (negative = left)
         */
        void set_scroll_speed(float speed) { scrollSpeed_ = speed; }

        /**
         * Get current scrolling speed
         * @return Current speed in pixels per second
         */
        float get_scroll_speed() const { return scrollSpeed_; }
};

#endif // SCROLLINGSYSTEM_HPP_
