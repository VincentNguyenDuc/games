#include <catch2/catch_test_macros.hpp>

#include "ecs.hpp"

#include <atomic>

struct StubSystem : ISystem {
    void update(EntityComponentRegistry&, CommandBuffer&) override {}
};

static StubSystem* make(std::vector<ComponentType> reads, std::vector<ComponentType> writes) {
    auto* s = new StubSystem();
    s->reads = std::move(reads);
    s->writes = std::move(writes);
    return s;
}

using CT = ComponentType;

struct A {};
struct B {};
struct C {};
struct D {};
struct Position {
    float x, y;
};
struct Velocity {
    float dx, dy;
};

// ── Waves shape helpers ───────────────────────────────────────────────────────

static std::size_t wave_count(SystemsScheduler& sched) { return sched.get_waves().size(); }

static std::size_t wave_size(SystemsScheduler& sched, std::size_t wave) {
    return sched.get_waves()[wave].size();
}

// ── No systems ───────────────────────────────────────────────────────────────

TEST_CASE("Empty scheduler produces no waves", "[scheduler]") {
    SystemsScheduler sched;
    sched.build();
    REQUIRE(wave_count(sched) == 0);
}

// ── Single system ─────────────────────────────────────────────────────────────

TEST_CASE("Single system ends up in one wave", "[scheduler]") {
    SystemsScheduler sched;
    sched.add_system(make({CT(typeid(A))}, {CT(typeid(B))}));
    sched.build();
    REQUIRE(wave_count(sched) == 1);
    REQUIRE(wave_size(sched, 0) == 1);
}

// ── Non-conflicting systems ───────────────────────────────────────────────────

TEST_CASE("Two systems with no shared components run in the same wave", "[scheduler]") {
    SystemsScheduler sched;
    sched.add_system(make({}, {CT(typeid(A))})); // writes A
    sched.add_system(make({}, {CT(typeid(B))})); // writes B — no overlap
    sched.build();
    REQUIRE(wave_count(sched) == 1);
    REQUIRE(wave_size(sched, 0) == 2);
}

TEST_CASE("Two systems that only read the same component run in the same wave", "[scheduler]") {
    SystemsScheduler sched;
    sched.add_system(make({CT(typeid(A))}, {})); // reads A
    sched.add_system(make({CT(typeid(A))}, {})); // reads A — readers never conflict
    sched.build();
    REQUIRE(wave_count(sched) == 1);
    REQUIRE(wave_size(sched, 0) == 2);
}

// ── Conflicting systems ───────────────────────────────────────────────────────

TEST_CASE("Writer then reader produces two waves", "[scheduler]") {
    SystemsScheduler sched;
    sched.add_system(make({}, {CT(typeid(A))})); // writes A
    sched.add_system(make({CT(typeid(A))}, {})); // reads A — depends on writer
    sched.build();
    REQUIRE(wave_count(sched) == 2);
    REQUIRE(wave_size(sched, 0) == 1);
    REQUIRE(wave_size(sched, 1) == 1);
}

TEST_CASE("Two writers of the same component produce two waves", "[scheduler]") {
    SystemsScheduler sched;
    sched.add_system(make({}, {CT(typeid(A))})); // writes A
    sched.add_system(make({}, {CT(typeid(A))})); // writes A — conflict
    sched.build();
    REQUIRE(wave_count(sched) == 2);
}

TEST_CASE("Reader then writer produces two waves", "[scheduler]") {
    SystemsScheduler sched;
    sched.add_system(make({CT(typeid(A))}, {})); // reads A  (registered first)
    sched.add_system(make({}, {CT(typeid(A))})); // writes A (registered second)
    sched.build();
    REQUIRE(wave_count(sched) == 2);
}

// ── Chains ────────────────────────────────────────────────────────────────────

TEST_CASE("Linear chain A->B->C produces three waves", "[scheduler]") {
    SystemsScheduler sched;
    sched.add_system(make({}, {CT(typeid(A))}));
    sched.add_system(make({CT(typeid(A))}, {CT(typeid(B))}));
    sched.add_system(make({CT(typeid(B))}, {}));
    sched.build();
    REQUIRE(wave_count(sched) == 3);
    REQUIRE(wave_size(sched, 0) == 1);
    REQUIRE(wave_size(sched, 1) == 1);
    REQUIRE(wave_size(sched, 2) == 1);
}

// ── Diamond ───────────────────────────────────────────────────────────────────

TEST_CASE("Diamond: independent middle systems run in parallel", "[scheduler]") {
    SystemsScheduler sched;
    sched.add_system(make({}, {CT(typeid(A))}));                // S0
    sched.add_system(make({CT(typeid(A))}, {CT(typeid(B))}));   // S1
    sched.add_system(make({CT(typeid(A))}, {CT(typeid(C))}));   // S2
    sched.add_system(make({CT(typeid(B)), CT(typeid(C))}, {})); // S3
    sched.build();
    REQUIRE(wave_count(sched) == 3);
    REQUIRE(wave_size(sched, 0) == 1); // S0
    REQUIRE(wave_size(sched, 1) == 2); // S1 and S2 in parallel
    REQUIRE(wave_size(sched, 2) == 1); // S3
}

// ── World integration ─────────────────────────────────────────────────────────

struct CountingSystem : ISystem {
    std::atomic<int>* counter;
    explicit CountingSystem(std::atomic<int>* c)
        : counter(c) {
        writes = {CT(typeid(Position))};
    }
    void update(EntityComponentRegistry& reg, CommandBuffer&) override {
        for (Entity e : reg.view<Position>()) {
            auto* p = reg.getComponent<Position>(e);
            if (p) {
                p->x += 1.0f;
            }
        }
        ++(*counter);
    }
};

TEST_CASE("Engine::tick runs all systems and mutates components", "[engine]") {
    std::atomic<int> ticks{0};
    Engine engine;
    engine.add_system<CountingSystem>(&ticks);
    engine.build();

    Entity e = engine.entities().createEntity();
    engine.registry().addComponent(e, Position{0.0f, 0.0f});

    engine.tick();
    engine.tick();

    REQUIRE(ticks.load() == 2);
    REQUIRE(engine.registry().getComponent<Position>(e)->x == 2.0f);
}

TEST_CASE("CommandBuffer deferred add_component is visible after flush", "[engine]") {
    struct SpawnSystem : ISystem {
        Entity target;
        explicit SpawnSystem(Entity e)
            : target(e) {
            writes = {CT(typeid(Velocity))};
        }
        void update(EntityComponentRegistry&, CommandBuffer& cmd) override {
            cmd.add_component(target, Velocity{3.0f, 4.0f});
        }
    };

    Engine engine;
    Entity e = engine.entities().createEntity();
    engine.add_system<SpawnSystem>(e);
    engine.build();

    REQUIRE(engine.registry().getComponent<Velocity>(e) == nullptr);
    engine.tick(); // SpawnSystem queues the add; flush applies it
    REQUIRE(engine.registry().getComponent<Velocity>(e) != nullptr);
    REQUIRE(engine.registry().getComponent<Velocity>(e)->dx == 3.0f);
}
