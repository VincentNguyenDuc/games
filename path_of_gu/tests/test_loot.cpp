#include <catch2/catch_test_macros.hpp>

#include "actions/loot.hpp"
#include "components/aperture.hpp"
#include "components/loot.hpp"
#include "components/name.hpp"
#include "components/position.hpp"
#include "ecs.hpp"
#include "items/gu_worm.hpp"
#include "world/world.hpp"

#include <memory>

// ── helpers ───────────────────────────────────────────────────────────────────

static GuWorm make_worm(const std::string& name) {
    auto def =
        std::make_shared<GuWormDefinition>(GuWormDefinition{name, 1, GuWormType::Offensive, 5, 10});
    return GuWorm{def};
}

static GuWormDrop always_drop(const std::string& name) {
    auto def =
        std::make_shared<GuWormDefinition>(GuWormDefinition{name, 1, GuWormType::Offensive, 5, 10});
    return GuWormDrop{def, 1.0f}; // 100% drop chance
}

static GuWormDrop never_drop(const std::string& name) {
    auto def =
        std::make_shared<GuWormDefinition>(GuWormDefinition{name, 1, GuWormType::Offensive, 5, 10});
    return GuWormDrop{def, 0.0f}; // 0% drop chance
}

// ── process_death ─────────────────────────────────────────────────────────────

TEST_CASE("process_death drops loot with 100% chance onto the map", "[loot]") {
    EntityComponentRegistry reg;
    EntityManager em;
    GameWorld world;

    Entity enemy = em.createEntity();
    reg.addComponent(enemy, Position{world.entrance_id()});
    reg.addComponent(enemy, Loot{{always_drop("Strength Gu")}});

    process_death(reg, world, enemy);

    auto* map = world.get_map(world.entrance_id());
    REQUIRE(map->dropped_worms.size() == 1);
    REQUIRE(map->dropped_worms[0].def->name == "Strength Gu");
}

TEST_CASE("process_death does not drop loot with 0% chance", "[loot]") {
    EntityComponentRegistry reg;
    EntityManager em;
    GameWorld world;

    Entity enemy = em.createEntity();
    reg.addComponent(enemy, Position{world.entrance_id()});
    reg.addComponent(enemy, Loot{{never_drop("Rare Gu")}});

    process_death(reg, world, enemy);

    auto* map = world.get_map(world.entrance_id());
    REQUIRE(map->dropped_worms.empty());
}

TEST_CASE("process_death with multiple guaranteed drops adds all", "[loot]") {
    EntityComponentRegistry reg;
    EntityManager em;
    GameWorld world;

    Entity enemy = em.createEntity();
    reg.addComponent(enemy, Position{world.entrance_id()});
    reg.addComponent(enemy, Loot{{always_drop("Gu A"), always_drop("Gu B"), always_drop("Gu C")}});

    process_death(reg, world, enemy);

    auto* map = world.get_map(world.entrance_id());
    REQUIRE(map->dropped_worms.size() == 3);
}

TEST_CASE("process_death on entity without Loot component is a no-op", "[loot]") {
    EntityComponentRegistry reg;
    EntityManager em;
    GameWorld world;

    Entity enemy = em.createEntity();
    reg.addComponent(enemy, Position{world.entrance_id()});
    // no Loot component

    REQUIRE_NOTHROW(process_death(reg, world, enemy));
    REQUIRE(world.get_map(world.entrance_id())->dropped_worms.empty());
}

// ── pickup_worm ───────────────────────────────────────────────────────────────

TEST_CASE("pickup_worm moves a worm from map to player aperture", "[loot]") {
    EntityComponentRegistry reg;
    EntityManager em;
    GameWorld world;

    Entity player = em.createEntity();
    reg.addComponent(player, Position{world.entrance_id()});
    reg.addComponent(player, Aperture{{}, 9});

    auto* map = world.get_map(world.entrance_id());
    map->dropped_worms.push_back(make_worm("Iron Skin Gu"));

    bool ok = pickup_worm(reg, world, player, 0);
    REQUIRE(ok);
    REQUIRE(reg.getComponent<Aperture>(player)->worms.size() == 1);
    REQUIRE(reg.getComponent<Aperture>(player)->worms[0].def->name == "Iron Skin Gu");
    REQUIRE(map->dropped_worms.empty());
}

TEST_CASE("pickup_worm fails when aperture is full", "[loot]") {
    EntityComponentRegistry reg;
    EntityManager em;
    GameWorld world;

    Entity player = em.createEntity();
    reg.addComponent(player, Position{world.entrance_id()});
    // capacity 1, already full
    reg.addComponent(player, Aperture{{make_worm("Existing Gu")}, 1});

    auto* map = world.get_map(world.entrance_id());
    map->dropped_worms.push_back(make_worm("New Gu"));

    bool ok = pickup_worm(reg, world, player, 0);
    REQUIRE(!ok);
    REQUIRE(reg.getComponent<Aperture>(player)->worms.size() == 1); // unchanged
    REQUIRE(map->dropped_worms.size() == 1);                        // still on map
}

