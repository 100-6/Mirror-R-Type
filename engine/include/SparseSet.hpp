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

template <typename Component>

class SparseSet {
        private:
            std::vector<std::optional<size_t>> sparse;
            std::vector<size_t> dense;
            std::vector<Component> data;
            size_t count;
        public:
            SparseSet() = default;
            ~SparseSet() = default;

            Component& operator[](size_t index)
            {
                size_t element = sparse[index].value();

                return data[element];
            }

            void erase(size_t index)
            {
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
        protected:
};

#endif /* !SPARSESET_HPP_ */
