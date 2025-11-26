/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** test_registry 
*/

#include <gtest/gtest.h>
#include "ecs/Registry.hpp"
#include <string>

// Test Components
struct Position {
    float x, y;
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

struct Velocity {
    float x, y;
    bool operator==(const Velocity& other) const {
        return x == other.x && y == other.y;
    }
};

struct Name {
    std::string name;
    bool operator==(const Name& other) const {
        return name == other.name;
    }
};

struct Health {
    int hp;
    bool operator==(const Health& other) const {
        return hp == other.hp;
    }
};

// ============================================================================
// REGISTRY TESTS
// ============================================================================

class RegistryTest : public ::testing::Test {
protected:
    Registry registry;

    void SetUp() override {
        registry.register_component<Position>();
        registry.register_component<Velocity>();
        registry.register_component<Name>();
        registry.register_component<Health>();
    }
};

// -----------------------------------------------
// TEST SUITE 1: Entity Spawning
// -----------------------------------------------

TEST_F(RegistryTest, SpawnEntity_ReturnsIncrementingIDs) {
    Entity e1 = registry.spawn_entity();
    Entity e2 = registry.spawn_entity();
    Entity e3 = registry.spawn_entity();

    EXPECT_EQ(e1, 0);
    EXPECT_EQ(e2, 1);
    EXPECT_EQ(e3, 2);
}

TEST_F(RegistryTest, SpawnEntity_MultipleEntities) {
    std::vector<Entity> entities;
    for (int i = 0; i < 100; i++) {
        entities.push_back(registry.spawn_entity());
    }

    for (size_t i = 0; i < entities.size(); i++) {
        EXPECT_EQ(entities[i], i);
    }
}

// -----------------------------------------------
// TEST SUITE 2: Component Registration
// -----------------------------------------------

TEST_F(RegistryTest, RegisterComponent_CanAccessComponents) {
    auto& positions = registry.get_components<Position>();
    EXPECT_NO_THROW(positions);
}

TEST_F(RegistryTest, RegisterComponent_MultipleDifferentComponents) {
    EXPECT_NO_THROW(registry.get_components<Position>());
    EXPECT_NO_THROW(registry.get_components<Velocity>());
    EXPECT_NO_THROW(registry.get_components<Name>());
    EXPECT_NO_THROW(registry.get_components<Health>());
}

// -----------------------------------------------
// TEST SUITE 3: Adding Components
// -----------------------------------------------

TEST_F(RegistryTest, AddComponent_SingleComponent) {
    Entity entity = registry.spawn_entity();
    registry.add_component<Position>(entity, Position{10.0f, 20.0f});

    auto& positions = registry.get_components<Position>();
    EXPECT_EQ(positions[entity].x, 10.0f);
    EXPECT_EQ(positions[entity].y, 20.0f);
}

TEST_F(RegistryTest, AddComponent_MultipleComponentsToSameEntity) {
    Entity entity = registry.spawn_entity();

    registry.add_component<Position>(entity, Position{5.0f, 15.0f});
    registry.add_component<Velocity>(entity, Velocity{1.0f, 2.0f});
    registry.add_component<Name>(entity, Name{"Player"});

    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& names = registry.get_components<Name>();

    EXPECT_EQ(positions[entity].x, 5.0f);
    EXPECT_EQ(positions[entity].y, 15.0f);
    EXPECT_EQ(velocities[entity].x, 1.0f);
    EXPECT_EQ(velocities[entity].y, 2.0f);
    EXPECT_EQ(names[entity].name, "Player");
}

TEST_F(RegistryTest, AddComponent_MultipleDifferentEntities) {
    Entity e1 = registry.spawn_entity();
    Entity e2 = registry.spawn_entity();
    Entity e3 = registry.spawn_entity();

    registry.add_component<Position>(e1, Position{10.0f, 10.0f});
    registry.add_component<Position>(e2, Position{20.0f, 20.0f});
    registry.add_component<Position>(e3, Position{30.0f, 30.0f});

    auto& positions = registry.get_components<Position>();

    EXPECT_EQ(positions[e1].x, 10.0f);
    EXPECT_EQ(positions[e2].x, 20.0f);
    EXPECT_EQ(positions[e3].x, 30.0f);
}

TEST_F(RegistryTest, AddComponent_NonSequentialEntityIDs) {
    Entity e5 = 5;
    Entity e10 = 10;
    Entity e100 = 100;

    registry.add_component<Position>(e5, Position{5.0f, 5.0f});
    registry.add_component<Position>(e10, Position{10.0f, 10.0f});
    registry.add_component<Position>(e100, Position{100.0f, 100.0f});

    auto& positions = registry.get_components<Position>();

    EXPECT_EQ(positions[e5].x, 5.0f);
    EXPECT_EQ(positions[e10].x, 10.0f);
    EXPECT_EQ(positions[e100].x, 100.0f);
}

// -----------------------------------------------
// TEST SUITE 4: Removing Components
// -----------------------------------------------

TEST_F(RegistryTest, RemoveComponent_RemovesSpecificComponent) {
    Entity entity = registry.spawn_entity();

    registry.add_component<Position>(entity, Position{10.0f, 20.0f});
    registry.add_component<Velocity>(entity, Velocity{1.0f, 2.0f});

    registry.remove_component<Position>(entity);

    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();

    EXPECT_THROW(positions[entity], std::bad_optional_access);
    EXPECT_NO_THROW(velocities[entity]);
}

TEST_F(RegistryTest, RemoveComponent_MultipleRemovals) {
    Entity e1 = registry.spawn_entity();
    Entity e2 = registry.spawn_entity();
    Entity e3 = registry.spawn_entity();

    registry.add_component<Position>(e1, Position{10.0f, 10.0f});
    registry.add_component<Position>(e2, Position{20.0f, 20.0f});
    registry.add_component<Position>(e3, Position{30.0f, 30.0f});

    registry.remove_component<Position>(e2);

    auto& positions = registry.get_components<Position>();

    EXPECT_NO_THROW(positions[e1]);
    EXPECT_THROW(positions[e2], std::bad_optional_access);
    EXPECT_NO_THROW(positions[e3]);
}

// -----------------------------------------------
// TEST SUITE 5: Killing Entities
// -----------------------------------------------

TEST_F(RegistryTest, KillEntity_RemovesAllComponents) {
    Entity entity = registry.spawn_entity();

    registry.add_component<Position>(entity, Position{10.0f, 20.0f});
    registry.add_component<Velocity>(entity, Velocity{1.0f, 2.0f});
    registry.add_component<Name>(entity, Name{"Enemy"});

    registry.kill_entity(entity);

    auto& positions = registry.get_components<Position>();
    auto& velocities = registry.get_components<Velocity>();
    auto& names = registry.get_components<Name>();

    EXPECT_THROW(positions[entity], std::bad_optional_access);
    EXPECT_THROW(velocities[entity], std::bad_optional_access);
    EXPECT_THROW(names[entity], std::bad_optional_access);
}

TEST_F(RegistryTest, KillEntity_DoesNotAffectOtherEntities) {
    Entity e1 = registry.spawn_entity();
    Entity e2 = registry.spawn_entity();
    Entity e3 = registry.spawn_entity();

    registry.add_component<Position>(e1, Position{10.0f, 10.0f});
    registry.add_component<Position>(e2, Position{20.0f, 20.0f});
    registry.add_component<Position>(e3, Position{30.0f, 30.0f});

    registry.kill_entity(e2);

    auto& positions = registry.get_components<Position>();

    EXPECT_NO_THROW(positions[e1]);
    EXPECT_THROW(positions[e2], std::bad_optional_access);
    EXPECT_NO_THROW(positions[e3]);

    EXPECT_EQ(positions[e1].x, 10.0f);
    EXPECT_EQ(positions[e3].x, 30.0f);
}

TEST_F(RegistryTest, KillEntity_MultipleEntities) {
    Entity e1 = registry.spawn_entity();
    Entity e2 = registry.spawn_entity();
    Entity e3 = registry.spawn_entity();

    registry.add_component<Position>(e1, Position{10.0f, 10.0f});
    registry.add_component<Position>(e2, Position{20.0f, 20.0f});
    registry.add_component<Position>(e3, Position{30.0f, 30.0f});

    registry.kill_entity(e1);
    registry.kill_entity(e3);

    auto& positions = registry.get_components<Position>();

    EXPECT_THROW(positions[e1], std::bad_optional_access);
    EXPECT_NO_THROW(positions[e2]);
    EXPECT_THROW(positions[e3], std::bad_optional_access);
}

// -----------------------------------------------
// TEST SUITE 6: Complex Scenarios
// -----------------------------------------------

TEST_F(RegistryTest, ComplexScenario_GameSimulation) {
    // Create player
    Entity player = registry.spawn_entity();
    registry.add_component<Position>(player, Position{0.0f, 0.0f});
    registry.add_component<Velocity>(player, Velocity{5.0f, 5.0f});
    registry.add_component<Name>(player, Name{"Player"});
    registry.add_component<Health>(player, Health{100});

    // Create enemies
    Entity enemy1 = registry.spawn_entity();
    Entity enemy2 = registry.spawn_entity();

    registry.add_component<Position>(enemy1, Position{50.0f, 50.0f});
    registry.add_component<Name>(enemy1, Name{"Enemy1"});
    registry.add_component<Health>(enemy1, Health{50});

    registry.add_component<Position>(enemy2, Position{100.0f, 100.0f});
    registry.add_component<Name>(enemy2, Name{"Enemy2"});
    registry.add_component<Health>(enemy2, Health{30});

    // Kill enemy1
    registry.kill_entity(enemy1);

    auto& positions = registry.get_components<Position>();
    auto& names = registry.get_components<Name>();
    auto& healths = registry.get_components<Health>();

    // Verify player still exists
    EXPECT_NO_THROW(positions[player]);
    EXPECT_EQ(names[player].name, "Player");
    EXPECT_EQ(healths[player].hp, 100);

    // Verify enemy1 is dead
    EXPECT_THROW(positions[enemy1], std::bad_optional_access);
    EXPECT_THROW(names[enemy1], std::bad_optional_access);
    EXPECT_THROW(healths[enemy1], std::bad_optional_access);

    // Verify enemy2 still exists
    EXPECT_NO_THROW(positions[enemy2]);
    EXPECT_EQ(names[enemy2].name, "Enemy2");
    EXPECT_EQ(healths[enemy2].hp, 30);
}

TEST_F(RegistryTest, ComplexScenario_MassEntityCreationAndDeletion) {
    std::vector<Entity> entities;

    // Create 1000 entities
    for (int i = 0; i < 1000; i++) {
        Entity e = registry.spawn_entity();
        registry.add_component<Position>(e, Position{static_cast<float>(i), static_cast<float>(i)});
        entities.push_back(e);
    }

    // Kill every other entity
    for (size_t i = 0; i < entities.size(); i += 2) {
        registry.kill_entity(entities[i]);
    }

    auto& positions = registry.get_components<Position>();

    // Verify odd entities still exist and even ones are dead
    for (size_t i = 0; i < entities.size(); i++) {
        if (i % 2 == 0) {
            EXPECT_THROW(positions[entities[i]], std::bad_optional_access);
        } else {
            EXPECT_NO_THROW(positions[entities[i]]);
            EXPECT_EQ(positions[entities[i]].x, static_cast<float>(i));
        }
    }
}

// -----------------------------------------------
// TEST SUITE 7: Edge Cases
// -----------------------------------------------

TEST_F(RegistryTest, EdgeCase_RemoveNonExistentComponent) {
    Entity entity = registry.spawn_entity();

    // Should not crash when removing a component that was never added
    // With the updated SparseSet implementation, erase silently ignores non-existent elements
    EXPECT_NO_THROW(registry.remove_component<Position>(entity));
}

TEST_F(RegistryTest, EdgeCase_KillEntityWithNoComponents) {
    Entity entity = registry.spawn_entity();

    // Should not crash when killing an entity with no components
    EXPECT_NO_THROW(registry.kill_entity(entity));
}

TEST_F(RegistryTest, EdgeCase_AccessComponentOnDeadEntity) {
    Entity entity = registry.spawn_entity();
    registry.add_component<Position>(entity, Position{10.0f, 20.0f});

    registry.kill_entity(entity);

    auto& positions = registry.get_components<Position>();
    EXPECT_THROW(positions[entity], std::bad_optional_access);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
