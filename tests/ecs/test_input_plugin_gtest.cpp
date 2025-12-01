/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Test Input Plugin - GTest version
*/

#include <gtest/gtest.h>
#include "ecs/Registry.hpp"
#include "ecs/Components.hpp"
#include "plugins/input/raylib/RaylibInputPlugin.hpp"

class InputPluginTest : public ::testing::Test {
protected:
    Registry registry;
    RaylibInputPlugin inputPlugin;

    void SetUp() override {
        registry.register_component<Position>();
        registry.register_component<Input>();
        registry.register_component<Collider>();
        registry.register_component<Controllable>();
    }

    void TearDown() override {
        if (inputPlugin.is_initialized()) {
            inputPlugin.shutdown();
        }
    }
};

TEST_F(InputPluginTest, PluginInitializesSuccessfully) {
    EXPECT_TRUE(inputPlugin.initialize()) << "Le plugin devrait s'initialiser correctement";
    EXPECT_TRUE(inputPlugin.is_initialized()) << "Le plugin devrait être marqué comme initialisé";
}

TEST_F(InputPluginTest, PluginShutdownWithoutInit) {
    // Ne devrait pas crasher même si on n'a pas initialisé
    EXPECT_NO_THROW(inputPlugin.shutdown());
}

TEST_F(InputPluginTest, MultipleInitCallsAreSafe) {
    EXPECT_TRUE(inputPlugin.initialize());
    EXPECT_TRUE(inputPlugin.is_initialized());
    
    // Re-initialisation devrait retourner true sans problème
    EXPECT_TRUE(inputPlugin.initialize());
    EXPECT_TRUE(inputPlugin.is_initialized());
}

TEST_F(InputPluginTest, PlayerEntityHasInputComponent) {
    ASSERT_TRUE(inputPlugin.initialize());

    Entity player = registry.spawn_entity();
    registry.add_component<Position>(player, Position{100.0f, 100.0f});
    registry.add_component<Input>(player, Input{});
    registry.add_component<Controllable>(player, Controllable{});

    auto& inputs = registry.get_components<Input>();
    EXPECT_TRUE(inputs.has_entity(player)) << "Le joueur devrait avoir un composant Input";
    
    // Vérifier que l'input est initialisé à false
    const Input& playerInput = inputs[player];
    EXPECT_FALSE(playerInput.up);
    EXPECT_FALSE(playerInput.down);
    EXPECT_FALSE(playerInput.left);
    EXPECT_FALSE(playerInput.right);
    EXPECT_FALSE(playerInput.fire);
    EXPECT_FALSE(playerInput.special);
}

TEST_F(InputPluginTest, InputComponentCanBeModified) {
    ASSERT_TRUE(inputPlugin.initialize());

    Entity player = registry.spawn_entity();
    registry.add_component<Input>(player, Input{});

    auto& inputs = registry.get_components<Input>();
    auto& playerInput = inputs[player];
    
    // Simuler des inputs
    playerInput.up = true;
    playerInput.fire = true;

    EXPECT_TRUE(playerInput.up);
    EXPECT_TRUE(playerInput.fire);
    EXPECT_FALSE(playerInput.down);
    EXPECT_FALSE(playerInput.left);
}

TEST_F(InputPluginTest, UpdateDoesNotCrash) {
    ASSERT_TRUE(inputPlugin.initialize());
    
    Entity player = registry.spawn_entity();
    registry.add_component<Input>(player, Input{});
    
    // Update ne devrait pas crasher
    EXPECT_NO_THROW(inputPlugin.update());
}
