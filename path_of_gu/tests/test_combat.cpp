#include "actions/combat.hpp"
#include "components/aperture.hpp"
#include "components/health.hpp"
#include "components/name.hpp"
#include "components/position.hpp"
#include "components/primeval_essence.hpp"
#include "components/stats.hpp"
#include "ecs.hpp"
#include "items/gu_worm.hpp"
#include "systems/effect_system.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>

using namespace ecse;

// ── helpers ───────────────────────────────────────────────────────────────────

static GuWorm make_worm(
    const std::string& name,
    GuWormType type,
    int essence_cost,
    int effect_value,
    int rank = 1,
    int range = 1
) {
    auto def = std::make_shared<GuWormDefinition>(GuWormDefinition{
        name, rank, type, essence_cost, effect_value, range});
    return GuWorm{def};
}

// Player at (5,5)
static Entity make_player(
    EntityComponentRegistry& reg,
    EntityManager& em,
    int hp = 100,
    int essence = 50,
    int x = 5,
    int y = 5
) {
    Entity p = em.createEntity();
    reg.addComponent(p, Health{hp, hp});
    reg.addComponent(p, PrimevalEssence{essence, essence, 0});
    reg.addComponent(p, Aperture{{}, 9});
    reg.addComponent(p, Name{"Gu Master"});
    reg.addComponent(p, Position{0, x, y});
    return p;
}

// Enemy at (5,6) — Chebyshev distance 1 from player (5,5)
static Entity make_enemy(
    EntityComponentRegistry& reg,
    EntityManager& em,
    int hp = 30,
    int defense = 0,
    int x = 5,
    int y = 6
) {
    Entity e = em.createEntity();
    reg.addComponent(e, Health{hp, hp});
    reg.addComponent(e, Name{"Wild Gu Worm Beast"});
    if (defense > 0)
        reg.addComponent(e, Stats{5, defense});
    reg.addComponent(e, Position{0, x, y});
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

    auto result = activate_worm(reg, player, enemy, 0);
    REQUIRE(result.success);
    REQUIRE(reg.getComponent<PrimevalEssence>(player)->current == 40); // essence spent immediately
    AttackEffectTickSystem::resolve(reg);
    REQUIRE(reg.getComponent<Health>(enemy)->hp < 30);
}

TEST_CASE("offensive worm respects enemy defense", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);
    Entity enemy = make_enemy(reg, em, 30, 15); // 15 defense

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Strength Gu", GuWormType::Offensive, 5, 20)
    );

    activate_worm(reg, player, enemy, 0);
    AttackEffectTickSystem::resolve(reg);
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

    activate_worm(reg, player, enemy, 0);
    AttackEffectTickSystem::resolve(reg);
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

    auto result = activate_worm(reg, player, enemy, 0);
    REQUIRE(!result.success);
    AttackEffectTickSystem::resolve(reg);
    REQUIRE(reg.getComponent<Health>(enemy)->hp == 30);               // no damage
    REQUIRE(reg.getComponent<PrimevalEssence>(player)->current == 5); // no spend
}

// ── slot validation ───────────────────────────────────────────────────────────

TEST_CASE("attack fails on invalid worm slot", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em);
    Entity enemy = make_enemy(reg, em);

    auto result = activate_worm(reg, player, enemy, 0);
    REQUIRE(!result.success);
}

TEST_CASE("attack fails on negative worm slot", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em);
    Entity enemy = make_enemy(reg, em);

    reg.getComponent<Aperture>(player)->worms.push_back(make_worm("Gu", GuWormType::Offensive, 2, 5)
    );

    auto result = activate_worm(reg, player, enemy, -1);
    REQUIRE(!result.success);
}

// ── recovery worm ─────────────────────────────────────────────────────────────

TEST_CASE("recovery worm heals the user", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);

    reg.getComponent<Health>(player)->hp = 60; // took 40 damage
    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Healing Gu", GuWormType::Recovery, 5, 30, 1, /*range=*/0)
    );

    auto result = activate_worm(reg, player, player, 0); // self-target
    REQUIRE(result.success);
    SelfEffectTickSystem::resolve(reg);
    REQUIRE(reg.getComponent<Health>(player)->hp == 90); // healed 30
}

TEST_CASE("recovery worm does not exceed max HP", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);

    reg.getComponent<Health>(player)->hp = 95;
    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Healing Gu", GuWormType::Recovery, 5, 30, 1, /*range=*/0)
    );

    activate_worm(reg, player, player, 0);
    SelfEffectTickSystem::resolve(reg);
    REQUIRE(reg.getComponent<Health>(player)->hp == 100); // capped at max
}

