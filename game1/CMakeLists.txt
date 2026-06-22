add_executable(game1 src/main.cpp)
target_include_directories(game1 PRIVATE include)
target_link_libraries(game1 PRIVATE fmt::fmt spdlog::spdlog)

add_subdirectory(tests)
