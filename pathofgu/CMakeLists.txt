add_executable(pathofgu
    src/main.cpp 
    src/ecs/entity.cpp
)
target_include_directories(pathofgu PRIVATE include include/ecs)
target_link_libraries(pathofgu PRIVATE fmt::fmt spdlog::spdlog)

add_subdirectory(tests)
