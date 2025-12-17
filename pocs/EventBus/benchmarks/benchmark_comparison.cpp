#include <benchmark/benchmark.h>
#include <memory>

#include "approach_a/AudioEngine.hpp"
#include "approach_a/ScoreManager.hpp"
#include "approach_a/Renderer.hpp"
#include "approach_a/PhysicsEngine.hpp"

#include "approach_b/EventBus.hpp"
#include "approach_b/AudioEngine.hpp"
#include "approach_b/ScoreManager.hpp"
#include "approach_b/Renderer.hpp"
#include "approach_b/PhysicsEngine.hpp"

static void BM_ApproachA_SingleCollision(benchmark::State& state) {
    approach_a::AudioEngine audio;
    approach_a::ScoreManager score;
    approach_a::Renderer renderer;
    approach_a::PhysicsEngine physics(audio, score, renderer);

    for (auto _ : state) {
        approach_a::CollisionData collision(1, approach_a::Position(100.0f, 200.0f), 100);
        physics.checkCollision(collision);
        benchmark::DoNotOptimize(collision);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ApproachA_SingleCollision);

static void BM_ApproachA_MultipleCollisions(benchmark::State& state) {
    approach_a::AudioEngine audio;
    approach_a::ScoreManager score;
    approach_a::Renderer renderer;
    approach_a::PhysicsEngine physics(audio, score, renderer);

    const int numCollisions = state.range(0);

    for (auto _ : state) {
        for (int i = 0; i < numCollisions; ++i) {
            approach_a::CollisionData collision(i, approach_a::Position(i * 10.0f, i * 20.0f), 100);
            physics.checkCollision(collision);
            benchmark::DoNotOptimize(collision);
        }
        audio.clear();
        score.reset();
        renderer.clear();
        physics.reset();
    }

    state.SetItemsProcessed(state.iterations() * numCollisions);
}
BENCHMARK(BM_ApproachA_MultipleCollisions)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_ApproachA_ComponentCreation(benchmark::State& state) {
    for (auto _ : state) {
        auto audio = std::make_unique<approach_a::AudioEngine>();
        auto score = std::make_unique<approach_a::ScoreManager>();
        auto renderer = std::make_unique<approach_a::Renderer>();
        auto physics = std::make_unique<approach_a::PhysicsEngine>(*audio, *score, *renderer);

        benchmark::DoNotOptimize(audio);
        benchmark::DoNotOptimize(score);
        benchmark::DoNotOptimize(renderer);
        benchmark::DoNotOptimize(physics);
    }
}
BENCHMARK(BM_ApproachA_ComponentCreation);

static void BM_ApproachB_SingleCollision(benchmark::State& state) {
    approach_b::EventBus eventBus;
    approach_b::AudioEngine audio(eventBus);
    approach_b::ScoreManager score(eventBus);
    approach_b::Renderer renderer(eventBus);
    approach_b::PhysicsEngine physics(eventBus);

    for (auto _ : state) {
        physics.checkCollision(1, approach_b::Position(100.0f, 200.0f), 100);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ApproachB_SingleCollision);

static void BM_ApproachB_MultipleCollisions(benchmark::State& state) {
    approach_b::EventBus eventBus;
    approach_b::AudioEngine audio(eventBus);
    approach_b::ScoreManager score(eventBus);
    approach_b::Renderer renderer(eventBus);
    approach_b::PhysicsEngine physics(eventBus);

    const int numCollisions = state.range(0);

    for (auto _ : state) {
        for (int i = 0; i < numCollisions; ++i) {
            physics.checkCollision(i, approach_b::Position(i * 10.0f, i * 20.0f), 100);
        }
        audio.clear();
        score.reset();
        renderer.clear();
        physics.reset();
        eventBus.reset();
    }

    state.SetItemsProcessed(state.iterations() * numCollisions);
}
BENCHMARK(BM_ApproachB_MultipleCollisions)->Arg(100)->Arg(1000)->Arg(10000);

static void BM_ApproachB_ComponentCreation(benchmark::State& state) {
    for (auto _ : state) {
        auto eventBus = std::make_unique<approach_b::EventBus>();
        auto audio = std::make_unique<approach_b::AudioEngine>(*eventBus);
        auto score = std::make_unique<approach_b::ScoreManager>(*eventBus);
        auto renderer = std::make_unique<approach_b::Renderer>(*eventBus);
        auto physics = std::make_unique<approach_b::PhysicsEngine>(*eventBus);

        benchmark::DoNotOptimize(eventBus);
        benchmark::DoNotOptimize(audio);
        benchmark::DoNotOptimize(score);
        benchmark::DoNotOptimize(renderer);
        benchmark::DoNotOptimize(physics);
    }
}
BENCHMARK(BM_ApproachB_ComponentCreation);

static void BM_EventBus_PublishNoSubscribers(benchmark::State& state) {
    approach_b::EventBus eventBus;

    for (auto _ : state) {
        approach_b::EnemyDestroyedEvent event(1, approach_b::Position(100, 200), 100);
        eventBus.publish(event);
        benchmark::DoNotOptimize(event);
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_EventBus_PublishNoSubscribers);

static void BM_EventBus_SubscriptionOverhead(benchmark::State& state) {
    const int numSubscribers = state.range(0);

    for (auto _ : state) {
        state.PauseTiming();
        approach_b::EventBus eventBus;
        std::vector<approach_b::EventBus::SubscriptionId> subIds;

        for (int i = 0; i < numSubscribers; ++i) {
            auto id = eventBus.subscribe<approach_b::EnemyDestroyedEvent>(
                [](const approach_b::EnemyDestroyedEvent&) {}
            );
            subIds.push_back(id);
        }
        state.ResumeTiming();

        approach_b::EnemyDestroyedEvent event(1, approach_b::Position(100, 200), 100);
        eventBus.publish(event);
        benchmark::DoNotOptimize(event);
    }
}
BENCHMARK(BM_EventBus_SubscriptionOverhead)->Arg(1)->Arg(3)->Arg(10)->Arg(50)->Arg(100);

static void BM_EventBus_SubscribeUnsubscribe(benchmark::State& state) {
    approach_b::EventBus eventBus;

    for (auto _ : state) {
        auto id = eventBus.subscribe<approach_b::EnemyDestroyedEvent>(
            [](const approach_b::EnemyDestroyedEvent&) {}
        );
        benchmark::DoNotOptimize(id);
        eventBus.unsubscribe(id);
    }
}
BENCHMARK(BM_EventBus_SubscribeUnsubscribe);

static void BM_ApproachA_MemoryFootprint(benchmark::State& state) {
    const int numEvents = state.range(0);

    for (auto _ : state) {
        approach_a::AudioEngine audio;
        approach_a::ScoreManager score;
        approach_a::Renderer renderer;
        approach_a::PhysicsEngine physics(audio, score, renderer);

        for (int i = 0; i < numEvents; ++i) {
            approach_a::CollisionData collision(i, approach_a::Position(i, i), 100);
            physics.checkCollision(collision);
        }

        benchmark::DoNotOptimize(audio);
        benchmark::DoNotOptimize(score);
        benchmark::DoNotOptimize(renderer);
    }
}
BENCHMARK(BM_ApproachA_MemoryFootprint)->Arg(1000)->Arg(10000);

static void BM_ApproachB_MemoryFootprint(benchmark::State& state) {
    const int numEvents = state.range(0);

    for (auto _ : state) {
        approach_b::EventBus eventBus;
        approach_b::AudioEngine audio(eventBus);
        approach_b::ScoreManager score(eventBus);
        approach_b::Renderer renderer(eventBus);
        approach_b::PhysicsEngine physics(eventBus);

        for (int i = 0; i < numEvents; ++i) {
            physics.checkCollision(i, approach_b::Position(i, i), 100);
        }

        benchmark::DoNotOptimize(eventBus);
        benchmark::DoNotOptimize(audio);
        benchmark::DoNotOptimize(score);
        benchmark::DoNotOptimize(renderer);
    }
}
BENCHMARK(BM_ApproachB_MemoryFootprint)->Arg(1000)->Arg(10000);

BENCHMARK_MAIN();
