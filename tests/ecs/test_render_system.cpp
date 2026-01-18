/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Test Render System - GTest version
*/

#include <gtest/gtest.h>
#include "ecs/Registry.hpp"
#include "components/GameComponents.hpp"
#include "ecs/systems/RenderSystem.hpp"
#include "plugin_manager/IGraphicsPlugin.hpp"
#include <vector>

// ============================================================================
// MOCK GRAPHICS PLUGIN
// ============================================================================

/**
 * @brief Mock Graphics Plugin for testing
 *
 * This mock plugin records all draw calls so we can verify
 * that the RenderSystem calls the plugin correctly.
 */
class MockGraphicsPlugin : public engine::IGraphicsPlugin {
private:
    bool initialized_ = false;
    std::vector<engine::Vector2f> drawn_positions_;
    std::vector<engine::TextureHandle> drawn_textures_;
    int draw_sprite_call_count_ = 0;

public:
    // IPlugin interface
    const char* get_name() const override { return "MockGraphicsPlugin"; }
    const char* get_version() const override { return "1.0.0"; }
    bool is_initialized() const override { return initialized_; }

    bool initialize() override {
        initialized_ = true;
        return true;
    }

    void shutdown() override {
        initialized_ = false;
        drawn_positions_.clear();
        drawn_textures_.clear();
        draw_sprite_call_count_ = 0;
    }

    // Window management (not used in tests)
    bool create_window(int, int, const char*) override { return true; }
    void close_window() override {}
    bool is_window_open() const override { return true; }
    void set_fullscreen(bool) override {}
    void set_vsync(bool) override {}

    // Rendering
    void clear(engine::Color) override {}
    void display() override {}

    // Drawing primitives
    void draw_sprite(const engine::Sprite& sprite, engine::Vector2f position) override {
        drawn_positions_.push_back(position);
        drawn_textures_.push_back(sprite.texture_handle);
        draw_sprite_call_count_++;
    }

    void draw_text(const std::string&, engine::Vector2f, engine::Color,
                   engine::FontHandle, int) override {}
    void draw_rectangle(const engine::Rectangle&, engine::Color) override {}
    void draw_rectangle_outline(const engine::Rectangle&, engine::Color, float) override {}
    void draw_circle(engine::Vector2f, float, engine::Color) override {}
    void draw_line(engine::Vector2f, engine::Vector2f, engine::Color, float) override {}
    float measure_text(const std::string&, int, engine::FontHandle) const override { return 0.0f; }

    // Resource loading (not used in tests)
    engine::TextureHandle load_texture(const std::string&) override { return 1; }
    engine::TextureHandle load_texture_from_memory(const uint8_t*, size_t) override { return 1; }
    void unload_texture(engine::TextureHandle) override {}
    engine::Vector2f get_texture_size(engine::TextureHandle) const override {
        return {0.0f, 0.0f};
    }
    engine::TextureHandle get_default_texture() const override { return 999; }
    engine::FontHandle load_font(const std::string&) override { return 1; }
    void unload_font(engine::FontHandle) override {}

    // Camera/View (not used in tests)
    void set_view(engine::Vector2f, engine::Vector2f) override {}
    void reset_view() override {}
    void* get_window_handle() const override { return nullptr; }

    // Blend modes (not used in tests)
    void begin_blend_mode(int) override {}
    void end_blend_mode() override {}

    // Accessibility (not used in tests)
    void set_colorblind_mode(engine::ColorBlindMode) override {}

    // Test helpers
    int get_draw_call_count() const { return draw_sprite_call_count_; }
    const std::vector<engine::Vector2f>& get_drawn_positions() const {
        return drawn_positions_;
    }
    const std::vector<engine::TextureHandle>& get_drawn_textures() const {
        return drawn_textures_;
    }
    void reset_draw_calls() {
        drawn_positions_.clear();
        drawn_textures_.clear();
        draw_sprite_call_count_ = 0;
    }
};

// ============================================================================
// RENDER SYSTEM TESTS
// ============================================================================

class RenderSystemTest : public ::testing::Test {
protected:
    Registry registry;
    MockGraphicsPlugin mockPlugin;
    RenderSystem* renderSystem = nullptr;

