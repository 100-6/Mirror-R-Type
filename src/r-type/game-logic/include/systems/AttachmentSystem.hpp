/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AttachmentSystem
*/

#ifndef ATTACHMENT_SYSTEM_HPP_
#define ATTACHMENT_SYSTEM_HPP_

#include "ecs/systems/ISystem.hpp"

class AttachmentSystem : public ISystem {
public:
    void init(Registry& registry) override;
    void update(Registry& registry, float dt) override;
    void shutdown() override {};
};

#endif /* !ATTACHMENT_SYSTEM_HPP_ */
