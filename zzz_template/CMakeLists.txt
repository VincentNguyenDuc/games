add_executable(template src/main.cpp)
target_include_directories(template PRIVATE include)
target_link_libraries(template PRIVATE fmt::fmt spdlog::spdlog)

add_subdirectory(tests)