    void SetUp() override {
        // Register all necessary components
        registry.register_component<Position>();
        registry.register_component<Sprite>();
        registry.register_component<Velocity>();

        // Initialize mock plugin
        mockPlugin.initialize();

        // Create render system with reference
        renderSystem = new RenderSystem(mockPlugin);
        renderSystem->init(registry);
    }

    void TearDown() override {
        if (renderSystem) {
            renderSystem->shutdown();
            delete renderSystem;
            renderSystem = nullptr;
        }
        mockPlugin.shutdown();
    }
};

// ============================================================================
// BASIC TESTS
// ============================================================================

TEST_F(RenderSystemTest, SystemInitializesSuccessfully) {
    EXPECT_NO_THROW({
        RenderSystem system(mockPlugin);
        system.init(registry);
        system.shutdown();
    }) << "RenderSystem should initialize without crashing";
}

TEST_F(RenderSystemTest, UpdateDoesNotCrashWithEmptyRegistry) {
    EXPECT_NO_THROW(renderSystem->update(registry, 0.016f))
        << "Update should not crash with empty registry";

    EXPECT_EQ(mockPlugin.get_draw_call_count(), 0)
        << "No draw calls should be made with empty registry";
}

// ============================================================================
// RENDERING TESTS
// ============================================================================

TEST_F(RenderSystemTest, EntityWithPositionAndSpriteIsRendered) {
    // Create entity with Position and Sprite
    Entity entity = registry.spawn_entity();
    registry.add_component<Position>(entity, Position{100.0f, 200.0f});
    registry.add_component<Sprite>(entity, Sprite{
        .texture = 42,
        .width = 32.0f,
        .height = 32.0f,
        .rotation = 0.0f,
        .tint = engine::Color::White
    });

    renderSystem->update(registry, 0.016f);

    // Verify draw call was made
    EXPECT_EQ(mockPlugin.get_draw_call_count(), 1)
        << "One sprite should be drawn";

    const auto& positions = mockPlugin.get_drawn_positions();
    ASSERT_EQ(positions.size(), 1);
    EXPECT_FLOAT_EQ(positions[0].x, 100.0f);
    EXPECT_FLOAT_EQ(positions[0].y, 200.0f);

    const auto& textures = mockPlugin.get_drawn_textures();
    ASSERT_EQ(textures.size(), 1);
    EXPECT_EQ(textures[0], 42);
}

TEST_F(RenderSystemTest, EntityWithoutSpriteIsNotRendered) {
    // Create entity with only Position (no Sprite)
    Entity entity = registry.spawn_entity();
    registry.add_component<Position>(entity, Position{100.0f, 200.0f});

    renderSystem->update(registry, 0.016f);

    EXPECT_EQ(mockPlugin.get_draw_call_count(), 0)
        << "Entity without Sprite should not be drawn";
}

TEST_F(RenderSystemTest, EntityWithoutPositionIsNotRendered) {
    // Create entity with only Sprite (no Position)
    Entity entity = registry.spawn_entity();
    registry.add_component<Sprite>(entity, Sprite{
        .texture = 42,
        .width = 32.0f,
        .height = 32.0f
    });

    renderSystem->update(registry, 0.016f);

    EXPECT_EQ(mockPlugin.get_draw_call_count(), 0)
        << "Entity without Position should not be drawn";
}

TEST_F(RenderSystemTest, EntityWithInvalidTextureIsNotRendered) {
    // Create entity with invalid texture handle
    Entity entity = registry.spawn_entity();
    registry.add_component<Position>(entity, Position{100.0f, 200.0f});
    registry.add_component<Sprite>(entity, Sprite{
        .texture = engine::INVALID_HANDLE,  // Invalid!
        .width = 32.0f,
        .height = 32.0f
    });

    renderSystem->update(registry, 0.016f);

    EXPECT_EQ(mockPlugin.get_draw_call_count(), 0)
        << "Entity with invalid texture should not be drawn";
}

// ============================================================================
// MULTIPLE ENTITIES TESTS
// ============================================================================

