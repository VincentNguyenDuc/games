#include <catch2/catch_test_macros.hpp>

#include <items/gu_worm_db.hpp>

TEST_CASE("InMemoryGuWormDatabase returns correct worm definition") {
    InMemoryGuWormDatabase db;

    db.add(GuWormDefinition{"test_worm", 3, GuWormType::Offensive, 10, 50});

    auto wormDef = db.get("test_worm");
    REQUIRE(wormDef != nullptr);
    REQUIRE(db.get("_nonexistent_worm") == nullptr);
}