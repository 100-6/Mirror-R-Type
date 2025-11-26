/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** ISystem
*/

#ifndef ISYSTEM_HPP_
#define ISYSTEM_HPP_

class ISystem {
    public:
        virtual ~ISystem() = default;

        virtual void update() = 0;
        virtual void init() = 0;
        virtual void shutdown() = 0;

    protected:
    private:
};

#endif /* !ISYSTEM_HPP_ */
