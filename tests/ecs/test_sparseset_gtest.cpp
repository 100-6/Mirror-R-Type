/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** test_sparseset_gtest
*/

#include <gtest/gtest.h>
#include "ecs/SparseSet.hpp"
#include <string>

// Test structures
struct TestData {
    int value;
    bool operator==(const TestData& other) const {
        return value == other.value;
    }
};

struct ComplexData {
    int x, y, z;
    std::string name;
    bool operator==(const ComplexData& other) const {
        return x == other.x && y == other.y && z == other.z && name == other.name;
    }
};

// ============================================================================
// SPARSESET TESTS
// ============================================================================

class SparseSetTest : public ::testing::Test {
protected:
    SparseSet<int> intSet;
    SparseSet<TestData> dataSet;
    SparseSet<ComplexData> complexSet;
};

// -----------------------------------------------
// TEST SUITE 1: Basic Insertion
// -----------------------------------------------

TEST_F(SparseSetTest, Insert_SingleElement) {
    intSet.insert_at(0, 42);

    EXPECT_EQ(intSet[0], 42);
}

TEST_F(SparseSetTest, Insert_MultipleSequentialElements) {
    intSet.insert_at(0, 10);
    intSet.insert_at(1, 20);
    intSet.insert_at(2, 30);

    EXPECT_EQ(intSet[0], 10);
    EXPECT_EQ(intSet[1], 20);
    EXPECT_EQ(intSet[2], 30);
}

TEST_F(SparseSetTest, Insert_NonSequentialIndices) {
    intSet.insert_at(5, 100);
    intSet.insert_at(10, 200);
    intSet.insert_at(100, 300);

    EXPECT_EQ(intSet[5], 100);
    EXPECT_EQ(intSet[10], 200);
    EXPECT_EQ(intSet[100], 300);
}

TEST_F(SparseSetTest, Insert_AtLargeIndex) {
    intSet.insert_at(1000, 42);

    EXPECT_EQ(intSet[1000], 42);
}

TEST_F(SparseSetTest, Insert_ComplexStructure) {
    ComplexData data{10, 20, 30, "Test"};
    complexSet.insert_at(0, data);

    EXPECT_EQ(complexSet[0].x, 10);
    EXPECT_EQ(complexSet[0].y, 20);
    EXPECT_EQ(complexSet[0].z, 30);
    EXPECT_EQ(complexSet[0].name, "Test");
}

// -----------------------------------------------
// TEST SUITE 2: Element Access
// -----------------------------------------------

TEST_F(SparseSetTest, Access_ExistingElement) {
    intSet.insert_at(5, 123);

    EXPECT_NO_THROW(intSet[5]);
    EXPECT_EQ(intSet[5], 123);
}

TEST_F(SparseSetTest, Access_NonExistentElement_ThrowsException) {
    intSet.insert_at(0, 10);
    intSet.insert_at(5, 50);  // Ensure sparse vector is resized to include index 1

    // Now accessing index 1 (which exists in sparse but has no value) should throw
    EXPECT_THROW(intSet[1], std::bad_optional_access);
}

TEST_F(SparseSetTest, Access_ModifyElement) {
    intSet.insert_at(0, 42);
    intSet[0] = 100;

    EXPECT_EQ(intSet[0], 100);
}

// -----------------------------------------------
// TEST SUITE 3: Deletion
// -----------------------------------------------

TEST_F(SparseSetTest, Erase_SingleElement) {
    intSet.insert_at(0, 42);
    intSet.erase(0);

    EXPECT_THROW(intSet[0], std::bad_optional_access);
}

TEST_F(SparseSetTest, Erase_FirstOfMultiple) {
    intSet.insert_at(0, 10);
    intSet.insert_at(1, 20);
    intSet.insert_at(2, 30);

    intSet.erase(0);

    EXPECT_THROW(intSet[0], std::bad_optional_access);
    EXPECT_EQ(intSet[1], 20);
    EXPECT_EQ(intSet[2], 30);
}

TEST_F(SparseSetTest, Erase_MiddleOfMultiple) {
    intSet.insert_at(0, 10);
    intSet.insert_at(1, 20);
    intSet.insert_at(2, 30);

    intSet.erase(1);

    EXPECT_EQ(intSet[0], 10);
    EXPECT_THROW(intSet[1], std::bad_optional_access);
    EXPECT_EQ(intSet[2], 30);
}

