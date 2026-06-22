#include <catch2/catch_test_macros.hpp>

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"

TEST_CASE("Entity ID is unique") {
    EntityManager em;
    Entity e1 = em.createEntity();
    Entity e2 = em.createEntity();
    REQUIRE(e1 != e2);
}

TEST_CASE("Entity recycling works") {
    EntityManager em;
    Entity e1 = em.createEntity();
    em.destroyEntity(e1);
    Entity e2 = em.createEntity();
    REQUIRE(e1 == e2);
}

TEST_CASE("Entity limit is enforced") {
    EntityManager em;
    for (std::uint32_t i = 0; i < ENTITY_LIMIT; ++i) {
        em.createEntity();
    }
    REQUIRE_THROWS_AS(em.createEntity(), std::runtime_error);
}

TEST_CASE("EntityComponentRegistry can add and retrieve components") {
    EntityComponentRegistry registry;
    EntityManager em;
    Entity e = em.createEntity();

    struct Position {
        float x, y;
    };
    Position pos{1.0f, 2.0f};

    registry.addComponent<Position>(e, pos);
    Position* retrievedPos = registry.getComponent<Position>(e);
    REQUIRE(retrievedPos != nullptr);
    REQUIRE(retrievedPos->x == 1.0f);
    REQUIRE(retrievedPos->y == 2.0f);
}

TEST_CASE("EntityComponentRegistry can remove components") {
    EntityComponentRegistry registry;
    EntityManager em;
    Entity e = em.createEntity();

    struct Position {
        float x, y;
    };
    Position pos{1.0f, 2.0f};

    registry.addComponent<Position>(e, pos);
    registry.removeComponent<Position>(e);
    Position* retrievedPos = registry.getComponent<Position>(e);
    REQUIRE(retrievedPos == nullptr);
}
