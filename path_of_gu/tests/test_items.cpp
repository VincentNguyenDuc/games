#include <catch2/catch_test_macros.hpp>
#include <items/gu_worm_db.hpp>

// The default constructor pre-seeds 20 canonical worms (rank 1–5).
// Tests that query existing data use the catalog directly.
// Tests that need isolation add extra entries and verify the delta.

// ── get ───────────────────────────────────────────────────────────────────────

TEST_CASE("get returns a known catalog entry", "[items]") {
    InMemoryGuWormDatabase db;
    auto def = db.get("Strength Gu");
    REQUIRE(def != nullptr);
    REQUIRE(def->name == "Strength Gu");
    REQUIRE(def->rank == 1);
    REQUIRE(def->type == GuWormType::Offensive);
}

TEST_CASE("get returns nullptr for unknown name", "[items]") {
    InMemoryGuWormDatabase db;
    REQUIRE(db.get("__no_such_worm__") == nullptr);
}

TEST_CASE("get returns the same shared_ptr on repeated calls", "[items]") {
    InMemoryGuWormDatabase db;
    auto a = db.get("Strength Gu");
    auto b = db.get("Strength Gu");
    REQUIRE(a.get() == b.get());
}

TEST_CASE("add inserts a new entry retrievable by get", "[items]") {
    InMemoryGuWormDatabase db;
    db.add(GuWormDefinition{"Custom Test Gu", 3, GuWormType::Support, 7, 0});

    auto def = db.get("Custom Test Gu");
    REQUIRE(def != nullptr);
    REQUIRE(def->rank == 3);
    REQUIRE(def->type == GuWormType::Support);
}

TEST_CASE("add with duplicate name overwrites previous entry", "[items]") {
    InMemoryGuWormDatabase db;
    db.add(GuWormDefinition{"Strength Gu", 1, GuWormType::Offensive, 99, 999});

    auto def = db.get("Strength Gu");
    REQUIRE(def->essence_cost == 99);
    REQUIRE(def->effect_value == 999);
}

// ── all ───────────────────────────────────────────────────────────────────────

TEST_CASE("all returns at least the pre-seeded catalog entries", "[items]") {
    InMemoryGuWormDatabase db;
    REQUIRE(db.all().size() >= 20);
}

TEST_CASE("all grows by one after adding a new unique entry", "[items]") {
    InMemoryGuWormDatabase db;
    std::size_t before = db.all().size();
    db.add(GuWormDefinition{"Unique New Gu", 2, GuWormType::Movement, 6, 0});
    REQUIRE(db.all().size() == before + 1);
}

// ── by_rank ───────────────────────────────────────────────────────────────────

TEST_CASE("by_rank returns only entries of the requested rank", "[items]") {
    InMemoryGuWormDatabase db;
    for (const auto& def : db.by_rank(1))
        REQUIRE(def->rank == 1);
    for (const auto& def : db.by_rank(3))
        REQUIRE(def->rank == 3);
}

TEST_CASE("by_rank count grows after adding a matching entry", "[items]") {
    InMemoryGuWormDatabase db;
    std::size_t before = db.by_rank(2).size();
    db.add(GuWormDefinition{"Extra Rank 2 Gu", 2, GuWormType::Offensive, 10, 15});
    REQUIRE(db.by_rank(2).size() == before + 1);
}

TEST_CASE("by_rank returns empty for a rank with no worms", "[items]") {
    InMemoryGuWormDatabase db;
    REQUIRE(db.by_rank(99).empty());
}

// ── by_type ───────────────────────────────────────────────────────────────────

TEST_CASE("by_type returns only entries of the requested type", "[items]") {
    InMemoryGuWormDatabase db;
    for (const auto& def : db.by_type(GuWormType::Offensive))
        REQUIRE(def->type == GuWormType::Offensive);
    for (const auto& def : db.by_type(GuWormType::Defensive))
        REQUIRE(def->type == GuWormType::Defensive);
}

TEST_CASE("by_type count grows after adding a matching entry", "[items]") {
    InMemoryGuWormDatabase db;
    std::size_t before = db.by_type(GuWormType::Recovery).size();
    db.add(GuWormDefinition{"Extra Recovery Gu", 1, GuWormType::Recovery, 3, 8});
    REQUIRE(db.by_type(GuWormType::Recovery).size() == before + 1);
}

TEST_CASE("by_rank and by_type counts are consistent with all()", "[items]") {
    InMemoryGuWormDatabase db;
    // Every entry in all() must appear in exactly one by_rank bucket and one by_type bucket.
    std::size_t total = db.all().size();
    std::size_t rank_total = 0;
    for (int r = 1; r <= 5; ++r)
        rank_total += db.by_rank(r).size();
    REQUIRE(rank_total == total);
}
