#include <catch2/catch_test_macros.hpp>

#include "ecs.hpp"

struct Pos {
    float x, y;
};
struct Speed {
    float value;
};

// ── EntityManager ────────────────────────────────────────────────────────────

TEST_CASE("Entity IDs are unique", "[ecs]") {
    EntityManager em;
    REQUIRE(em.createEntity() != em.createEntity());
}

TEST_CASE("Destroyed entity ID is recycled", "[ecs]") {
    EntityManager em;
    Entity e1 = em.createEntity();
    em.destroyEntity(e1);
    REQUIRE(em.createEntity() == e1);
}

TEST_CASE("Entity limit is enforced", "[ecs]") {
    EntityManager em;
    for (std::uint32_t i = 0; i < ENTITY_LIMIT; ++i)
        em.createEntity();
    REQUIRE_THROWS_AS(em.createEntity(), std::runtime_error);
}

// ── Registry: add / get / remove ─────────────────────────────────────────────

TEST_CASE("addComponent and getComponent round-trip", "[ecs]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity e = em.createEntity();

    reg.addComponent(e, Pos{3.0f, 4.0f});
    auto* p = reg.getComponent<Pos>(e);

    REQUIRE(p != nullptr);
    REQUIRE(p->x == 3.0f);
    REQUIRE(p->y == 4.0f);
}

TEST_CASE("getComponent returns nullptr for missing entity", "[ecs]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity e = em.createEntity();

    REQUIRE(reg.getComponent<Pos>(e) == nullptr);
}

TEST_CASE("getComponent returns nullptr for wrong component type", "[ecs]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity e = em.createEntity();

    reg.addComponent(e, Pos{1.0f, 2.0f});
    REQUIRE(reg.getComponent<Speed>(e) == nullptr);
}

TEST_CASE("removeComponent makes component inaccessible", "[ecs]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity e = em.createEntity();

    reg.addComponent(e, Pos{1.0f, 2.0f});
    reg.removeComponent<Pos>(e);

    REQUIRE(reg.getComponent<Pos>(e) == nullptr);
}

TEST_CASE("removeComponent on missing component is a no-op", "[ecs]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity e = em.createEntity();
    REQUIRE_NOTHROW(reg.removeComponent<Pos>(e));
}

TEST_CASE("Multiple component types on the same entity are independent", "[ecs]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity e = em.createEntity();

    reg.addComponent(e, Pos{1.0f, 2.0f});
    reg.addComponent(e, Speed{5.0f});

    REQUIRE(reg.getComponent<Pos>(e)->x == 1.0f);
    REQUIRE(reg.getComponent<Speed>(e)->value == 5.0f);

    reg.removeComponent<Pos>(e);
    REQUIRE(reg.getComponent<Pos>(e) == nullptr);
    REQUIRE(reg.getComponent<Speed>(e) != nullptr);
}

TEST_CASE("addComponent overwrites an existing component", "[ecs]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity e = em.createEntity();

    reg.addComponent(e, Pos{1.0f, 2.0f});
    reg.addComponent(e, Pos{9.0f, 8.0f});

    REQUIRE(reg.getComponent<Pos>(e)->x == 9.0f);
}

TEST_CASE("getComponent mutation is reflected on next access", "[ecs]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity e = em.createEntity();

    reg.addComponent(e, Pos{0.0f, 0.0f});
    reg.getComponent<Pos>(e)->x = 7.0f;

    REQUIRE(reg.getComponent<Pos>(e)->x == 7.0f);
}

// ── Registry: view ───────────────────────────────────────────────────────────

TEST_CASE("view returns all entities with a given component", "[ecs]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity e1 = em.createEntity();
    Entity e2 = em.createEntity();
    Entity e3 = em.createEntity();

    reg.addComponent(e1, Pos{0, 0});
    reg.addComponent(e2, Pos{1, 1});
    reg.addComponent(e3, Speed{2.0f});

    REQUIRE(reg.view<Pos>().size() == 2);
}

TEST_CASE("view returns empty vector when no entities have the component", "[ecs]") {
    EntityComponentRegistry reg;
    REQUIRE(reg.view<Pos>().empty());
}
