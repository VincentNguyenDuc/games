add_executable(tictactoe src/main.cpp src/state.cpp)
target_include_directories(tictactoe PRIVATE include)
target_link_libraries(tictactoe PRIVATE fmt::fmt)

add_subdirectory(tests)
