add_library(ecs_engine STATIC
    src/entity.cpp
)
target_include_directories(ecs_engine PUBLIC include)
target_link_libraries(ecs_engine PUBLIC spdlog::spdlog)
