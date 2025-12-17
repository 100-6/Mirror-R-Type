/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** AudioSystem
*/

#include "ecs/systems/AudioSystem.hpp"
#include <iostream>

AudioSystem::AudioSystem(engine::IAudioPlugin& plugin)
    : audio_plugin(plugin)
{
}

AudioSystem::~AudioSystem()
{
}

void AudioSystem::init(Registry& registry)
{
    std::cout << "AudioSystem: Initialisation" << std::endl;

    // Load sounds
    // Using existing assets as placeholders
    enemy_death_sound = audio_plugin.load_sound("assets/sounds/death.mp3");
    player_hit_sound = audio_plugin.load_sound("assets/sounds/android.mp3");
    powerup_sound = audio_plugin.load_sound("assets/sounds/rizz.mp3");
    shoot_sound = audio_plugin.load_sound("assets/sounds/shoot.mp3"); // Load shoot sound

    if (enemy_death_sound == engine::INVALID_HANDLE) {
        std::cerr << "AudioSystem: Failed to load enemy death sound" << std::endl;
    }
    if (player_hit_sound == engine::INVALID_HANDLE) {
        std::cerr << "AudioSystem: Failed to load player hit sound" << std::endl;
    }
    if (powerup_sound == engine::INVALID_HANDLE) {
        std::cerr << "AudioSystem: Failed to load powerup sound" << std::endl;
    }
    if (shoot_sound == engine::INVALID_HANDLE) {
        std::cerr << "AudioSystem: Failed to load shoot sound" << std::endl;
    }


    auto& eventBus = registry.get_event_bus();

    // Subscribe to EnemyKilledEvent
    subscriptions.push_back(eventBus.subscribe<ecs::EnemyKilledEvent>(
        [this](const ecs::EnemyKilledEvent&) {
            // Play sound for enemy death
            if (enemy_death_sound != engine::INVALID_HANDLE) {
                audio_plugin.play_sound(enemy_death_sound);
                std::cout << "AudioSystem: Playing enemy death sound" << std::endl;
            }
        }
    ));

    // Subscribe to PlayerHitEvent
    subscriptions.push_back(eventBus.subscribe<ecs::PlayerHitEvent>(
        [this](const ecs::PlayerHitEvent&) {
            if (player_hit_sound != engine::INVALID_HANDLE) {
                audio_plugin.play_sound(player_hit_sound);
                std::cout << "AudioSystem: Playing player hit sound" << std::endl;
            }
        }
    ));

    // Subscribe to PowerUpCollectedEvent
    subscriptions.push_back(eventBus.subscribe<ecs::PowerUpCollectedEvent>(
        [this](const ecs::PowerUpCollectedEvent&) {
            if (powerup_sound != engine::INVALID_HANDLE) {
                audio_plugin.play_sound(powerup_sound);
                std::cout << "AudioSystem: Playing powerup sound" << std::endl;
            }
        }
    ));

    // Subscribe to ShotFiredEvent
    subscriptions.push_back(eventBus.subscribe<ecs::ShotFiredEvent>(
        [this](const ecs::ShotFiredEvent&) {
            if (shoot_sound != engine::INVALID_HANDLE) {
                audio_plugin.play_sound(shoot_sound);
                std::cout << "AudioSystem: Playing shoot sound" << std::endl;
            }
        }
    ));
}

void AudioSystem::update(Registry& registry, float dt)
{
    (void)registry;
    (void)dt;
    // AudioSystem reacts to events, no per-frame update needed
}

void AudioSystem::shutdown()
{
    std::cout << "AudioSystem: Shutdown" << std::endl;

    // Unload sounds
    if (enemy_death_sound != engine::INVALID_HANDLE) {
        audio_plugin.unload_sound(enemy_death_sound);
    }
    if (player_hit_sound != engine::INVALID_HANDLE) {
        audio_plugin.unload_sound(player_hit_sound);
    }
    if (powerup_sound != engine::INVALID_HANDLE) {
        audio_plugin.unload_sound(powerup_sound);
    }
    if (shoot_sound != engine::INVALID_HANDLE) {
        audio_plugin.unload_sound(shoot_sound);
    }
    
    // Note: subscriptions are cleaned up by EventBus or we could store registry ref and unsubscribe here
    // But since Registry owns EventBus and Systems, EventBus will be destroyed too.
}
