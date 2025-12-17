#include "EntityManager.hpp"
#include "GameDimensions.hpp"
#include <iostream>
#include <algorithm>

namespace rtype::client {

namespace {
constexpr float PROJECTILE_DESPAWN_MARGIN = 250.0f;
constexpr uint8_t ENTITY_STALE_THRESHOLD = 6; // ~0.1s before removing missing entities
}

struct EntityDimensions {
    float width;
    float height;
};

static EntityDimensions get_collider_dimensions(protocol::EntityType type) {
    using namespace rtype::shared::dimensions;

    // Map bonus types to powerup types
    if (type == protocol::EntityType::BONUS_HEALTH)
        type = protocol::EntityType::POWERUP_HEALTH;
    else if (type == protocol::EntityType::BONUS_SHIELD)
        type = protocol::EntityType::POWERUP_SHIELD;
    else if (type == protocol::EntityType::BONUS_SPEED)
        type = protocol::EntityType::POWERUP_SPEED;

    switch (type) {
        case protocol::EntityType::PLAYER:
            return {PLAYER_WIDTH, PLAYER_HEIGHT};
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
            return {BONUS_SIZE * 2.0f, BONUS_SIZE * 2.0f};
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
    sprite.texture = textures_.get_enemy();
    auto dims = get_collider_dimensions(type);

    // Initialize dimensions with collider size as fallback
    sprite.width = dims.width;
    sprite.height = dims.height;

    sprite.tint = engine::Color::White;
    sprite.layer = 5;

    switch (type) {
        case protocol::EntityType::PLAYER:
            sprite.texture = textures_.get_player_frame(0);
            sprite.layer = 10;
            sprite.tint = is_local_player ? engine::Color::Cyan : engine::Color::White;
            break;
        case protocol::EntityType::ENEMY_FAST:
            sprite.tint = engine::Color{255, 180, 0, 255};
            break;
        case protocol::EntityType::ENEMY_TANK:
            sprite.tint = engine::Color{200, 80, 80, 255};
            break;
        case protocol::EntityType::ENEMY_BOSS:
            sprite.layer = 6;
            sprite.tint = engine::Color{180, 0, 255, 255};
            break;
        case protocol::EntityType::WALL:
            sprite.texture = textures_.get_wall();
            sprite.layer = 2;
            sprite.tint = engine::Color{180, 180, 255, 255};
            break;
        case protocol::EntityType::PROJECTILE_PLAYER:
        case protocol::EntityType::PROJECTILE_ENEMY:
            sprite.texture = textures_.get_projectile();
            sprite.layer = 20;
            sprite.tint = type == protocol::EntityType::PROJECTILE_PLAYER
                ? engine::Color::Cyan
                : engine::Color::Red;
            break;
        case protocol::EntityType::POWERUP_HEALTH:
        case protocol::EntityType::POWERUP_SHIELD:
        case protocol::EntityType::POWERUP_SPEED:
        case protocol::EntityType::POWERUP_SCORE:
            sprite.texture = textures_.get_projectile();
            sprite.layer = 4;
            sprite.tint = engine::Color{120, 255, 120, 255};
            break;
        default:
            break;
    }

    if (type == protocol::EntityType::ENEMY_BASIC && subtype != 0) {
        sprite.tint = engine::Color{200, 200, 255, 255};
    }

    // Adjust sprite dimensions to preserve aspect ratio while fitting in collider box
    engine::Vector2f tex_size = textures_.get_texture_size(sprite.texture);
    if (tex_size.x > 0 && tex_size.y > 0) {
        float scale_x = dims.width / tex_size.x;
        float scale_y = dims.height / tex_size.y;
        float scale = std::min(scale_x, scale_y);

        // Apply x2 scaling only for enemies
        if (type == protocol::EntityType::ENEMY_BASIC ||
            type == protocol::EntityType::ENEMY_FAST ||
            type == protocol::EntityType::ENEMY_TANK ||
            type == protocol::EntityType::ENEMY_BOSS) {
            scale *= 2.0f;
        }

        sprite.width = tex_size.x * scale;
        sprite.height = tex_size.y * scale;
    }

    return sprite;
}

std::string EntityManager::get_player_label(uint32_t server_id) {
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

    if (is_new) {
        entity = registry_.spawn_entity();
        server_to_local_[server_id] = entity;
        registry_.add_component(entity, NetworkId{server_id});
    } else {
        entity = server_to_local_[server_id];
    }

    server_types_[server_id] = type;
    stale_counters_[server_id] = 0;

    // Mark projectiles, players, and moving entities for local integration (smooth movement)
    if (type == protocol::EntityType::PROJECTILE_PLAYER || 
        type == protocol::EntityType::PROJECTILE_ENEMY ||
        type == protocol::EntityType::PLAYER ||
        type == protocol::EntityType::ENEMY_BASIC ||
        type == protocol::EntityType::ENEMY_FAST ||
        type == protocol::EntityType::ENEMY_TANK ||
        type == protocol::EntityType::ENEMY_BOSS ||
        type == protocol::EntityType::POWERUP_HEALTH ||
        type == protocol::EntityType::POWERUP_SHIELD ||
        type == protocol::EntityType::POWERUP_SPEED ||
        type == protocol::EntityType::POWERUP_SCORE) {
        locally_integrated_.insert(server_id);
    } else {
        locally_integrated_.erase(server_id);
    }

    // Check if this is the local player
    bool local_subtype_match = false;
    if (type == protocol::EntityType::PLAYER && local_player_id_ != 0) {
        uint8_t local_id_byte = static_cast<uint8_t>(local_player_id_ & 0xFFu);
        if (subtype == local_id_byte) {
            local_player_entity_id_ = server_id;
            local_subtype_match = true;
        }
    }
    bool highlight_as_local = (server_id == local_player_entity_id_) || local_subtype_match;

    // Update position
    auto& positions = registry_.get_components<Position>();
    if (positions.has_entity(entity)) {
        positions[entity].x = x;
        positions[entity].y = y;
    } else {
        registry_.add_component(entity, Position{x, y});
    }

    // Update sprite
    Sprite sprite = build_sprite(type, highlight_as_local, subtype);
    auto& sprites = registry_.get_components<Sprite>();
    if (sprites.has_entity(entity)) {
        sprites[entity] = sprite;
    } else {
        registry_.add_component(entity, sprite);
    }

    // Update collider
    auto& colliders = registry_.get_components<Collider>();
    EntityDimensions collider_dims = get_collider_dimensions(type);
    if (colliders.has_entity(entity)) {
        colliders[entity].width = collider_dims.width;
        colliders[entity].height = collider_dims.height;
    } else {
        registry_.add_component(entity, Collider{collider_dims.width, collider_dims.height});
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

    // Handle controllable
    auto& controllables = registry_.get_components<Controllable>();
    if (highlight_as_local) {
        if (!controllables.has_entity(entity)) {
            registry_.add_component(entity, Controllable{300.0f});
        }
    } else if (controllables.has_entity(entity) && server_id != local_player_entity_id_) {
        registry_.remove_component<Controllable>(entity);
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
    }

    return entity;
}

void EntityManager::remove_entity(uint32_t server_id) {
    auto it = server_to_local_.find(server_id);
    if (it == server_to_local_.end())
        return;

    registry_.kill_entity(it->second);
    server_types_.erase(server_id);
    stale_counters_.erase(server_id);
    locally_integrated_.erase(server_id);
    snapshot_updated_.erase(server_id);
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
    for (const auto& pair : server_to_local_)
        registry_.kill_entity(pair.second);

    server_to_local_.clear();
    server_types_.clear();
    stale_counters_.clear();
    locally_integrated_.clear();
    snapshot_updated_.clear();
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

        // Only despawn projectiles that go out of bounds
        auto type_it = server_types_.find(server_id);
        bool is_projectile = type_it != server_types_.end() && 
                            (type_it->second == protocol::EntityType::PROJECTILE_PLAYER || 
                             type_it->second == protocol::EntityType::PROJECTILE_ENEMY);

        if (is_projectile && 
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
    std::vector<uint32_t> orphan_tags;

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

        const Position& player_pos = positions[player_entity];
        float sprite_height = 0.0f;
        if (sprites.has_entity(player_entity))
            sprite_height = sprites[player_entity].height;

        Position& tag_pos = positions[text_entity];
        tag_pos.x = player_pos.x;
        tag_pos.y = player_pos.y - (sprite_height * 0.2f + 30.0f);
    }

    for (uint32_t server_id : orphan_tags) {
        auto it = player_name_tags_.find(server_id);
        if (it != player_name_tags_.end()) {
            registry_.kill_entity(it->second);
            player_name_tags_.erase(it);
        }
    }
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

Entity EntityManager::get_entity(uint32_t server_id) const {
    auto it = server_to_local_.find(server_id);
    if (it != server_to_local_.end())
        return it->second;
    return Entity(std::numeric_limits<uint32_t>::max());
}

bool EntityManager::has_entity(uint32_t server_id) const {
    return server_to_local_.find(server_id) != server_to_local_.end();
}

}
