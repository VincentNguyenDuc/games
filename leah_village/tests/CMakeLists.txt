add_executable(leah_village-tests test.cpp)
target_include_directories(leah_village-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(leah_village-tests PRIVATE Catch2::Catch2WithMain fmt::fmt spdlog::spdlog)

include(CTest)
include(Catch)
catch_discover_tests(leah_village-tests)
