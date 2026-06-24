add_executable(pathofgu-tests
    test_ecs.cpp
    test_items.cpp
    test_event_queue.cpp
    test_combat.cpp
    test_loot.cpp
    ../src/items/gu_worm_db.cpp
    ../src/systems/combat_system.cpp
    ../src/systems/effect_system.cpp
    ../src/systems/loot_system.cpp
    ../src/world/world.cpp
)
target_include_directories(pathofgu-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(pathofgu-tests PRIVATE ecs_engine Catch2::Catch2WithMain fmt::fmt spdlog::spdlog)

include(CTest)
include(Catch)
catch_discover_tests(pathofgu-tests)
