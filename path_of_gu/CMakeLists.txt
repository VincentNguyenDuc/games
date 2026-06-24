find_package(ftxui CONFIG REQUIRED)

add_executable(path_of_gu
    src/main.cpp
    src/items/gu_worm_db.cpp
    src/input/parser.cpp
    src/world/world.cpp
    src/systems/render_system.cpp
    src/systems/combat_system.cpp
    src/systems/effect_system.cpp
    src/systems/movement_system.cpp
    src/systems/loot_system.cpp
    src/systems/ai_system.cpp
    src/game.cpp
)
target_include_directories(path_of_gu PRIVATE include)
target_link_libraries(path_of_gu PRIVATE
    ecs_engine
    fmt::fmt
    spdlog::spdlog
    ftxui::screen
    ftxui::dom
    ftxui::component
)

add_subdirectory(tests)
