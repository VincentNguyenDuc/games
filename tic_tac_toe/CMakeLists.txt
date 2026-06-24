add_executable(tic_tac_toe src/main.cpp src/state.cpp)
target_include_directories(tic_tac_toe PRIVATE include)
target_link_libraries(tic_tac_toe PRIVATE fmt::fmt)

add_subdirectory(tests)
