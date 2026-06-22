add_executable(tictactoe-tests test.cpp ../src/state.cpp)
target_include_directories(tictactoe-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(tictactoe-tests PRIVATE Catch2::Catch2WithMain fmt::fmt)

include(CTest)
include(Catch)
catch_discover_tests(tictactoe-tests)
