add_executable(tic_tac_toe-tests test.cpp ../src/state.cpp)
target_include_directories(tic_tac_toe-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(tic_tac_toe-tests PRIVATE Catch2::Catch2WithMain fmt::fmt)

include(CTest)
include(Catch)
catch_discover_tests(tic_tac_toe-tests)