TEST_F(RenderSystemTest, MultipleEntitiesAreRendered) {
    // Create 3 entities with different positions
    Entity entity1 = registry.spawn_entity();
    registry.add_component<Position>(entity1, Position{10.0f, 20.0f});
    registry.add_component<Sprite>(entity1, Sprite{.texture = 1, .width = 32.0f, .height = 32.0f});

    Entity entity2 = registry.spawn_entity();
    registry.add_component<Position>(entity2, Position{100.0f, 200.0f});
    registry.add_component<Sprite>(entity2, Sprite{.texture = 2, .width = 64.0f, .height = 64.0f});

    Entity entity3 = registry.spawn_entity();
    registry.add_component<Position>(entity3, Position{300.0f, 400.0f});
    registry.add_component<Sprite>(entity3, Sprite{.texture = 3, .width = 16.0f, .height = 16.0f});

    renderSystem->update(registry, 0.016f);

    EXPECT_EQ(mockPlugin.get_draw_call_count(), 3)
        << "All 3 entities should be drawn";

    const auto& positions = mockPlugin.get_drawn_positions();
    ASSERT_EQ(positions.size(), 3);

    // Verify all positions were drawn
    EXPECT_FLOAT_EQ(positions[0].x, 10.0f);
    EXPECT_FLOAT_EQ(positions[0].y, 20.0f);
    EXPECT_FLOAT_EQ(positions[1].x, 100.0f);
    EXPECT_FLOAT_EQ(positions[1].y, 200.0f);
    EXPECT_FLOAT_EQ(positions[2].x, 300.0f);
    EXPECT_FLOAT_EQ(positions[2].y, 400.0f);

    const auto& textures = mockPlugin.get_drawn_textures();
    ASSERT_EQ(textures.size(), 3);
    EXPECT_EQ(textures[0], 1);
    EXPECT_EQ(textures[1], 2);
    EXPECT_EQ(textures[2], 3);
}

TEST_F(RenderSystemTest, MixedEntitiesSomeWithoutSprite) {
    // Entity 1: Has both Position and Sprite
    Entity entity1 = registry.spawn_entity();
    registry.add_component<Position>(entity1, Position{10.0f, 20.0f});
    registry.add_component<Sprite>(entity1, Sprite{.texture = 1, .width = 32.0f, .height = 32.0f});

    // Entity 2: Only Position (should not render)
    Entity entity2 = registry.spawn_entity();
    registry.add_component<Position>(entity2, Position{100.0f, 200.0f});

    // Entity 3: Has both Position and Sprite
    Entity entity3 = registry.spawn_entity();
    registry.add_component<Position>(entity3, Position{300.0f, 400.0f});
    registry.add_component<Sprite>(entity3, Sprite{.texture = 3, .width = 16.0f, .height = 16.0f});

    renderSystem->update(registry, 0.016f);

    EXPECT_EQ(mockPlugin.get_draw_call_count(), 2)
        << "Only 2 entities with both components should be drawn";
}

// ============================================================================
// PERSISTENCE TESTS
// ============================================================================

TEST_F(RenderSystemTest, MultipleUpdatesRenderCorrectly) {
    Entity entity = registry.spawn_entity();
    registry.add_component<Position>(entity, Position{100.0f, 200.0f});
    registry.add_component<Sprite>(entity, Sprite{.texture = 42, .width = 32.0f, .height = 32.0f});

    // First update
    renderSystem->update(registry, 0.016f);
    EXPECT_EQ(mockPlugin.get_draw_call_count(), 1);

    mockPlugin.reset_draw_calls();

    // Second update (should render again)
    renderSystem->update(registry, 0.016f);
    EXPECT_EQ(mockPlugin.get_draw_call_count(), 1);
}

