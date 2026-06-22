add_executable(pathofgu-tests test_ecs.cpp ../src/ecs/entity.cpp)
target_include_directories(pathofgu-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(pathofgu-tests PRIVATE Catch2::Catch2WithMain fmt::fmt spdlog::spdlog)

include(CTest)
include(Catch)
catch_discover_tests(pathofgu-tests)
