add_executable(pathofgu
    src/main.cpp
    src/ecs/entity.cpp
    src/items/gu_worm_db.cpp
)
target_include_directories(pathofgu PRIVATE include)
target_link_libraries(pathofgu PRIVATE fmt::fmt spdlog::spdlog)

add_subdirectory(tests)
