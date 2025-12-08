/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** Registry
*/

#ifndef REGISTRY_HPP_
#define REGISTRY_HPP_
#include "SparseSet.hpp"
#include "systems/ISystem.hpp"
#include "core/event/EventBus.hpp"
#include <unordered_map>
#include <any>
#include <typeindex>
#include <typeinfo>
#include <functional>
#include <memory>

class Registry {
    private:
        size_t next_entity_id = 0;
        std::unordered_map<std::type_index, std::any> components;
        std::vector<std::function<void (Registry&, Entity)>> to_kill;
        std::vector<std::unique_ptr<ISystem>> systems;
        core::EventBus eventBus_;
    public:
        Registry() = default;
        ~Registry() = default;

        core::EventBus& get_event_bus() {
            return eventBus_;
        }

        template <typename Component>
        SparseSet<Component>& register_component()
        {
            std::type_index index = std::type_index(typeid(Component));

            components[index] = SparseSet<Component>();
            to_kill.push_back([this](Registry& r, Entity e){
                r.remove_component<Component>(e);
            });

            return std::any_cast<SparseSet<Component>&>(components[index]);
        }

        template <typename System, typename... Args>
        void register_system(Args&&... args)
        {
            // Assure que T hérite bien de ISystem
            static_assert(std::is_base_of<ISystem, System>::value, "System must inherit from ISystem"); 
            
            auto system = std::make_unique<System>(std::forward<Args>(args)...);
            
            system->init(*this);
            systems.push_back(std::move(system));
        }

        template <typename Component>
        SparseSet<Component>& get_components()
        {
            std::type_index index = std::type_index(typeid(Component));

            return std::any_cast<SparseSet<Component>&>(components[index]);
        }

        template <typename Component>
        void add_component(Entity entity, Component&& component)
        {
            // -> Component (le type genre Postion)
            // -> component (la donné genre {x = 10, y = 10})
            // j'aurais pu mettre un auto a la place de SparseSet<Component>

            SparseSet<Component>& sparseset = get_components<Component>();
            
            sparseset.insert_at(entity, component);
        }

        template <typename Component>
        void remove_component(Entity entity)
        {
            SparseSet<Component>& sparseset = get_components<Component>();

            sparseset.erase(entity);
        }

        Entity spawn_entity()
        {
            return next_entity_id++;
        }

        void kill_entity(Entity entity)
        {
            for (auto& cleaner : to_kill)
                cleaner(*this, entity);
        }

        void run_systems(float dt)
        {
            for (auto& system : systems)
                system->update(*this, dt);
        }

};

#endif /* !REGISTRY_HPP_ */
