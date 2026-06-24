add_library(ecs_engine STATIC
    entity.cpp
    system.cpp
    world.cpp
)
target_include_directories(ecs_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ecs_engine PUBLIC spdlog::spdlog)

add_subdirectory(tests)
