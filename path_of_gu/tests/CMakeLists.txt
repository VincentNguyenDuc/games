add_executable(path_of_gu-tests
    test_ecs.cpp
    test_items.cpp
    test_combat.cpp
    test_loot.cpp
    ../src/items/gu_worm_db.cpp
    ../src/actions/combat.cpp
    ../src/actions/loot.cpp
    ../src/systems/effect_system.cpp
    ../src/world/world.cpp
)
target_include_directories(path_of_gu-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(path_of_gu-tests PRIVATE ecs_engine Catch2::Catch2WithMain fmt::fmt spdlog::spdlog)

include(CTest)
include(Catch)
catch_discover_tests(path_of_gu-tests)
