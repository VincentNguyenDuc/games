#include <catch2/catch_test_macros.hpp>

#include "components/aperture.hpp"
#include "components/health.hpp"
#include "components/name.hpp"
#include "components/primeval_essence.hpp"
#include "components/stats.hpp"
#include "ecs/entity.hpp"
#include "ecs/registry.hpp"
#include "items/gu_worm.hpp"
#include "systems/combat_system.hpp"

#include <memory>

// ── helpers ───────────────────────────────────────────────────────────────────

static GuWorm make_worm(
    const std::string& name, GuWormType type, int essence_cost, int effect_value, int rank = 1
) {
    auto def = std::make_shared<GuWormDefinition>(GuWormDefinition{
        name, rank, type, essence_cost, effect_value});
    return GuWorm{def};
}

static Entity make_player(
    EntityComponentRegistry& reg, EntityManager& em, int hp = 100, int essence = 50
) {
    Entity p = em.createEntity();
    reg.addComponent(p, Health{hp, hp});
    reg.addComponent(p, PrimevalEssence{essence, essence, 0});
    reg.addComponent(p, Aperture{{}, 9});
    reg.addComponent(p, Name{"Gu Master"});
    return p;
}

static Entity make_enemy(
    EntityComponentRegistry& reg, EntityManager& em, int hp = 30, int defense = 0
) {
    Entity e = em.createEntity();
    reg.addComponent(e, Health{hp, hp});
    reg.addComponent(e, Name{"Wild Gu Worm Beast"});
    if (defense > 0)
        reg.addComponent(e, Stats{5, defense});
    return e;
}

// ── offensive worm ────────────────────────────────────────────────────────────

TEST_CASE("offensive worm deals damage and costs essence", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);
    Entity enemy = make_enemy(reg, em, 30);

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Strength Gu", GuWormType::Offensive, 10, 20)
    );

    auto result = player_attack(reg, player, enemy, 0);
    REQUIRE(result.success);
    REQUIRE(reg.getComponent<Health>(enemy)->hp < 30);
    REQUIRE(reg.getComponent<PrimevalEssence>(player)->current == 40);
}

TEST_CASE("offensive worm respects enemy defense", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);
    Entity enemy = make_enemy(reg, em, 30, 15); // 15 defense

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Strength Gu", GuWormType::Offensive, 5, 20)
    );

    auto result = player_attack(reg, player, enemy, 0);
    REQUIRE(result.success);
    // damage = max(1, 20 - 15) = 5
    REQUIRE(reg.getComponent<Health>(enemy)->hp == 25);
}

TEST_CASE("high defense still lets minimum 1 damage through", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);
    Entity enemy = make_enemy(reg, em, 30, 100); // impenetrable defense

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Weak Gu", GuWormType::Offensive, 2, 5)
    );

    auto result = player_attack(reg, player, enemy, 0);
    REQUIRE(result.success);
    REQUIRE(reg.getComponent<Health>(enemy)->hp == 29); // 1 damage minimum
}

// ── essence gating ────────────────────────────────────────────────────────────

TEST_CASE("attack fails when essence is insufficient", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 5); // only 5 essence
    Entity enemy = make_enemy(reg, em, 30);

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Expensive Gu", GuWormType::Offensive, 10, 20) // costs 10
    );

    auto result = player_attack(reg, player, enemy, 0);
    REQUIRE(!result.success);
    REQUIRE(reg.getComponent<Health>(enemy)->hp == 30);               // no damage
    REQUIRE(reg.getComponent<PrimevalEssence>(player)->current == 5); // no spend
}

// ── slot validation ───────────────────────────────────────────────────────────

TEST_CASE("attack fails on invalid worm slot", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em);
    Entity enemy = make_enemy(reg, em);

    auto result = player_attack(reg, player, enemy, 0);
    REQUIRE(!result.success);
}

TEST_CASE("attack fails on negative worm slot", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em);
    Entity enemy = make_enemy(reg, em);

    reg.getComponent<Aperture>(player)->worms.push_back(make_worm("Gu", GuWormType::Offensive, 2, 5)
    );

    auto result = player_attack(reg, player, enemy, -1);
    REQUIRE(!result.success);
}

// ── recovery worm ─────────────────────────────────────────────────────────────

TEST_CASE("recovery worm heals the player", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);
    Entity enemy = make_enemy(reg, em, 30);

    reg.getComponent<Health>(player)->hp = 60; // player took 40 damage
    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Healing Gu", GuWormType::Recovery, 5, 30)
    );

    auto result = player_attack(reg, player, enemy, 0);
    REQUIRE(result.success);
    REQUIRE(reg.getComponent<Health>(player)->hp == 90); // healed 30
}

TEST_CASE("recovery worm does not exceed max HP", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);
    Entity enemy = make_enemy(reg, em, 30);

    reg.getComponent<Health>(player)->hp = 95;
    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Healing Gu", GuWormType::Recovery, 5, 30)
    );

    auto result = player_attack(reg, player, enemy, 0);
    REQUIRE(result.success);
    REQUIRE(reg.getComponent<Health>(player)->hp == 100); // capped at max
}

// ── death detection ───────────────────────────────────────────────────────────

TEST_CASE("target_died is set when enemy HP drops to zero", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);
    Entity enemy = make_enemy(reg, em, 5); // very low HP

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Kill Shot Gu", GuWormType::Offensive, 5, 100)
    );

    auto result = player_attack(reg, player, enemy, 0);
    REQUIRE(result.success);
    REQUIRE(result.target_died);
}

TEST_CASE("target_died is false when enemy survives", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);
    Entity enemy = make_enemy(reg, em, 100);

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Weak Gu", GuWormType::Offensive, 5, 10)
    );

    auto result = player_attack(reg, player, enemy, 0);
    REQUIRE(result.success);
    REQUIRE(!result.target_died);
}
