/*
** EPITECH PROJECT, 2025
** Mirror-R-Type
** File description:
** LevelSystem implementation
*/

#include "systems/LevelSystem.hpp"
#include "ecs/events/GameEvents.hpp"
#include <iostream>

namespace rtype {
namespace game {

LevelSystem::LevelSystem()
{
}

void LevelSystem::init(Registry& registry)
{
    // Nothing to initialize
}

void LevelSystem::shutdown()
{
    // Nothing to cleanup
}

void LevelSystem::update(Registry& registry, float dt)
{
    auto& level_controllers = registry.get_components<LevelController>();
    if (level_controllers.size() == 0) {
        return;  // No level controller, nothing to do
    }

    // Get the first (and only) level controller
    LevelController& lc = level_controllers.get_data_at(0);
    lc.state_timer += dt;

    // State machine
    switch (lc.state) {
        case LevelState::LEVEL_START:
            // Wait 0.5 seconds for level intro (reduced for faster gameplay)
            if (lc.state_timer >= 0.5f) {
                transition_to_waves(lc);
                on_waves_started(registry, lc);
            }
            break;

        case LevelState::WAVES:
            // Check if we passed the map end
            // 1 Chunk = 480px (30 tiles * 16px)
            {
               auto& scroll_states = registry.get_components<ScrollState>();
               float current_scroll = 0.0f;
               if (scroll_states.size() > 0) {
                   current_scroll = scroll_states.get_data_at(0).current_scroll;
               }
               
               float level_end_scroll = static_cast<float>(lc.total_chunks * 30 * 16);
               
               if (current_scroll >= level_end_scroll) {
                   std::cout << "[LevelSystem] Reached end of map (" << level_end_scroll << "px) - Forcing boss transition\n";
                   transition_to_boss_transition(lc);
                   on_boss_transition_started(registry, lc);
                   break;
               }
            }
            
            // Check if all phases complete and no enemies remain
            // Only end early if explicit trigger
            if (all_phases_complete(registry, lc)) {
                if (no_enemies_remaining(registry)) {
                    std::cout << "[LevelSystem] All waves complete and no enemies remaining - starting boss transition\n";
                    transition_to_boss_transition(lc);
                    on_boss_transition_started(registry, lc);
                } else {
                    // Waves complete but enemies still alive - wait for them to die
                    if (static_cast<int>(lc.state_timer) % 5 == 0) {  // Log every 5 seconds
                        auto& enemies = registry.get_components<Enemy>();
                        auto& boss_phases = registry.get_components<BossPhase>();
                        size_t enemy_count = 0;
                        for (size_t i = 0; i < enemies.size(); ++i) {
                            Entity entity = enemies.get_entity_at(i);
                            // Count only non-boss enemies (entities without BossPhase)
                            if (!boss_phases.has_entity(entity)) {
                                enemy_count++;
                            }
                        }
                        std::cout << "[LevelSystem] Waiting for " << enemy_count << " enemies to be defeated...\n";
                    }
                }
            }
            break;

        case LevelState::BOSS_TRANSITION:
            // Wait 3 seconds for boss warning
            if (lc.state_timer >= 3.0f) {
                transition_to_boss_fight(lc);
                on_boss_fight_started(registry, lc);
            }
            break;

        case LevelState::BOSS_FIGHT:
            // Check if boss is defeated
            if (boss_defeated(registry, lc)) {
                transition_to_level_complete(lc);
                on_level_completed(registry, lc);
            }
            break;

        case LevelState::LEVEL_COMPLETE:
            // Wait 5 seconds for victory animation
            if (lc.state_timer >= 5.0f) {
                load_next_level_or_final_victory(registry, lc);
            }
            break;

        case LevelState::GAME_OVER:
            // Do nothing, game is over
            break;
    }
}

// ============================================================================
// STATE TRANSITION CALLBACKS
// ============================================================================

void LevelSystem::on_waves_started(Registry& registry, LevelController& lc)
{
    std::cout << "[LevelSystem] Waves started for level " << static_cast<int>(lc.current_level) << "\n";
}

void LevelSystem::on_boss_transition_started(Registry& registry, LevelController& lc)
{
    std::cout << "[LevelSystem] Boss transition - WARNING: BOSS APPROACHING!\n";
    // Note: Boss warning messages can be added here if needed
}

void LevelSystem::on_boss_fight_started(Registry& registry, LevelController& lc)
{
    std::cout << "[LevelSystem] Boss fight started!\n";
    // Boss spawning is handled by GameSession::update() (see GameSession.cpp lines 374-447)

    // Emit scene change event for boss fight music
    registry.get_event_bus().publish(ecs::SceneChangeEvent{
        ecs::SceneChangeEvent::SceneType::BOSS_FIGHT,
        static_cast<int>(lc.current_level)
    });
}

void LevelSystem::on_level_completed(Registry& registry, LevelController& lc)
{
    std::cout << "[LevelSystem] Level " << static_cast<int>(lc.current_level) << " completed!\n";

    // Emit scene change event for victory music
    registry.get_event_bus().publish(ecs::SceneChangeEvent{
        ecs::SceneChangeEvent::SceneType::VICTORY,
        static_cast<int>(lc.current_level)
    });
}

// ============================================================================
// QUERY METHODS
// ============================================================================

bool LevelSystem::all_phases_complete(Registry& registry, const LevelController& lc)
{
    // Check if all waves have been triggered (updated by GameSession from WaveManager)
    return lc.all_waves_triggered;
}

bool LevelSystem::no_enemies_remaining(Registry& registry)
{
    auto& enemies = registry.get_components<Enemy>();
    auto& boss_phases = registry.get_components<BossPhase>();

    // Count non-boss enemies (entities without BossPhase component)
    size_t enemy_count = 0;
    for (size_t i = 0; i < enemies.size(); ++i) {
        Entity entity = enemies.get_entity_at(i);
        if (!boss_phases.has_entity(entity)) {
            enemy_count++;
        }
    }

    return enemy_count == 0;
}

bool LevelSystem::boss_defeated(Registry& registry, const LevelController& lc)
{
    if (!lc.boss_spawned) {
        return false;  // Boss hasn't spawned yet
    }

    if (lc.boss_entity == engine::INVALID_HANDLE) {
        return true;  // Boss entity was destroyed, boss is defeated
    }

    // Check if boss entity still exists in registry
    auto& enemies = registry.get_components<Enemy>();
    if (!enemies.has_entity(lc.boss_entity)) {
        return true;  // Boss entity no longer exists, boss is defeated
    }

    // Check boss health
    auto& healths = registry.get_components<Health>();
    if (healths.has_entity(lc.boss_entity)) {
        const Health& boss_health = healths[lc.boss_entity];
        if (boss_health.current <= 0) {
            return true;  // Boss health is zero, boss is defeated
        }
    }

    return false;  // Boss is still alive
}

// ============================================================================
// STATE TRANSITION HELPERS
// ============================================================================

void LevelSystem::transition_to_level_start(LevelController& lc)
{
    lc.state = LevelState::LEVEL_START;
    lc.state_timer = 0.0f;
    std::cout << "[LevelSystem] Transition: LEVEL_START\n";
}

void LevelSystem::transition_to_waves(LevelController& lc)
{
    lc.state = LevelState::WAVES;
    lc.state_timer = 0.0f;
    lc.current_phase_index = 0;
    lc.current_wave_in_phase = 0;
    std::cout << "[LevelSystem] Transition: WAVES\n";
}

void LevelSystem::transition_to_boss_transition(LevelController& lc)
{
    lc.state = LevelState::BOSS_TRANSITION;
    lc.state_timer = 0.0f;
    std::cout << "[LevelSystem] Transition: BOSS_TRANSITION\n";
}

void LevelSystem::transition_to_boss_fight(LevelController& lc)
{
    lc.state = LevelState::BOSS_FIGHT;
    lc.state_timer = 0.0f;
    std::cout << "[LevelSystem] Transition: BOSS_FIGHT\n";
}

void LevelSystem::transition_to_level_complete(LevelController& lc)
{
    lc.state = LevelState::LEVEL_COMPLETE;
    lc.state_timer = 0.0f;
    std::cout << "[LevelSystem] Transition: LEVEL_COMPLETE\n";
}

void LevelSystem::transition_to_game_over(LevelController& lc)
{
    lc.state = LevelState::GAME_OVER;
    lc.state_timer = 0.0f;
    std::cout << "[LevelSystem] Transition: GAME_OVER\n";
    // Note: SceneChangeEvent for GAME_OVER should be emitted by GameStateSystem
    // which has access to the registry's event bus
}

// ============================================================================
// LEVEL LOADING
// ============================================================================

void LevelSystem::load_next_level_or_final_victory(Registry& registry, LevelController& lc)
{
    if (lc.current_level >= lc.total_levels) {
        // Final victory! All levels completed
        std::cout << "[LevelSystem] FINAL VICTORY! All " << static_cast<int>(lc.total_levels) << " levels completed!\n";
        transition_to_game_over(lc);
        return;
    }

    // Load next level index
    uint8_t next_level = lc.current_level + 1;

    // SKIP LEVEL 2 (Nebula Station) - This is a special procedural map
    // not meant for the main campaign progression
    if (next_level == 2) {
        std::cout << "[LevelSystem] Skipping Level 2 (Special Procedural Map)...\n";
        next_level++;
    }

    std::cout << "[LevelSystem] Moving to level " << static_cast<int>(next_level) << "\n";

    // Update level controller
    lc.current_level = next_level;
    
    // Transition to level start
    transition_to_level_start(lc);

    // Note: Actual data loading (waves, maps) is handled by GameSession::update()
    // by detecting the current_level change.
}

void LevelSystem::clear_all_enemies_and_projectiles(Registry& registry)
{
    // Mark all enemies for destruction
    auto& enemies = registry.get_components<Enemy>();
    for (size_t i = 0; i < enemies.size(); ++i) {
        Entity entity = enemies.get_entity_at(i);
        auto& to_destroy = registry.get_components<ToDestroy>();
        if (!to_destroy.has_entity(entity)) {
            registry.add_component(entity, ToDestroy{});
        }
    }

    // Mark all projectiles for destruction
    auto& projectiles = registry.get_components<Projectile>();
    for (size_t i = 0; i < projectiles.size(); ++i) {
        Entity entity = projectiles.get_entity_at(i);
        auto& to_destroy = registry.get_components<ToDestroy>();
        if (!to_destroy.has_entity(entity)) {
            registry.add_component(entity, ToDestroy{});
        }
    }

    std::cout << "[LevelSystem] Cleared " << enemies.size() << " enemies and "
              << projectiles.size() << " projectiles\n";
}

} // namespace game
} // namespace rtype