TEST_F(SparseSetTest, Erase_LastOfMultiple) {
    intSet.insert_at(0, 10);
    intSet.insert_at(1, 20);
    intSet.insert_at(2, 30);

    intSet.erase(2);

    EXPECT_EQ(intSet[0], 10);
    EXPECT_EQ(intSet[1], 20);
    EXPECT_THROW(intSet[2], std::bad_optional_access);
}

TEST_F(SparseSetTest, Erase_NonSequentialElements) {
    intSet.insert_at(0, 100);
    intSet.insert_at(5, 200);
    intSet.insert_at(10, 300);

    intSet.erase(5);

    EXPECT_EQ(intSet[0], 100);
    EXPECT_THROW(intSet[5], std::bad_optional_access);
    EXPECT_EQ(intSet[10], 300);
}

TEST_F(SparseSetTest, Erase_AllElements) {
    intSet.insert_at(0, 10);
    intSet.insert_at(1, 20);
    intSet.insert_at(2, 30);

    intSet.erase(0);
    intSet.erase(1);
    intSet.erase(2);

    EXPECT_THROW(intSet[0], std::bad_optional_access);
    EXPECT_THROW(intSet[1], std::bad_optional_access);
    EXPECT_THROW(intSet[2], std::bad_optional_access);
}

// -----------------------------------------------
// TEST SUITE 4: Swap and Remove Mechanism
// -----------------------------------------------

TEST_F(SparseSetTest, Erase_VerifySwapBehavior) {
    // Insert elements
    intSet.insert_at(0, 111);
    intSet.insert_at(3, 1);
    intSet.insert_at(4, 2);
    intSet.insert_at(5, 3);

    // Erase element at index 4
    // This should swap the last element (index 5) into position 4
    intSet.erase(4);

    // Element 5 should still be accessible
    EXPECT_EQ(intSet[5], 3);

    // Element 4 should be gone
    EXPECT_THROW(intSet[4], std::bad_optional_access);

    // Other elements should be unaffected
    EXPECT_EQ(intSet[0], 111);
    EXPECT_EQ(intSet[3], 1);
}

TEST_F(SparseSetTest, Erase_MultipleSwaps) {
    intSet.insert_at(0, 10);
    intSet.insert_at(1, 20);
    intSet.insert_at(2, 30);
    intSet.insert_at(3, 40);
    intSet.insert_at(4, 50);

    // Erase middle element
    intSet.erase(2);

    // All remaining elements should be accessible
    EXPECT_EQ(intSet[0], 10);
    EXPECT_EQ(intSet[1], 20);
    EXPECT_THROW(intSet[2], std::bad_optional_access);
    EXPECT_EQ(intSet[3], 40);
    EXPECT_EQ(intSet[4], 50);

    // Erase another
    intSet.erase(1);

    EXPECT_EQ(intSet[0], 10);
    EXPECT_THROW(intSet[1], std::bad_optional_access);
    EXPECT_THROW(intSet[2], std::bad_optional_access);
    EXPECT_EQ(intSet[3], 40);
    EXPECT_EQ(intSet[4], 50);
}

// -----------------------------------------------
// TEST SUITE 5: Complex Data Types
// -----------------------------------------------

TEST_F(SparseSetTest, ComplexData_InsertAndAccess) {
    ComplexData d1{1, 2, 3, "First"};
    ComplexData d2{4, 5, 6, "Second"};
    ComplexData d3{7, 8, 9, "Third"};

    complexSet.insert_at(0, d1);
    complexSet.insert_at(5, d2);
    complexSet.insert_at(10, d3);

    EXPECT_EQ(complexSet[0], d1);
    EXPECT_EQ(complexSet[5], d2);
    EXPECT_EQ(complexSet[10], d3);
}

TEST_F(SparseSetTest, ComplexData_EraseAndVerify) {
    ComplexData d1{1, 2, 3, "First"};
    ComplexData d2{4, 5, 6, "Second"};
    ComplexData d3{7, 8, 9, "Third"};

    complexSet.insert_at(0, d1);
    complexSet.insert_at(5, d2);
    complexSet.insert_at(10, d3);

    complexSet.erase(5);

    EXPECT_EQ(complexSet[0], d1);
    EXPECT_THROW(complexSet[5], std::bad_optional_access);
    EXPECT_EQ(complexSet[10], d3);
}

