/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** SparseSet
*/

#ifndef SPARSESET_HPP_
#define SPARSESET_HPP_
#include <vector>
#include <iostream>
#include <optional>

using Entity = size_t;

template <typename Component>
class SparseSet {
        private:
            std::vector<std::optional<size_t>> sparse;
            std::vector<Entity> dense;
            std::vector<Component> data;
        public:
            SparseSet() = default;
            ~SparseSet() = default;

            
            Component& operator[](Entity entity_id)
            {
                if (entity_id >= sparse.size() || !sparse[entity_id].has_value()) {
                    throw std::bad_optional_access();
                }
                size_t element = sparse[entity_id].value();
                return data[element];
            }


            // --- Méthodes d'API ECS pour l'itération ---

            // 1. Retourne la taille de l'itération (nombre de composants actifs)
            size_t size() const {
                return data.size();
            }

            // 2. Vérifie si l'entité possède ce composant
            bool has_entity(Entity entity_id) const {
                if (entity_id >= sparse.size()) {
                    return false;
                }
                return sparse[entity_id].has_value();
            }

            // 3. Obtient l'ID de l'entité à l'index d'itération (pour la boucle for)
            Entity get_entity_at(size_t index) const {
                if (index >= dense.size()) {
                    throw std::out_of_range("Index out of bounds in SparseSet::get_entity_at");
                }
                return dense[index];
            }

            // 4. Obtient la donnée du composant à l'index d'itération (pour la boucle for)
            Component& get_data_at(size_t index) {
                if (index >= data.size()) {
                    throw std::out_of_range("Index out of bounds in SparseSet::get_data_at");
                }
                return data[index];
            }
            
            // 5. Equivalent de l'opérateur [] mais en fonction (utilisé par SystemMouvement)
            Component& get_data_by_entity_id(Entity entity_id) {
                return (*this)[entity_id];
            }

            // Méthodes
            void erase(size_t index)
            {
                if (index >= sparse.size() || !sparse[index].has_value()) {return;}

                size_t delete_id = sparse[index].value();
                size_t last_entity_id = dense[dense.size() - 1];

                dense[delete_id] = last_entity_id;
                dense.pop_back();

                data[delete_id] = data[data.size() - 1];
                data.pop_back();

                sparse[last_entity_id] = delete_id;
                sparse[index].reset();
            }

            void insert_at(size_t index, const Component& component)
            {
                if (index >= sparse.size()) sparse.resize(index + 1);
                dense.push_back(index);
                data.push_back(component);
                sparse[index] = dense.size() - 1;
            }
};

#endif /* !SPARSESET_HPP_ */
