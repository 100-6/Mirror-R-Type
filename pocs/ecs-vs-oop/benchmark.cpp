#include <iostream>
#include <vector>
#include <chrono>
#include <memory>
#include <algorithm>

// =============================================================================
// OOP IMPLEMENTATION (avec polymorphisme et indirection)
// =============================================================================

class Entity {
public:
    float x, y;      // Position
    float vx, vy;    // Velocity
    float health;

    Entity(float px, float py, float velx, float vely)
        : x(px), y(py), vx(velx), vy(vely), health(100.0f) {}

    virtual ~Entity() = default;

    virtual void update(float dt) {
        x += vx * dt;
        y += vy * dt;
    }

    virtual void takeDamage(float damage) {
        health -= damage;
    }

    virtual bool isAlive() const {
        return health > 0;
    }
};

class Player : public Entity {
public:
    Player(float px, float py, float velx, float vely)
        : Entity(px, py, velx, vely) {}

    void update(float dt) override {
        Entity::update(dt);
        // Bounds checking
        if (x < 0) x = 0;
        if (x > 800) x = 800;
        if (y < 0) y = 0;
        if (y > 600) y = 600;
    }
};

class Enemy : public Entity {
public:
    Enemy(float px, float py, float velx, float vely)
        : Entity(px, py, velx, vely) {}

    void update(float dt) override {
        Entity::update(dt);
        // Wrap around
        if (x < 0) x = 800;
        if (x > 800) x = 0;
        if (y < 0) y = 600;
        if (y > 600) y = 0;
    }
};

void benchmarkOOP(int numEntities, int iterations) {
    std::vector<std::unique_ptr<Entity>> entities;
    entities.reserve(numEntities);

    // Create entities (half players, half enemies)
    for (int i = 0; i < numEntities / 2; ++i) {
        entities.push_back(std::make_unique<Player>(
            static_cast<float>(rand() % 800),
            static_cast<float>(rand() % 600),
            static_cast<float>((rand() % 100) - 50),
            static_cast<float>((rand() % 100) - 50)
        ));
    }
    for (int i = 0; i < numEntities / 2; ++i) {
        entities.push_back(std::make_unique<Enemy>(
            static_cast<float>(rand() % 800),
            static_cast<float>(rand() % 600),
            static_cast<float>((rand() % 100) - 50),
            static_cast<float>((rand() % 100) - 50)
        ));
    }

    float dt = 0.016f;
    auto start = std::chrono::high_resolution_clock::now();

    for (int iter = 0; iter < iterations; ++iter) {
        // Update positions (virtual function calls)
        for (auto& e : entities) {
            if (!e->isAlive()) continue;
            e->update(dt);
        }

        // Apply damage (virtual function calls)
        for (auto& e : entities) {
            if (!e->isAlive()) continue;
            e->takeDamage(0.1f);
        }
    }

    // Cleanup dead entities after benchmark
    entities.erase(
        std::remove_if(entities.begin(), entities.end(),
            [](const std::unique_ptr<Entity>& e) { return !e->isAlive(); }),
        entities.end()
    );

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "  Time: " << duration.count() / 1000.0 << " ms";
    std::cout << " | Avg: " << duration.count() / static_cast<double>(iterations) << " µs/iter";
    std::cout << " | Remaining: " << entities.size() << "\n";
}

// =============================================================================
// ECS IMPLEMENTATION
// =============================================================================

struct Components {
    std::vector<float> x, y;       // Position
    std::vector<float> vx, vy;     // Velocity
    std::vector<float> health;
    std::vector<bool> active;
    std::vector<bool> isPlayer;    // Type flag

    void reserve(size_t count) {
        x.reserve(count);
        y.reserve(count);
        vx.reserve(count);
        vy.reserve(count);
        health.reserve(count);
        active.reserve(count);
        isPlayer.reserve(count);
    }

