#include "EntityManager.hpp"
#include "GameConfig.hpp"
#include "components/ShipComponents.hpp"
#include "ecs/events/GameEvents.hpp"
#include <iostream>
#include <algorithm>
#include <random>

namespace rtype::client {

namespace {
constexpr float PROJECTILE_DESPAWN_MARGIN = 250.0f;
constexpr uint8_t ENTITY_STALE_THRESHOLD = 6; // ~0.1s before removing missing entities
}

struct EntityDimensions {
    float width;
    float height;
};

static EntityDimensions get_collider_dimensions(protocol::EntityType type, uint8_t subtype = 0) {
    using namespace rtype::shared::config;

    // Map bonus types to powerup types
    if (type == protocol::EntityType::BONUS_HEALTH)
        type = protocol::EntityType::POWERUP_HEALTH;
    else if (type == protocol::EntityType::BONUS_SHIELD)
        type = protocol::EntityType::POWERUP_SHIELD;
    else if (type == protocol::EntityType::BONUS_SPEED)
        type = protocol::EntityType::POWERUP_SPEED;

    switch (type) {
        case protocol::EntityType::PLAYER: {
            // Extract ship type from subtype (which contains skin_id for players)
            // Note: subtype for players is encoded as (player_id << 4) | skin_id
            uint8_t skin_id = subtype & 0x0F;  // Extract low 4 bits
            auto hitbox = rtype::game::get_hitbox_dimensions_from_skin_id(skin_id);
            return {hitbox.width, hitbox.height};
        }
        case protocol::EntityType::ENEMY_BASIC:
            return {ENEMY_BASIC_WIDTH, ENEMY_BASIC_HEIGHT};
        case protocol::EntityType::ENEMY_FAST:
            return {ENEMY_FAST_WIDTH, ENEMY_FAST_HEIGHT};
        case protocol::EntityType::ENEMY_TANK:
            return {ENEMY_TANK_WIDTH, ENEMY_TANK_HEIGHT};
        case protocol::EntityType::ENEMY_BOSS:
            return {ENEMY_BOSS_WIDTH, ENEMY_BOSS_HEIGHT};
        case protocol::EntityType::PROJECTILE_PLAYER:
        case protocol::EntityType::PROJECTILE_ENEMY:
            return {PROJECTILE_WIDTH, PROJECTILE_HEIGHT};
        case protocol::EntityType::WALL:
            return {WALL_WIDTH, WALL_HEIGHT};
        case protocol::EntityType::POWERUP_HEALTH:
        case protocol::EntityType::POWERUP_SHIELD:
        case protocol::EntityType::POWERUP_SPEED:
        case protocol::EntityType::POWERUP_SCORE:
            return {BONUS_SIZE, BONUS_SIZE};
        default:
            return {ENEMY_BASIC_WIDTH, ENEMY_BASIC_HEIGHT};
    }
}

EntityManager::EntityManager(Registry& registry, TextureManager& textures,
                            int screen_width, int screen_height)
    : registry_(registry)
    , textures_(textures)
    , screen_width_(screen_width)
    , screen_height_(screen_height)
    , local_player_id_(0)
    , local_player_entity_id_(std::numeric_limits<uint32_t>::max()) {
}

Sprite EntityManager::build_sprite(protocol::EntityType type, bool is_local_player, uint8_t subtype) {
    Sprite sprite{};
    sprite.texture = textures_.get_enemy_9();
    auto dims = get_collider_dimensions(type, subtype);
    bool use_fixed_dimensions = false;

    // Initialize dimensions with collider size as fallback
    sprite.width = dims.width;
    sprite.height = dims.height;

    sprite.tint = engine::Color::White;
    sprite.layer = 5;

    switch (type) {
        case protocol::EntityType::PLAYER: {
            // Use skin_id from subtype to select the correct ship sprite
            // skin_id formula: skin_id = color * 5 + type (0-14 range)
            uint8_t skin_id = subtype % 15;  // Clamp to valid range
            ShipColor color = static_cast<ShipColor>(skin_id / 5);
            ShipType ship_type = static_cast<ShipType>(skin_id % 5);
            engine::Sprite ship_sprite = textures_.get_ship_manager().create_ship_sprite(color, ship_type, 4.0f);
            sprite.texture = ship_sprite.texture_handle;
            
            // Copier le source_rect pour le découpage de la spritesheet
            sprite.source_rect.x = ship_sprite.source_rect.x;
            sprite.source_rect.y = ship_sprite.source_rect.y;
            sprite.source_rect.width = ship_sprite.source_rect.width;
            sprite.source_rect.height = ship_sprite.source_rect.height;
            
            // Utiliser les dimensions du sprite retourné (déjà avec le scale appliqué)
            sprite.width = ship_sprite.size.x;
            sprite.height = ship_sprite.size.y;
            
            // Copier l'origine (centrée)
            sprite.origin_x = ship_sprite.origin.x;
            sprite.origin_y = ship_sprite.origin.y;
            
            sprite.layer = 10;
            sprite.tint = engine::Color::White;
            break;
        }
        case protocol::EntityType::ENEMY_FAST:
            // Unify all FAST enemies to use the same sprite (enemie13)
            // Except Kamikaze which is special
            if (subtype == 12) {
                 // Kamikaze
                 sprite.texture = textures_.get_kamikaze();
                 sprite.tint = engine::Color::White;
                 {
                    engine::Vector2f tex_size = textures_.get_texture_size(sprite.texture);
                    if (tex_size.x > 0) {
                        float target_width = 200.0f;
                        float scale = target_width / tex_size.x;
                        sprite.width = tex_size.x * scale;
                        sprite.height = tex_size.y * scale;
                        use_fixed_dimensions = true;
                    }
                 }
            } else {
                // All other FAST types (Default, Zigzag, Coward, Chaser) use enemie13
                sprite.texture = textures_.get_enemy_13();
                sprite.tint = engine::Color::White;
            }
            break;

        case protocol::EntityType::ENEMY_TANK:
            // Unify all TANK enemies to use the same sprite (enemie11)
            // Default, Patrol, Spiral
            sprite.texture = textures_.get_enemy_11();
            sprite.tint = engine::Color::White;
            break;
        case protocol::EntityType::ENEMY_BOSS:
            // Select boss sprite based on current map
            if (current_map_id_ == 3) {
                sprite.texture = textures_.get_boss_uranus();
            } else if (current_map_id_ == 4) {
                sprite.texture = textures_.get_boss_jupiter();
            } else {
                // Default to Mars boss (Map 1) or generic
                sprite.texture = textures_.get_boss_mars();
            }
            
            sprite.layer = 6;
            sprite.tint = engine::Color::White; // No tint, use original sprite colors
            break;
        case protocol::EntityType::WALL:
            sprite.texture = textures_.get_wall();
            sprite.layer = 2;
            sprite.tint = engine::Color{180, 180, 255, 255};
            break;
        case protocol::EntityType::PROJECTILE_PLAYER:
            sprite.texture = textures_.get_bullet_animation();
            sprite.layer = 20;
            sprite.tint = engine::Color::White;
            sprite.source_rect = {0.0f, 0.0f, 16.0f, 16.0f};
            sprite.width = shared::config::PROJECTILE_WIDTH;
            sprite.height = shared::config::PROJECTILE_HEIGHT;
            use_fixed_dimensions = true;
            break;
        case protocol::EntityType::PROJECTILE_ENEMY:
            sprite.texture = textures_.get_projectile();
            sprite.layer = 20;
            sprite.tint = engine::Color::White;  // Removed red tint
            sprite.source_rect = {16.0f, 0.0f, 16.0f, 16.0f};
            sprite.width = shared::config::PROJECTILE_WIDTH;
            sprite.height = shared::config::PROJECTILE_HEIGHT;
            use_fixed_dimensions = true;
            break;
        case protocol::EntityType::POWERUP_HEALTH:
        case protocol::EntityType::POWERUP_SHIELD:
        case protocol::EntityType::POWERUP_SPEED:
        case protocol::EntityType::POWERUP_SCORE:
            sprite.texture = textures_.get_projectile();
            sprite.layer = 5;
            sprite.source_rect = {0.0f, 0.0f, 16.0f, 16.0f};
            sprite.width = 80.0f;  // Grande taille pour les bonus droppés
            sprite.height = 80.0f;
            use_fixed_dimensions = true;
            // Couleur basée sur le subtype (qui contient le BonusType)
            // 0 = HEALTH (vert), 1 = SHIELD (violet), 2 = SPEED (bleu), 3 = BONUS_WEAPON (jaune)
            switch (subtype) {
                case 0: // HEALTH - Vert
                    sprite.tint = engine::Color::Green;
                    break;
                case 1: // SHIELD - Violet
                    sprite.tint = engine::Color::Purple;
                    break;
                case 2: // SPEED - Bleu
                    sprite.tint = engine::Color::SpeedBlue;
                    break;
                case 3: // BONUS_WEAPON - Jaune
                    sprite.tint = engine::Color::Yellow;
                    break;
                default:
                    sprite.tint = engine::Color{120, 255, 120, 255}; // Vert par défaut
                    break;
            }
            break;
        default:
            break;
    }

    // Apply colors for ENEMY_BASIC based on subtype (new enemy types)
    if (type == protocol::EntityType::ENEMY_BASIC) {
        switch (subtype) {
            case 0:
                sprite.texture = textures_.get_enemy_9();
                sprite.tint = engine::Color::White;
                break;
            case 10:
                sprite.texture = textures_.get_enemy_10();
                sprite.tint = engine::Color::White;
                break;
            case 13:
                sprite.texture = textures_.get_enemy_11();
                sprite.tint = engine::Color::White;
                break;
            case 14:
                sprite.texture = textures_.get_enemy_12();
                sprite.tint = engine::Color::White;
                break;
            case 16:
                sprite.texture = textures_.get_enemy_13();
                sprite.tint = engine::Color::White;
                break;
            case 18:
                // Re-use 9 for variety
                sprite.texture = textures_.get_enemy_9();
                sprite.tint = engine::Color::White;
                break;
            default:
                // Default fallback
                sprite.texture = textures_.get_enemy_10();
                sprite.tint = engine::Color::White;
                break;
        }

        // Apply scale fix if texture is large
        engine::Vector2f tex_size = textures_.get_texture_size(sprite.texture);
        if (tex_size.x > 0) {
           float target_width = dims.width; // Use collider width (approx 64-80)
           if (tex_size.x > target_width) {
               float scale = target_width / tex_size.x;
               sprite.width = tex_size.x * scale;
               sprite.height = tex_size.y * scale;
               use_fixed_dimensions = true;
           }
        }
    }

    // Adjust sprite dimensions to preserve aspect ratio while fitting in collider box
    // SAUF pour les joueurs qui ont déjà leurs dimensions définies par le SpaceshipManager
    if (type != protocol::EntityType::PLAYER) {
        if (!use_fixed_dimensions) {
            engine::Vector2f tex_size = textures_.get_texture_size(sprite.texture);
            if (tex_size.x > 0 && tex_size.y > 0) {
                float scale_x = dims.width / tex_size.x;
                float scale_y = dims.height / tex_size.y;
                float scale = std::min(scale_x, scale_y);

                if (type == protocol::EntityType::ENEMY_BASIC ||
                    type == protocol::EntityType::ENEMY_FAST ||
                    type == protocol::EntityType::ENEMY_TANK ||
                    type == protocol::EntityType::ENEMY_BOSS ||
                    type == protocol::EntityType::PROJECTILE_PLAYER) {
                    scale *= 2.0f;
                }

                sprite.width = tex_size.x * scale;
                sprite.height = tex_size.y * scale;
            }
        }

        sprite.origin_x = sprite.width / 2.0f;
        sprite.origin_y = sprite.height / 2.0f;
    }

    return sprite;
}

std::string EntityManager::get_player_label(uint32_t server_id) {
    // First, try to find the player_id for this server_id
    auto pid_it = server_to_player_id_.find(server_id);
    if (pid_it != server_to_player_id_.end()) {
        uint32_t player_id = pid_it->second;
        auto name_it = player_names_.find(player_id);
        if (name_it != player_names_.end() && !name_it->second.empty()) {
            return name_it->second;
        }
        return std::string("Player ") + std::to_string(player_id);
    }
    // Fallback: try server_id directly
    auto it = player_names_.find(server_id);
    if (it != player_names_.end() && !it->second.empty())
        return it->second;
    return std::string("Player ") + std::to_string(server_id);
}

void EntityManager::ensure_player_name_tag(uint32_t server_id, float x, float y) {
    auto& texts = registry_.get_components<UIText>();
    auto& positions = registry_.get_components<Position>();
    auto it = player_name_tags_.find(server_id);
    Entity text_entity;

    if (it == player_name_tags_.end()) {
        text_entity = registry_.spawn_entity();
        player_name_tags_[server_id] = text_entity;
        registry_.add_component(text_entity, Position{x, y - 30.0f});
        registry_.add_component(text_entity, UIText{
            get_player_label(server_id),
            engine::Color::White,
            engine::Color{0, 0, 0, 180},
            20,
            true,
            1.5f,
            1.5f,
            true,
            105
        });
    } else {
        text_entity = it->second;
        if (texts.has_entity(text_entity)) {
            texts[text_entity].text = get_player_label(server_id);
        }
    }
}

Entity EntityManager::spawn_or_update_entity(uint32_t server_id, protocol::EntityType type,
                                             float x, float y, uint16_t health, uint8_t subtype) {
    Entity entity;
    const bool is_new = server_to_local_.find(server_id) == server_to_local_.end();
    const uint32_t previous_local_server_id = local_player_entity_id_;

    if (is_new) {
        entity = registry_.spawn_entity();
        server_to_local_[server_id] = entity;
        registry_.add_component(entity, NetworkId{server_id});
    } else {
        entity = server_to_local_[server_id];
    }

    server_types_[server_id] = type;
    stale_counters_[server_id] = 0;

    // For player entities, decode subtype: high 4 bits = player_id, low 4 bits = skin_id
    uint8_t decoded_player_id = 0;
    uint8_t decoded_skin_id = 0;
    if (type == protocol::EntityType::PLAYER && subtype != 0) {
        decoded_player_id = (subtype >> 4) & 0x0F;
        decoded_skin_id = subtype & 0x0F;
        server_to_player_id_[server_id] = decoded_player_id;
    }

    // Check if this is the local player
    bool local_subtype_match = false;
    if (type == protocol::EntityType::PLAYER && local_player_id_ != 0) {
        uint8_t local_id_byte = static_cast<uint8_t>(local_player_id_ & 0x0Fu);
        if (decoded_player_id == local_id_byte) {
            local_player_entity_id_ = server_id;
            local_subtype_match = true;
        }
    }
    bool highlight_as_local = (server_id == local_player_entity_id_) || local_subtype_match;

    auto& localPlayers = registry_.get_components<LocalPlayer>();
    const uint32_t invalid_local_server = std::numeric_limits<uint32_t>::max();
    if (highlight_as_local) {
        if (previous_local_server_id != invalid_local_server &&
            previous_local_server_id != local_player_entity_id_) {
            auto prev_it = server_to_local_.find(previous_local_server_id);
            if (prev_it != server_to_local_.end() && localPlayers.has_entity(prev_it->second)) {
                registry_.remove_component<LocalPlayer>(prev_it->second);
            }
        }
        if (!localPlayers.has_entity(entity)) {
            registry_.add_component(entity, LocalPlayer{});
        }
    } else if (localPlayers.has_entity(entity)) {
        registry_.remove_component<LocalPlayer>(entity);
    }

    // Mark projectiles, players, and moving entities for local integration (smooth movement)
    if (!highlight_as_local && (type == protocol::EntityType::PROJECTILE_PLAYER ||
        type == protocol::EntityType::PROJECTILE_ENEMY ||
        type == protocol::EntityType::PLAYER ||
        type == protocol::EntityType::ENEMY_BASIC ||
        type == protocol::EntityType::ENEMY_FAST ||
        type == protocol::EntityType::ENEMY_TANK ||
        type == protocol::EntityType::ENEMY_BOSS ||
        type == protocol::EntityType::POWERUP_HEALTH ||
        type == protocol::EntityType::POWERUP_SHIELD ||
        type == protocol::EntityType::POWERUP_SPEED ||
        type == protocol::EntityType::POWERUP_SCORE)) {
        locally_integrated_.insert(server_id);
    } else {
        locally_integrated_.erase(server_id);
    }

    // Update position
    auto& positions = registry_.get_components<Position>();
    if (positions.has_entity(entity)) {
        positions[entity].x = x;
        positions[entity].y = y;
    } else {
        registry_.add_component(entity, Position{x, y});
    }

    // Update sprite - use decoded_skin_id for players, raw subtype for others
    uint8_t sprite_subtype = (type == protocol::EntityType::PLAYER) ? decoded_skin_id : subtype;
    Sprite sprite = build_sprite(type, highlight_as_local, sprite_subtype);
    auto& sprites = registry_.get_components<Sprite>();
    if (sprites.has_entity(entity)) {
        sprites[entity] = sprite;
    } else {
        registry_.add_component(entity, sprite);
    }

    // Update collider
    auto& colliders = registry_.get_components<Collider>();
    EntityDimensions collider_dims = get_collider_dimensions(type, sprite_subtype);
    if (colliders.has_entity(entity)) {
        colliders[entity].width = collider_dims.width;
        colliders[entity].height = collider_dims.height;
    } else {
        registry_.add_component(entity, Collider{collider_dims.width, collider_dims.height});
    }

    // Ensure bonus metadata for powerups so client-side collection can trigger visuals
    auto& bonuses = registry_.get_components<Bonus>();
    const bool is_powerup = (type == protocol::EntityType::POWERUP_HEALTH ||
        type == protocol::EntityType::POWERUP_SHIELD ||
        type == protocol::EntityType::POWERUP_SPEED ||
        type == protocol::EntityType::POWERUP_SCORE);
    if (is_powerup && type != protocol::EntityType::POWERUP_SCORE) {
        BonusType bonus_type = BonusType::HEALTH;
        switch (subtype) {
            case 0:
                bonus_type = BonusType::HEALTH;
                break;
            case 1:
                bonus_type = BonusType::SHIELD;
                break;
            case 2:
                bonus_type = BonusType::SPEED;
                break;
            case 3:
                bonus_type = BonusType::BONUS_WEAPON;
                break;
            default:
                bonus_type = BonusType::HEALTH;
                break;
        }
        if (bonuses.has_entity(entity)) {
            bonuses[entity].type = bonus_type;
            bonuses[entity].radius = rtype::shared::config::BONUS_SIZE / 2.0f;
        } else {
            registry_.add_component(entity, Bonus{bonus_type, rtype::shared::config::BONUS_SIZE / 2.0f});
        }

        // Add velocity so bonus scrolls with the game (moves left)
        auto& velocities = registry_.get_components<Velocity>();
        if (!velocities.has_entity(entity)) {
            registry_.add_component(entity, Velocity{-rtype::shared::config::GAME_SCROLL_SPEED, 0.0f});
        }
    } else if (bonuses.has_entity(entity)) {
        registry_.remove_component<Bonus>(entity);
    }

    // Update health
    auto& healths = registry_.get_components<Health>();
    int hp = static_cast<int>(health);
    if (healths.has_entity(entity)) {
        healths[entity].current = hp;
        healths[entity].max = std::max(healths[entity].max, hp);
    } else {
        Health comp;
        comp.current = hp;
        comp.max = hp;
        registry_.add_component(entity, comp);
    }

    // Ensure score component
    auto& scores = registry_.get_components<Score>();
    if (!scores.has_entity(entity)) {
        registry_.add_component(entity, Score{0});
    }

    // Tag walls so the client-side prediction system can collide against them
    auto& walls = registry_.get_components<Wall>();
    if (type == protocol::EntityType::WALL) {
        if (!walls.has_entity(entity)) {
            registry_.add_component(entity, Wall{});
        }
    } else if (walls.has_entity(entity)) {
        registry_.remove_component<Wall>(entity);
    }

    // Handle controllable - keep on ALL players for wall collision resolution
    auto& controllables = registry_.get_components<Controllable>();
    if (type == protocol::EntityType::PLAYER) {
        if (!controllables.has_entity(entity)) {
            registry_.add_component(entity, Controllable{300.0f});
        }
    } else if (controllables.has_entity(entity)) {
        // Remove from non-player entities
        registry_.remove_component<Controllable>(entity);
    }

    // Mark local player with LocalPlayer component for HUD to find

    if (type == protocol::EntityType::PLAYER && highlight_as_local) {
        if (!localPlayers.has_entity(entity)) {
            registry_.add_component(entity, LocalPlayer{});
        }
    } else if (localPlayers.has_entity(entity)) {
        // Remove from non-local-player entities
        registry_.remove_component<LocalPlayer>(entity);
    }

    // Handle player animation
    if (type == protocol::EntityType::PLAYER) {
        auto& animations = registry_.get_components<SpriteAnimation>();
        if (!animations.has_entity(entity)) {
            SpriteAnimation anim{};
            const auto& frames = textures_.get_player_frames();
            anim.frames.insert(anim.frames.end(), frames.begin(), frames.end());
            anim.frameTime = 0.1f;
            anim.loop = true;
            anim.playing = true;
            registry_.add_component(entity, anim);
        }
        ensure_player_name_tag(server_id, x, y);
        // Note: Le vaisseau bonus n'est plus créé automatiquement ici
        // Il sera créé quand le joueur collectera le bonus BONUS_WEAPON
    }

    // Créer un effet de feu pour les nouveaux projectiles joueur via ECS event
    if (type == protocol::EntityType::PROJECTILE_PLAYER && is_new) {
        // Ajouter l'animation de balle au projectile
        auto& bulletAnims = registry_.get_components<BulletAnimation>();
        if (!bulletAnims.has_entity(entity)) {
            registry_.add_component(entity, BulletAnimation{0.0f, 0.1f, 0});
        }

        // Trouver le joueur ou vaisseau bonus le plus proche (celui qui a tiré)
        // IMPORTANT: Les joueurs tirent vers la DROITE, donc on cherche seulement
        // les tireurs qui sont À GAUCHE du projectile (shooterX < projectileX)
        Entity closestShooter = 0;
        float closestDistance = 1000000.0f;
        bool isCompanionShot = false;
        auto& positions = registry_.get_components<Position>();
        auto& bonusWeapons = registry_.get_components<BonusWeapon>();

        // D'abord chercher parmi les joueurs
        for (const auto& [srv_id, local_ent] : server_to_local_) {
            if (server_types_[srv_id] == protocol::EntityType::PLAYER && positions.has_entity(local_ent)) {
                float playerX = positions[local_ent].x;
                float playerY = positions[local_ent].y;

                // Ne considérer que les joueurs À GAUCHE du projectile
                // (car ils tirent vers la droite)
                if (playerX < x) {
                    float dx = playerX - x;
                    float dy = playerY - y;
                    float dist = dx*dx + dy*dy;
                    if (dist < closestDistance) {
                        closestDistance = dist;
                        closestShooter = local_ent;
                        isCompanionShot = false;
                    }
                }
            }
        }

        // Ensuite chercher parmi les vaisseaux bonus (companion turrets)
        for (size_t i = 0; i < bonusWeapons.size(); i++) {
            Entity playerWithBonus = bonusWeapons.get_entity_at(i);
            const BonusWeapon& bw = bonusWeapons[playerWithBonus];
            if (bw.weaponEntity != static_cast<size_t>(-1)) {
                Entity companionEntity = static_cast<Entity>(bw.weaponEntity);
                if (positions.has_entity(companionEntity)) {
                    float companionX = positions[companionEntity].x;
                    float companionY = positions[companionEntity].y;

                    // Ne considérer que les companions À GAUCHE du projectile
                    if (companionX < x) {
                        float dx = companionX - x;
                        float dy = companionY - y;
                        float dist = dx*dx + dy*dy;
                        if (dist < closestDistance) {
                            closestDistance = dist;
                            closestShooter = companionEntity;
                            isCompanionShot = true;
                        }
                    }
                }
            }
        }

        // Publish event to MuzzleFlashSystem (ECS architecture)
        if (closestShooter != 0) {
            // Get shooter width from Collider
            auto& colliders = registry_.get_components<Collider>();
            float shooterWidth = 104.0f;  // Default to medium ship

            if (colliders.has_entity(closestShooter)) {
                shooterWidth = colliders[closestShooter].width;
            }

            registry_.get_event_bus().publish(ecs::MuzzleFlashSpawnEvent{
                closestShooter, x, y, isCompanionShot, false, shooterWidth
            });

            // Emit companion shot sound event if this is a companion turret shot
            if (isCompanionShot) {
                registry_.get_event_bus().publish(ecs::CompanionShotEvent{
                    closestShooter, x, y
                });
            }
        }
    }

    // Créer un effet de feu pour les nouveaux projectiles ennemis via ECS event
    if (type == protocol::EntityType::PROJECTILE_ENEMY && is_new) {
        // Ajouter l'animation de balle au projectile
        auto& bulletAnims = registry_.get_components<BulletAnimation>();
        if (!bulletAnims.has_entity(entity)) {
            registry_.add_component(entity, BulletAnimation{0.0f, 0.1f, 0});
        }

        // Trouver l'ennemi le plus proche (celui qui a tiré)
        // IMPORTANT: Les ennemis tirent vers la GAUCHE, donc on cherche seulement
        // les ennemis qui sont À DROITE du projectile (enemyX > projectileX)
        Entity closestEnemy = 0;
        float closestDistance = 1000000.0f;
        float enemyWidth = 120.0f;  // Default to Basic enemy width
        auto& positions = registry_.get_components<Position>();
        auto& colliders = registry_.get_components<Collider>();

        // Chercher parmi tous les ennemis
        for (const auto& [srv_id, local_ent] : server_to_local_) {
            auto srv_type = server_types_[srv_id];

            // Check if this is an enemy type
            if ((srv_type == protocol::EntityType::ENEMY_BASIC ||
                 srv_type == protocol::EntityType::ENEMY_FAST ||
                 srv_type == protocol::EntityType::ENEMY_TANK ||
                 srv_type == protocol::EntityType::ENEMY_BOSS) &&
                positions.has_entity(local_ent)) {

                float enemyX = positions[local_ent].x;
                float enemyY = positions[local_ent].y;

                // Ne considérer que les ennemis À DROITE du projectile
                // (car ils tirent vers la gauche)
                if (enemyX > x) {
                    float dx = enemyX - x;
                    float dy = enemyY - y;
                    float dist = dx*dx + dy*dy;

                    if (dist < closestDistance) {
                        closestDistance = dist;
                        closestEnemy = local_ent;

                        // Get width from Collider if available
                        if (colliders.has_entity(local_ent)) {
                            enemyWidth = colliders[local_ent].width;
                        }
                    }
                }
            }
        }

        // Publish event to MuzzleFlashSystem
        if (closestEnemy != 0) {
            registry_.get_event_bus().publish(ecs::MuzzleFlashSpawnEvent{
                closestEnemy, x, y, false, true, enemyWidth
            });
        }
    }

    return entity;
}

void EntityManager::remove_entity(uint32_t server_id) {
    auto it = server_to_local_.find(server_id);
    if (it == server_to_local_.end())
        return;

    Entity entity_to_remove = it->second;

    // Emit explosion sound based on entity type
    auto type_it = server_types_.find(server_id);
    if (type_it != server_types_.end()) {
        protocol::EntityType entity_type = type_it->second;
        auto& positions = registry_.get_components<Position>();
        float pos_x = 0.0f, pos_y = 0.0f;
        if (positions.has_entity(entity_to_remove)) {
            pos_x = positions[entity_to_remove].x;
            pos_y = positions[entity_to_remove].y;
        }

        // Determine explosion sound type based on entity type
        ecs::ExplosionSoundEvent::ExplosionType sound_type;
        bool should_emit_sound = true;

        switch (entity_type) {
            case protocol::EntityType::ENEMY_BASIC:
            case protocol::EntityType::ENEMY_FAST:
                sound_type = ecs::ExplosionSoundEvent::ExplosionType::ENEMY_BASIC;
                break;
            case protocol::EntityType::ENEMY_TANK:
                sound_type = ecs::ExplosionSoundEvent::ExplosionType::ENEMY_TANK;
                break;
            case protocol::EntityType::ENEMY_BOSS:
                sound_type = ecs::ExplosionSoundEvent::ExplosionType::ENEMY_BOSS;
                break;
            case protocol::EntityType::PLAYER:
                sound_type = ecs::ExplosionSoundEvent::ExplosionType::PLAYER;
                break;
            default:
                // Don't emit sound for projectiles, walls, powerups, etc.
                should_emit_sound = false;
                break;
        }

        if (should_emit_sound) {
            registry_.get_event_bus().publish(ecs::ExplosionSoundEvent{
                sound_type, pos_x, pos_y, 1.0f
            });
        }
    }

    // If this entity has a companion turret, destroy it directly before killing the player
    // This ensures the companion is cleaned up while the player entity still exists
    auto& bonusWeapons = registry_.get_components<BonusWeapon>();
    if (bonusWeapons.has_entity(entity_to_remove)) {
        const BonusWeapon& bonusWeapon = bonusWeapons[entity_to_remove];
        if (bonusWeapon.weaponEntity != static_cast<size_t>(-1)) {
            Entity companionEntity = static_cast<Entity>(bonusWeapon.weaponEntity);
            // Kill the companion entity directly
            registry_.kill_entity(companionEntity);
        }
        registry_.remove_component<BonusWeapon>(entity_to_remove);
    }


    registry_.kill_entity(entity_to_remove);
    server_types_.erase(server_id);
    stale_counters_.erase(server_id);
    locally_integrated_.erase(server_id);
    snapshot_updated_.erase(server_id);
    server_to_player_id_.erase(server_id);
    server_to_local_.erase(it);

    if (local_player_entity_id_ == server_id)
        local_player_entity_id_ = std::numeric_limits<uint32_t>::max();

    auto tag_it = player_name_tags_.find(server_id);
    if (tag_it != player_name_tags_.end()) {
        registry_.kill_entity(tag_it->second);
        player_name_tags_.erase(tag_it);
    }
}

void EntityManager::clear_all() {
    // First, destroy all companion entities attached to players
    auto& bonusWeapons = registry_.get_components<BonusWeapon>();
    for (const auto& pair : server_to_local_) {
        Entity entity = pair.second;
        if (bonusWeapons.has_entity(entity)) {
            const BonusWeapon& bonusWeapon = bonusWeapons[entity];
            if (bonusWeapon.weaponEntity != static_cast<size_t>(-1)) {
                Entity companionEntity = static_cast<Entity>(bonusWeapon.weaponEntity);
                registry_.kill_entity(companionEntity);
            }
        }
    }

    // Now kill all networked entities
    for (const auto& pair : server_to_local_)
        registry_.kill_entity(pair.second);

    server_to_local_.clear();
    server_types_.clear();
    stale_counters_.clear();
    locally_integrated_.clear();
    snapshot_updated_.clear();
    server_to_player_id_.clear();
    local_player_entity_id_ = std::numeric_limits<uint32_t>::max();

    for (const auto& tag_pair : player_name_tags_)
        registry_.kill_entity(tag_pair.second);
    player_name_tags_.clear();
}

void EntityManager::process_snapshot_update(const std::unordered_set<uint32_t>& updated_ids) {
    std::vector<uint32_t> stale_removals;

    for (const auto& pair : server_to_local_) {
        uint32_t server_id = pair.first;
        if (updated_ids.count(server_id))
            continue;

        // Skip stale check for entities that are locally integrated (predicted client-side)
        // This includes walls, powerups/bonuses that scroll predictably
        if (locally_integrated_.count(server_id))
            continue;

        // Also skip stale check for powerups/bonuses based on their type
        auto type_it = server_types_.find(server_id);
        if (type_it != server_types_.end()) {
            protocol::EntityType type = type_it->second;
            // Powerups/bonuses are not in snapshots - they scroll predictably like walls
            if (type == protocol::EntityType::POWERUP_HEALTH ||
                type == protocol::EntityType::POWERUP_SHIELD ||
                type == protocol::EntityType::POWERUP_SPEED ||
                type == protocol::EntityType::POWERUP_SCORE ||
                type == protocol::EntityType::WALL) {
                continue;
            }
        }

        auto counter_it = stale_counters_.find(server_id);
        if (counter_it == stale_counters_.end()) {
            counter_it = stale_counters_.emplace(server_id, 0).first;
        }

        uint8_t& counter = counter_it->second;
        if (counter < 250)
            counter++;

        if (counter > ENTITY_STALE_THRESHOLD) {
            stale_removals.push_back(server_id);
        }
    }

    for (uint32_t id : stale_removals) {
        remove_entity(id);
    }

    snapshot_updated_.clear();
}

void EntityManager::update_projectiles(float delta_time) {
    if (locally_integrated_.empty())
        return;

    std::vector<uint32_t> despawn_list;
    despawn_list.reserve(locally_integrated_.size());
    auto& positions = registry_.get_components<Position>();
    auto& velocities = registry_.get_components<Velocity>();

    for (uint32_t server_id : locally_integrated_) {
        if (snapshot_updated_.count(server_id))
            continue;

        auto it = server_to_local_.find(server_id);
        if (it == server_to_local_.end())
            continue;

        Entity entity = it->second;
        if (!positions.has_entity(entity) || !velocities.has_entity(entity))
            continue;

        Position& pos = positions[entity];
        Velocity& vel = velocities[entity];
        pos.x += vel.x * delta_time;
        pos.y += vel.y * delta_time;

        // Despawn projectiles and powerups that go out of bounds
        auto type_it = server_types_.find(server_id);
        if (type_it == server_types_.end())
            continue;

        protocol::EntityType etype = type_it->second;
        bool is_projectile = (etype == protocol::EntityType::PROJECTILE_PLAYER ||
                              etype == protocol::EntityType::PROJECTILE_ENEMY);
        bool is_powerup = (etype == protocol::EntityType::POWERUP_HEALTH ||
                           etype == protocol::EntityType::POWERUP_SHIELD ||
                           etype == protocol::EntityType::POWERUP_SPEED ||
                           etype == protocol::EntityType::POWERUP_SCORE);

        // Despawn if out of bounds (projectiles and powerups)
        if ((is_projectile || is_powerup) &&
            (pos.x < -PROJECTILE_DESPAWN_MARGIN ||
            pos.x > screen_width_ + PROJECTILE_DESPAWN_MARGIN ||
            pos.y < -PROJECTILE_DESPAWN_MARGIN ||
            pos.y > screen_height_ + PROJECTILE_DESPAWN_MARGIN)) {
            despawn_list.push_back(server_id);
        }
    }

    for (uint32_t id : despawn_list) {
        remove_entity(id);
    }
}

void EntityManager::update_name_tags() {
    auto& positions = registry_.get_components<Position>();
    auto& sprites = registry_.get_components<Sprite>();
    auto& texts = registry_.get_components<UIText>();
    auto& gameStates = registry_.get_components<GameState>();
    std::vector<uint32_t> orphan_tags;

    // Check if game is over or victory - hide player names
    bool hideNames = false;
    for (size_t i = 0; i < gameStates.size(); ++i) {
        Entity entity = gameStates.get_entity_at(i);
        const GameState& state = gameStates[entity];
        if (state.currentState == GameStateType::GAME_OVER ||
            state.currentState == GameStateType::VICTORY) {
            hideNames = true;
            break;
        }
    }

    for (const auto& [server_id, text_entity] : player_name_tags_) {
        auto ent_it = server_to_local_.find(server_id);
        if (ent_it == server_to_local_.end()) {
            orphan_tags.push_back(server_id);
            continue;
        }

        Entity player_entity = ent_it->second;
        if (!positions.has_entity(player_entity) || !positions.has_entity(text_entity) ||
            !texts.has_entity(text_entity))
            continue;

        // Hide or show name tag based on game state
        texts[text_entity].active = !hideNames;

        const Position& player_pos = positions[player_entity];
        float sprite_height = 0.0f;
        if (sprites.has_entity(player_entity))
            sprite_height = sprites[player_entity].height;

        // Position is center-based, place tag above the top edge of the sprite
        Position& tag_pos = positions[text_entity];
        tag_pos.x = player_pos.x; // Center X
        tag_pos.y = player_pos.y - (sprite_height / 2.0f) - 30.0f; // Above top edge
    }

    for (uint32_t server_id : orphan_tags) {
        auto it = player_name_tags_.find(server_id);
        if (it != player_name_tags_.end()) {
            registry_.kill_entity(it->second);
            player_name_tags_.erase(it);
        }
    }
}

void EntityManager::spawn_explosion(float x, float y, float scale) {
    engine::TextureHandle explosion_tex = textures_.get_explosion();
    if (explosion_tex == engine::INVALID_HANDLE)
        return;

    Entity explosion = registry_.spawn_entity();
    registry_.add_component(explosion, Position{x, y});

    Sprite sprite{};
    sprite.texture = explosion_tex;
    sprite.layer = 30;
    sprite.source_rect.x = 0.0f;
    sprite.source_rect.y = 0.0f;
    sprite.source_rect.width = rtype::shared::config::EXPLOSION_FRAME_SIZE;
    sprite.source_rect.height = rtype::shared::config::EXPLOSION_FRAME_SIZE;

    float size = rtype::shared::config::EXPLOSION_FRAME_SIZE * rtype::shared::config::EXPLOSION_DRAW_SCALE * scale;
    sprite.width = size;
    sprite.height = size;
    sprite.origin_x = size / 2.0f;
    sprite.origin_y = size / 2.0f;

    registry_.add_component(explosion, sprite);

    engine::Vector2f tex_size = textures_.get_texture_size(explosion_tex);
    ExplosionAnimation anim{};
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> speed_dist(0.7f, 1.3f);
    anim.frameDuration = rtype::shared::config::EXPLOSION_FRAME_TIME * speed_dist(rng);
    anim.frameWidth = static_cast<int>(rtype::shared::config::EXPLOSION_FRAME_SIZE);
    anim.frameHeight = static_cast<int>(rtype::shared::config::EXPLOSION_FRAME_SIZE);

    if (anim.frameWidth <= 0)
        anim.frameWidth = 16;
    if (anim.frameHeight <= 0)
        anim.frameHeight = 16;

    anim.framesPerRow = anim.frameWidth > 0 ? static_cast<int>(tex_size.x / anim.frameWidth) : 1;
    if (anim.framesPerRow <= 0)
        anim.framesPerRow = 1;
    int rows = anim.frameHeight > 0 ? static_cast<int>(tex_size.y / anim.frameHeight) : 1;
    if (rows <= 0)
        rows = 1;
    anim.totalFrames = std::max(1, anim.framesPerRow * rows);

    registry_.add_component(explosion, anim);
}

void EntityManager::set_player_name(uint32_t server_id, const std::string& name) {
    player_names_[server_id] = name;

    auto tag_it = player_name_tags_.find(server_id);
    if (tag_it != player_name_tags_.end()) {
        auto& texts = registry_.get_components<UIText>();
        if (texts.has_entity(tag_it->second)) {
            texts[tag_it->second].text = name;
        }
    }
}

std::string EntityManager::get_player_name(uint32_t player_id) const {
    auto it = player_names_.find(player_id);
    if (it != player_names_.end()) {
        return it->second;
    }
    return "Player " + std::to_string(player_id);
}

Entity EntityManager::get_entity(uint32_t server_id) const {
    auto it = server_to_local_.find(server_id);
    if (it != server_to_local_.end())
        return it->second;
    return Entity(std::numeric_limits<uint32_t>::max());
}

bool EntityManager::has_entity(uint32_t server_id) const {
    return server_to_local_.find(server_id) != server_to_local_.end();
}

bool EntityManager::get_entity_type(uint32_t server_id, protocol::EntityType& out_type) const {
    auto it = server_types_.find(server_id);
    if (it != server_types_.end()) {
        out_type = it->second;
        return true;
    }
    return false;
}

void EntityManager::update_player_skin(uint32_t server_id, uint8_t new_skin_id) {
    auto it = server_to_local_.find(server_id);
    if (it == server_to_local_.end())
        return;

    Entity entity = it->second;

    // Extract color and ship type from skin_id
    // skin_id = color * 5 + ship_type
    ShipColor color = static_cast<ShipColor>(new_skin_id / 5);
    ShipType ship_type = static_cast<ShipType>(new_skin_id % 5);

    // Get new sprite from ship manager
    engine::Sprite ship_sprite = textures_.get_ship_manager().create_ship_sprite(color, ship_type, 4.0f);

    // Update sprite component
    auto& sprites = registry_.get_components<Sprite>();
    if (sprites.has_entity(entity)) {
        Sprite& sprite = sprites[entity];
        sprite.texture = ship_sprite.texture_handle;
        sprite.source_rect.x = ship_sprite.source_rect.x;
        sprite.source_rect.y = ship_sprite.source_rect.y;
        sprite.source_rect.width = ship_sprite.source_rect.width;
        sprite.source_rect.height = ship_sprite.source_rect.height;
        sprite.width = ship_sprite.size.x;
        sprite.height = ship_sprite.size.y;
        sprite.origin_x = ship_sprite.origin.x;
        sprite.origin_y = ship_sprite.origin.y;
    }

    // Update collider based on new ship type
    auto& colliders = registry_.get_components<Collider>();
    if (colliders.has_entity(entity)) {
        auto hitbox = rtype::game::get_hitbox_dimensions_from_skin_id(new_skin_id);
        colliders[entity].width = hitbox.width;
        colliders[entity].height = hitbox.height;
    }

    std::cout << "[EntityManager] Updated player " << server_id
              << " skin to " << static_cast<int>(new_skin_id)
              << " (color: " << static_cast<int>(color)
              << ", ship: " << static_cast<int>(ship_type) << ")\n";
}

}
