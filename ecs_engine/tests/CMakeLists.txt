add_executable(ecs_engine-tests
    test_scheduler.cpp
)
target_link_libraries(ecs_engine-tests PRIVATE ecs_engine Catch2::Catch2WithMain spdlog::spdlog)

include(CTest)
include(Catch)
catch_discover_tests(ecs_engine-tests)
