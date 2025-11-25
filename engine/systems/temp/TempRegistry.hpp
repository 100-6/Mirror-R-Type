/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Temporary Registry for System Testing (Simple ECS)
*/

#pragma once

#include "TempComponents.hpp"
#include <vector>
#include <unordered_map>
#include <optional>
#include <typeindex>
#include <memory>
#include <algorithm>

namespace rtype {

/**
 * @brief Simple component storage for a specific component type
 */
class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void entity_destroyed(EntityId entity) = 0;
};

template<typename T>
class ComponentArray : public IComponentArray {
public:
    void insert(EntityId entity, const T& component) {
        components_[entity] = component;
    }

    void remove(EntityId entity) {
        components_.erase(entity);
    }

    std::optional<T*> get(EntityId entity) {
        auto it = components_.find(entity);
        if (it != components_.end()) {
            return &it->second;
        }
        return std::nullopt;
    }

    std::optional<const T*> get(EntityId entity) const {
        auto it = components_.find(entity);
        if (it != components_.end()) {
            return &it->second;
        }
        return std::nullopt;
    }

    bool has(EntityId entity) const {
        return components_.find(entity) != components_.end();
    }

    void entity_destroyed(EntityId entity) override {
        remove(entity);
    }

    auto begin() { return components_.begin(); }
    auto end() { return components_.end(); }
    auto begin() const { return components_.begin(); }
    auto end() const { return components_.end(); }

private:
    std::unordered_map<EntityId, T> components_;
};

/**
 * @brief Simple ECS Registry for testing
 * Will be replaced by the final Registry implementation
 */
class TempRegistry {
public:
    TempRegistry() : next_entity_id_(1) {}

    /**
     * @brief Create a new entity
     * @return Entity ID
     */
    EntityId create_entity() {
        EntityId id = next_entity_id_++;
        entities_.push_back(id);
        return id;
    }

    /**
     * @brief Destroy an entity and all its components
     * @param entity Entity to destroy
     */
    void destroy_entity(EntityId entity) {
        // Remove from entity list
        entities_.erase(std::remove(entities_.begin(), entities_.end(), entity), entities_.end());
        
        // Remove from all component arrays
        for (auto& [type, array] : component_arrays_) {
            array->entity_destroyed(entity);
        }
    }

    /**
     * @brief Add a component to an entity
     * @tparam T Component type
     * @param entity Entity ID
     * @param component Component data
     */
    template<typename T>
    void add_component(EntityId entity, const T& component) {
        get_component_array<T>()->insert(entity, component);
    }

    /**
     * @brief Remove a component from an entity
     * @tparam T Component type
     * @param entity Entity ID
     */
    template<typename T>
    void remove_component(EntityId entity) {
        get_component_array<T>()->remove(entity);
    }

    /**
     * @brief Get a component from an entity
     * @tparam T Component type
     * @param entity Entity ID
     * @return Optional pointer to component
     */
    template<typename T>
    std::optional<T*> get_component(EntityId entity) {
        return get_component_array<T>()->get(entity);
    }

    /**
     * @brief Check if entity has a component
     * @tparam T Component type
     * @param entity Entity ID
     * @return true if component exists
     */
    template<typename T>
    bool has_component(EntityId entity) const {
        auto array = get_component_array<T>();
        return array && array->has(entity);
    }

    /**
     * @brief Get all entities with specific components
     * @tparam Components Component types to filter by
     * @return Vector of entity IDs
     */
    template<typename... Components>
    std::vector<EntityId> get_entities_with() const {
        std::vector<EntityId> result;
        
        for (EntityId entity : entities_) {
            if ((has_component<Components>(entity) && ...)) {
                result.push_back(entity);
            }
        }
        
        return result;
    }

    /**
     * @brief Get all entities
     * @return Vector of entity IDs
     */
    const std::vector<EntityId>& get_all_entities() const {
        return entities_;
    }

    /**
     * @brief Check if entity exists
     * @param entity Entity ID
     * @return true if entity exists
     */
    bool entity_exists(EntityId entity) const {
        return std::find(entities_.begin(), entities_.end(), entity) != entities_.end();
    }

private:
    template<typename T>
    ComponentArray<T>* get_component_array() {
        auto type = std::type_index(typeid(T));
        
        // Create array if it doesn't exist
        if (component_arrays_.find(type) == component_arrays_.end()) {
            component_arrays_[type] = std::make_shared<ComponentArray<T>>();
        }
        
        return static_cast<ComponentArray<T>*>(component_arrays_[type].get());
    }

    template<typename T>
    const ComponentArray<T>* get_component_array() const {
        auto type = std::type_index(typeid(T));
        auto it = component_arrays_.find(type);
        
        if (it != component_arrays_.end()) {
            return static_cast<const ComponentArray<T>*>(it->second.get());
        }
        
        return nullptr;
    }

    EntityId next_entity_id_;
    std::vector<EntityId> entities_;
    std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> component_arrays_;
};

} // namespace rtype
