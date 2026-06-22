add_executable(pathofgu src/main.cpp)
target_include_directories(pathofgu PRIVATE include)
target_link_libraries(pathofgu PRIVATE fmt::fmt)

add_subdirectory(tests)
