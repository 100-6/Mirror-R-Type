/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ISystem
*/

#ifndef ISYSTEM_HPP_
#define ISYSTEM_HPP_

// Forward declaration pour éviter la dépendance circulaire
class Registry;

class ISystem {
    public:
        virtual ~ISystem() = default;

        virtual void update(Registry& registry) = 0;
        virtual void init(Registry& registry) = 0;
        virtual void shutdown() = 0;

    protected:
    private:
};

#endif /* !ISYSTEM_HPP_ */
