/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AudioSystem
*/

#ifndef AUDIOSYSTEM_HPP_
#define AUDIOSYSTEM_HPP_

#include "ISystem.hpp"
#include "ecs/Registry.hpp"
#include "plugin_manager/IAudioPlugin.hpp"
#include "core/event/EventBus.hpp"
#include "ecs/events/GameEvents.hpp"
#include "ecs/events/InputEvents.hpp"
#include <vector>

class AudioSystem : public ISystem {
    private:
        engine::IAudioPlugin& audio_plugin;
        
        // Sound handles
        engine::SoundHandle enemy_death_sound = engine::INVALID_HANDLE;
        engine::SoundHandle player_hit_sound = engine::INVALID_HANDLE;
        engine::SoundHandle powerup_sound = engine::INVALID_HANDLE;
        engine::SoundHandle shoot_sound = engine::INVALID_HANDLE; // Added shoot sound

        // Subscription IDs
        std::vector<core::EventBus::SubscriptionId> subscriptions;

    public:
        explicit AudioSystem(engine::IAudioPlugin& plugin);
        ~AudioSystem() override;

        void init(Registry& registry) override;
        void update(Registry& registry, float dt) override;
        void shutdown() override;
};

#endif /* !AUDIOSYSTEM_HPP_ */