TEST_CASE("pickup_worm fails on out-of-range index", "[loot]") {
    EntityComponentRegistry reg;
    EntityManager em;
    GameWorld world;

    Entity player = em.createEntity();
    reg.addComponent(player, Position{world.entrance_id()});
    reg.addComponent(player, Aperture{{}, 9});

    auto* map = world.get_map(world.entrance_id());
    map->dropped_worms.push_back(make_worm("Gu"));

    REQUIRE(!pickup_worm(reg, world, player, 5));  // index 5 out of range
    REQUIRE(!pickup_worm(reg, world, player, -1)); // negative index
}

TEST_CASE("pickup_worm removes correct worm when multiple drops exist", "[loot]") {
    EntityComponentRegistry reg;
    EntityManager em;
    GameWorld world;

    Entity player = em.createEntity();
    reg.addComponent(player, Position{world.entrance_id()});
    reg.addComponent(player, Aperture{{}, 9});

    auto* map = world.get_map(world.entrance_id());
    map->dropped_worms.push_back(make_worm("Gu A"));
    map->dropped_worms.push_back(make_worm("Gu B"));
    map->dropped_worms.push_back(make_worm("Gu C"));

    bool ok = pickup_worm(reg, world, player, 1); // pick up index 1 ("Gu B")
    REQUIRE(ok);
    REQUIRE(reg.getComponent<Aperture>(player)->worms[0].def->name == "Gu B");
    REQUIRE(map->dropped_worms.size() == 2);
    REQUIRE(map->dropped_worms[0].def->name == "Gu A");
    REQUIRE(map->dropped_worms[1].def->name == "Gu C");
}

// ── drop_worm ─────────────────────────────────────────────────────────────────

TEST_CASE("drop_worm moves worm from aperture to map", "[loot]") {
    EntityComponentRegistry reg;
    EntityManager em;
    GameWorld world;

    Entity player = em.createEntity();
    reg.addComponent(player, Position{world.entrance_id()});
    reg.addComponent(player, Aperture{{make_worm("Strength Gu"), make_worm("Iron Skin Gu")}, 9});

    bool ok = drop_worm(reg, world, player, 0);
    REQUIRE(ok);
    REQUIRE(reg.getComponent<Aperture>(player)->worms.size() == 1);
    REQUIRE(reg.getComponent<Aperture>(player)->worms[0].def->name == "Iron Skin Gu");
    REQUIRE(world.get_map(world.entrance_id())->dropped_worms.size() == 1);
    REQUIRE(world.get_map(world.entrance_id())->dropped_worms[0].def->name == "Strength Gu");
}

TEST_CASE("drop_worm fails on out-of-range slot", "[loot]") {
    EntityComponentRegistry reg;
    EntityManager em;
    GameWorld world;

    Entity player = em.createEntity();
    reg.addComponent(player, Position{world.entrance_id()});
    reg.addComponent(player, Aperture{{make_worm("Strength Gu")}, 9});

    REQUIRE(!drop_worm(reg, world, player, 1)); // only slot 0 exists
    REQUIRE(!drop_worm(reg, world, player, -1));
    REQUIRE(reg.getComponent<Aperture>(player)->worms.size() == 1); // unchanged
}

TEST_CASE("drop_worm on empty aperture fails", "[loot]") {
    EntityComponentRegistry reg;
    EntityManager em;
    GameWorld world;

    Entity player = em.createEntity();
    reg.addComponent(player, Position{world.entrance_id()});
    reg.addComponent(player, Aperture{{}, 9});

    REQUIRE(!drop_worm(reg, world, player, 0));
    REQUIRE(world.get_map(world.entrance_id())->dropped_worms.empty());
}

TEST_CASE("dropped worm can be picked back up", "[loot]") {
    EntityComponentRegistry reg;
    EntityManager em;
    GameWorld world;

    Entity player = em.createEntity();
    reg.addComponent(player, Position{world.entrance_id()});
    reg.addComponent(player, Aperture{{make_worm("Strength Gu")}, 9});

    REQUIRE(drop_worm(reg, world, player, 0));
    REQUIRE(reg.getComponent<Aperture>(player)->worms.empty());

    REQUIRE(pickup_worm(reg, world, player, 0));
    REQUIRE(reg.getComponent<Aperture>(player)->worms.size() == 1);
    REQUIRE(reg.getComponent<Aperture>(player)->worms[0].def->name == "Strength Gu");
}