// ── death detection ───────────────────────────────────────────────────────────

TEST_CASE("enemy HP drops to zero after lethal attack", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);
    Entity enemy = make_enemy(reg, em, 5); // very low HP

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Kill Shot Gu", GuWormType::Offensive, 5, 100)
    );

    activate_worm(reg, player, enemy, 0);
    AttackEffectTickSystem::resolve(reg);
    REQUIRE(reg.getComponent<Health>(enemy)->hp <= 0);
}

TEST_CASE("enemy survives a weak attack", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);
    Entity enemy = make_enemy(reg, em, 100);

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Weak Gu", GuWormType::Offensive, 5, 10)
    );

    activate_worm(reg, player, enemy, 0);
    AttackEffectTickSystem::resolve(reg);
    REQUIRE(reg.getComponent<Health>(enemy)->hp > 0);
}

// ── symmetry: enemy attacks player ───────────────────────────────────────────

TEST_CASE("enemy can activate worm against player symmetrically", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50);
    Entity enemy = make_enemy(reg, em, 30);

    // Give the enemy an aperture and essence
    reg.addComponent(enemy, PrimevalEssence{30, 30, 0});
    reg.addComponent(enemy, Aperture{{make_worm("Strength Gu", GuWormType::Offensive, 5, 15)}, 1});

    auto result = activate_worm(reg, enemy, player, 0);
    REQUIRE(result.success);
    AttackEffectTickSystem::resolve(reg);
    REQUIRE(reg.getComponent<Health>(player)->hp < 100); // player took damage
}

// ── range checks ──────────────────────────────────────────────────────────────

TEST_CASE("melee worm hits adjacent enemy (distance 1)", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50, 5, 5);
    Entity enemy = make_enemy(reg, em, 30, 0, 5, 6);

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Strength Gu", GuWormType::Offensive, 5, 10, 1, /*range=*/1)
    );

    auto result = activate_worm(reg, player, enemy, 0);
    REQUIRE(result.success);
    AttackEffectTickSystem::resolve(reg);
    REQUIRE(reg.getComponent<Health>(enemy)->hp < 30);
}

TEST_CASE("melee worm fails when enemy is out of range", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50, 1, 1);
    Entity enemy = make_enemy(reg, em, 30, 0, 7, 7);

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Strength Gu", GuWormType::Offensive, 5, 10, 1, /*range=*/1)
    );

    auto result = activate_worm(reg, player, enemy, 0);
    REQUIRE(!result.success);
    REQUIRE(reg.getComponent<Health>(enemy)->hp == 30);                // no damage
    REQUIRE(reg.getComponent<PrimevalEssence>(player)->current == 50); // no spend
}

TEST_CASE("ranged worm hits enemy at distance within its range", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50, 5, 5);
    Entity enemy = make_enemy(reg, em, 30, 0, 5, 8);

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Lightning Gu", GuWormType::Offensive, 12, 20, 2, /*range=*/3)
    );

    auto result = activate_worm(reg, player, enemy, 0);
    REQUIRE(result.success);
    AttackEffectTickSystem::resolve(reg);
    REQUIRE(reg.getComponent<Health>(enemy)->hp < 30);
}

TEST_CASE("ranged worm fails beyond its own range", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50, 1, 1);
    Entity enemy = make_enemy(reg, em, 30, 0, 8, 8);

    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Lightning Gu", GuWormType::Offensive, 12, 20, 2, /*range=*/3)
    );

    auto result = activate_worm(reg, player, enemy, 0);
    REQUIRE(!result.success);
}

TEST_CASE("recovery worm ignores distance (range 0 = self only)", "[combat]") {
    EntityComponentRegistry reg;
    EntityManager em;
    Entity player = make_player(reg, em, 100, 50, 1, 1);

    reg.getComponent<Health>(player)->hp = 60;
    reg.getComponent<Aperture>(player)->worms.push_back(
        make_worm("Liquor Worm", GuWormType::Recovery, 4, 20, 1, /*range=*/0)
    );

    auto result = activate_worm(reg, player, player, 0);
    REQUIRE(result.success);
    SelfEffectTickSystem::resolve(reg);
    REQUIRE(reg.getComponent<Health>(player)->hp == 80); // healed 20
}