TEST_F(RenderSystemTest, PositionChangeReflectedInRender) {
    Entity entity = registry.spawn_entity();
    registry.add_component<Position>(entity, Position{100.0f, 200.0f});
    registry.add_component<Sprite>(entity, Sprite{.texture = 42, .width = 32.0f, .height = 32.0f});

    // First render
    renderSystem->update(registry, 0.016f);

    auto positions1 = mockPlugin.get_drawn_positions();
    ASSERT_EQ(positions1.size(), 1);
    EXPECT_FLOAT_EQ(positions1[0].x, 100.0f);
    EXPECT_FLOAT_EQ(positions1[0].y, 200.0f);

    // Change position
    auto& pos = registry.get_components<Position>()[entity];
    pos.x = 500.0f;
    pos.y = 600.0f;

    mockPlugin.reset_draw_calls();

    // Second render
    renderSystem->update(registry, 0.016f);

    auto positions2 = mockPlugin.get_drawn_positions();
    ASSERT_EQ(positions2.size(), 1);
    EXPECT_FLOAT_EQ(positions2[0].x, 500.0f);
    EXPECT_FLOAT_EQ(positions2[0].y, 600.0f);
}

// ============================================================================
// LAYER ORDERING TESTS
// ============================================================================

TEST_F(RenderSystemTest, EntitiesAreDrawnInLayerOrder) {
    // Create 3 entities with different layers
    Entity background = registry.spawn_entity();
    registry.add_component<Position>(background, Position{100.0f, 100.0f});
    registry.add_component<Sprite>(background, Sprite{
        .texture = 1,
        .width = 32.0f,
        .height = 32.0f,
        .rotation = 0.0f,
        .tint = engine::Color::White,
        .origin_x = 0.0f,
        .origin_y = 0.0f,
        .layer = 0  // Background
    });

    Entity foreground = registry.spawn_entity();
    registry.add_component<Position>(foreground, Position{200.0f, 200.0f});
    registry.add_component<Sprite>(foreground, Sprite{
        .texture = 2,
        .width = 32.0f,
        .height = 32.0f,
        .rotation = 0.0f,
        .tint = engine::Color::White,
        .origin_x = 0.0f,
        .origin_y = 0.0f,
        .layer = 10  // Foreground
    });

    Entity midground = registry.spawn_entity();
    registry.add_component<Position>(midground, Position{300.0f, 300.0f});
    registry.add_component<Sprite>(midground, Sprite{
        .texture = 3,
        .width = 32.0f,
        .height = 32.0f,
        .rotation = 0.0f,
        .tint = engine::Color::White,
        .origin_x = 0.0f,
        .origin_y = 0.0f,
        .layer = 5  // Middle
    });

    renderSystem->update(registry, 0.016f);

    EXPECT_EQ(mockPlugin.get_draw_call_count(), 3);

    const auto& textures = mockPlugin.get_drawn_textures();
    ASSERT_EQ(textures.size(), 3);

    // Verify draw order: layer 0, then 5, then 10
    EXPECT_EQ(textures[0], 1) << "Background (layer 0) should be drawn first";
    EXPECT_EQ(textures[1], 3) << "Midground (layer 5) should be drawn second";
    EXPECT_EQ(textures[2], 2) << "Foreground (layer 10) should be drawn last";
}

TEST_F(RenderSystemTest, SameLayerEntitiesMaintainOrder) {
    // Create 3 entities with the same layer
    Entity entity1 = registry.spawn_entity();
    registry.add_component<Position>(entity1, Position{100.0f, 100.0f});
    registry.add_component<Sprite>(entity1, Sprite{.texture = 1, .width = 32.0f, .height = 32.0f, .layer = 5});

    Entity entity2 = registry.spawn_entity();
    registry.add_component<Position>(entity2, Position{200.0f, 200.0f});
    registry.add_component<Sprite>(entity2, Sprite{.texture = 2, .width = 32.0f, .height = 32.0f, .layer = 5});

    Entity entity3 = registry.spawn_entity();
    registry.add_component<Position>(entity3, Position{300.0f, 300.0f});
    registry.add_component<Sprite>(entity3, Sprite{.texture = 3, .width = 32.0f, .height = 32.0f, .layer = 5});

    renderSystem->update(registry, 0.016f);

    EXPECT_EQ(mockPlugin.get_draw_call_count(), 3);

    // All should be drawn (order may vary within same layer, but all present)
    const auto& textures = mockPlugin.get_drawn_textures();
    ASSERT_EQ(textures.size(), 3);
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