    void add(float px, float py, float velx, float vely, bool player) {
        x.push_back(px);
        y.push_back(py);
        vx.push_back(velx);
        vy.push_back(vely);
        health.push_back(100.0f);
        active.push_back(true);
        isPlayer.push_back(player);
    }

    size_t size() const { return x.size(); }
};

void updateMovement(Components& c, float dt) {
    const size_t n = c.size();
    for (size_t i = 0; i < n; ++i) {
        if (!c.active[i]) continue;
        
        c.x[i] += c.vx[i] * dt;
        c.y[i] += c.vy[i] * dt;

        if (c.isPlayer[i]) {
            // Bounds checking for players
            if (c.x[i] < 0) c.x[i] = 0;
            if (c.x[i] > 800) c.x[i] = 800;
            if (c.y[i] < 0) c.y[i] = 0;
            if (c.y[i] > 600) c.y[i] = 600;
        } else {
            // Wrap around for enemies
            if (c.x[i] < 0) c.x[i] = 800;
            if (c.x[i] > 800) c.x[i] = 0;
            if (c.y[i] < 0) c.y[i] = 600;
            if (c.y[i] > 600) c.y[i] = 0;
        }
    }
}

void updateHealth(Components& c, float damage) {
    const size_t n = c.size();
    for (size_t i = 0; i < n; ++i) {
        if (!c.active[i]) continue;
        c.health[i] -= damage;
        if (c.health[i] <= 0) {
            c.active[i] = false;
        }
    }
}

void cleanup(Components& c) {
    size_t write = 0;
    const size_t n = c.size();
    for (size_t read = 0; read < n; ++read) {
        if (c.active[read]) {
            if (write != read) {
                c.x[write] = c.x[read];
                c.y[write] = c.y[read];
                c.vx[write] = c.vx[read];
                c.vy[write] = c.vy[read];
                c.health[write] = c.health[read];
                c.active[write] = c.active[read];
                c.isPlayer[write] = c.isPlayer[read];
            }
            write++;
        }
    }
    c.x.resize(write);
    c.y.resize(write);
    c.vx.resize(write);
    c.vy.resize(write);
    c.health.resize(write);
    c.active.resize(write);
    c.isPlayer.resize(write);
}

void benchmarkECS(int numEntities, int iterations) {
    Components components;
    components.reserve(numEntities);

    // Create entities (half players, half enemies)
    for (int i = 0; i < numEntities / 2; ++i) {
        components.add(
            static_cast<float>(rand() % 800),
            static_cast<float>(rand() % 600),
            static_cast<float>((rand() % 100) - 50),
            static_cast<float>((rand() % 100) - 50),
            true // Player
        );
    }
    for (int i = 0; i < numEntities / 2; ++i) {
        components.add(
            static_cast<float>(rand() % 800),
            static_cast<float>(rand() % 600),
            static_cast<float>((rand() % 100) - 50),
            static_cast<float>((rand() % 100) - 50),
            false // Enemy
        );
    }

    float dt = 0.016f;
    auto start = std::chrono::high_resolution_clock::now();

    for (int iter = 0; iter < iterations; ++iter) {
        updateMovement(components, dt);
        updateHealth(components, 0.1f);
    }
    cleanup(components);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "  Time: " << duration.count() / 1000.0 << " ms";
    std::cout << " | Avg: " << duration.count() / static_cast<double>(iterations) << " µs/iter";
    std::cout << " | Remaining: " << components.size() << "\n";
}

// =============================================================================
// MAIN BENCHMARK
// =============================================================================

void runComparison(int numEntities, int iterations) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "Entities: " << numEntities << " | Iterations: " << iterations << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    std::cout << "OOP:";
    benchmarkOOP(numEntities, iterations);
    
    std::cout << "ECS:";
    benchmarkECS(numEntities, iterations);
}

int main() {
    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║        ECS vs OOP Performance Benchmark         ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n";

    runComparison(1000, 1000);
    runComparison(10000, 100);
    runComparison(50000, 10);
    runComparison(100000, 10);
    runComparison(500000, 5);

    std::cout << "\n";
    return 0;
}
