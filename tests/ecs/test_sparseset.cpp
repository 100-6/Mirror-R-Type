/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** test_sparseset
*/

#include <iostream>
#include "ecs/SparseSet.hpp"


void test_insert()
{
    SparseSet<int> mySet;

    std::cout << "Test d'insertion..." << std::endl;
    
    mySet.insert_at(5, 42);
    
    mySet.insert_at(0, 100);

    std::cout << "Test de lecture..." << std::endl;

    std::cout << "Entity 5: " << mySet[5] << " (Attendu: 42)" << std::endl;
    std::cout << "Entity 0: " << mySet[0] << " (Attendu: 100)" << std::endl;

}

void test_delete()
{
    SparseSet<int> mySet;

    mySet.insert_at(0, 111);
    mySet.insert_at(3, 1);
    mySet.insert_at(4, 2);
    mySet.insert_at(5, 3);

    mySet.erase(4);
    std::cout << "Entity 5 : " << mySet[5] << std::endl;
}

int main()
{
    SparseSet<int> set;

    set.insert_at(0, 10);
    set.insert_at(1, 20);
    set.insert_at(2, 30);

    std::cout << "Avant suppression, Entité 2 : " << set[2] << std::endl;

    std::cout << "Suppression de l'entité 0..." << std::endl;
    set.erase(0);

    std::cout << "Après suppression, Entité 2 : " << set[2] << std::endl;

    return 0;
}