// -----------------------------------------------
// TEST SUITE 6: Stress Tests
// -----------------------------------------------

TEST_F(SparseSetTest, Stress_InsertMany) {
    for (int i = 0; i < 1000; i++) {
        intSet.insert_at(i, i * 10);
    }

    for (int i = 0; i < 1000; i++) {
        EXPECT_EQ(intSet[i], i * 10);
    }
}

TEST_F(SparseSetTest, Stress_InsertAndEraseMany) {
    // Insert 1000 elements
    for (int i = 0; i < 1000; i++) {
        intSet.insert_at(i, i * 10);
    }

    // Erase every other element
    for (int i = 0; i < 1000; i += 2) {
        intSet.erase(i);
    }

    // Verify odd elements still exist
    for (int i = 1; i < 1000; i += 2) {
        EXPECT_EQ(intSet[i], i * 10);
    }

    // Verify even elements are gone
    for (int i = 0; i < 1000; i += 2) {
        EXPECT_THROW(intSet[i], std::bad_optional_access);
    }
}

TEST_F(SparseSetTest, Stress_SparseInsertions) {
    // Insert at widely spaced indices
    for (int i = 0; i < 100; i++) {
        intSet.insert_at(i * 100, i);
    }

    // Verify all insertions
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(intSet[i * 100], i);
    }
}

// -----------------------------------------------
// TEST SUITE 7: Edge Cases
// -----------------------------------------------

TEST_F(SparseSetTest, EdgeCase_InsertAtIndexZero) {
    intSet.insert_at(0, 999);
    EXPECT_EQ(intSet[0], 999);
}

TEST_F(SparseSetTest, EdgeCase_AccessAfterErase) {
    intSet.insert_at(0, 42);
    intSet.erase(0);

    EXPECT_THROW(intSet[0], std::bad_optional_access);
}

TEST_F(SparseSetTest, EdgeCase_ReinsertAfterErase) {
    intSet.insert_at(5, 100);
    intSet.erase(5);
    intSet.insert_at(5, 200);

    EXPECT_EQ(intSet[5], 200);
}

TEST_F(SparseSetTest, EdgeCase_EraseNonExistentElement) {
    intSet.insert_at(0, 10);
    intSet.insert_at(5, 50); // Ensure sparse[1] exists but is empty

    // Should not crash when trying to erase non-existent element
    EXPECT_NO_THROW(intSet.erase(1));

    // Verify other elements are still intact
    EXPECT_EQ(intSet[0], 10);
    EXPECT_EQ(intSet[5], 50);
}

TEST_F(SparseSetTest, EdgeCase_MultipleInsertsSameIndex) {
    intSet.insert_at(0, 10);
    intSet.insert_at(0, 20);  // Overwrite
    intSet.insert_at(0, 30);  // Overwrite again

    // Should have the last value
    EXPECT_EQ(intSet[0], 30);
}

// -----------------------------------------------
// TEST SUITE 8: Integration Scenarios
// -----------------------------------------------

TEST_F(SparseSetTest, Integration_AlternatingInsertErase) {
    for (int i = 0; i < 100; i++) {
        intSet.insert_at(i, i);
    }

    for (int i = 0; i < 50; i++) {
        intSet.erase(i);
    }

    for (int i = 0; i < 50; i++) {
        intSet.insert_at(i, i * 2);
    }

    // First half should have doubled values
    for (int i = 0; i < 50; i++) {
        EXPECT_EQ(intSet[i], i * 2);
    }

    // Second half should have original values
    for (int i = 50; i < 100; i++) {
        EXPECT_EQ(intSet[i], i);
    }
}

TEST_F(SparseSetTest, Integration_FragmentedMemory) {
    // Create a fragmented pattern
    intSet.insert_at(1, 10);
    intSet.insert_at(10, 20);
    intSet.insert_at(100, 30);
    intSet.insert_at(1000, 40);
    intSet.insert_at(10000, 50);

    // Erase some
    intSet.erase(10);
    intSet.erase(1000);

    // Verify remaining elements
    EXPECT_EQ(intSet[1], 10);
    EXPECT_THROW(intSet[10], std::bad_optional_access);
    EXPECT_EQ(intSet[100], 30);
    EXPECT_THROW(intSet[1000], std::bad_optional_access);
    EXPECT_EQ(intSet[10000], 50);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
