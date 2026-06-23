find_package(ftxui CONFIG REQUIRED)

add_executable(pathofgu
    src/main.cpp
    src/ecs/entity.cpp
    src/items/gu_worm_db.cpp
    src/input/input_thread.cpp
    src/world/world.cpp
    src/systems/render_system.cpp
    src/systems/combat_system.cpp
    src/systems/movement_system.cpp
    src/systems/loot_system.cpp
    src/systems/ai_system.cpp
    src/game.cpp
)
target_include_directories(pathofgu PRIVATE include)
target_link_libraries(pathofgu PRIVATE
    fmt::fmt
    spdlog::spdlog
    ftxui::screen
    ftxui::dom
    ftxui::component
)

add_subdirectory(tests)
