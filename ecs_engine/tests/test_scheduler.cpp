#include <catch2/catch_test_macros.hpp>

#include "ecs.hpp"

// Minimal concrete system — never actually called, just carries access metadata.
struct StubSystem : ISystem {
    void update(EntityComponentRegistry&) override {}
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
    sched.add_system(make({}, {CT(typeid(A))})); // writes A (registered second — depends on reader)
    sched.build();
    REQUIRE(wave_count(sched) == 2);
}

// ── Chains ────────────────────────────────────────────────────────────────────

TEST_CASE("Linear chain A->B->C produces three waves", "[scheduler]") {
    // S0 writes A, S1 reads A + writes B, S2 reads B
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
    // S0 writes A
    // S1 reads A, writes B   (depends on S0)
    // S2 reads A, writes C   (depends on S0)
    // S3 reads B + C          (depends on S1 and S2)
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
