#pragma once

#include <cstddef>
#include <limits>

namespace engine {

/**
 * @brief Lightweight entity identifier used throughout the ECS
 * 
 * Entities are represented as simple integer IDs for performance.
 * IDs are unique, stable, and can be reused after deletion.
 */
using Entity = std::size_t;

/**
 * @brief Constant representing an invalid entity
 */
constexpr Entity INVALID_ENTITY = std::numeric_limits<std::size_t>::max();

/**
 * @brief Check if an entity ID is valid
 * 
 * @param entity The entity ID to check
 * @return true if the entity is valid, false otherwise
 */
inline bool is_valid(Entity entity) {
    return entity != INVALID_ENTITY;
}

}
