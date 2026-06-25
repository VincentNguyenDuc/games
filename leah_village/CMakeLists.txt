find_package(ftxui CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_executable(leah_village
    src/main.cpp
    src/game.cpp
    src/assets.cpp
    src/world/map.cpp
    src/world/world.cpp
    src/systems/produce_system.cpp
    src/systems/build_system.cpp
    src/systems/boost_system.cpp
    src/systems/level_up_system.cpp
    src/systems/extract_system.cpp
    src/persistence/save_load.cpp
)

target_include_directories(leah_village PRIVATE include)

target_link_libraries(leah_village PRIVATE
    ecs_engine
    fmt::fmt
    spdlog::spdlog
    ftxui::screen
    ftxui::dom
    ftxui::component
    unofficial::sqlite3::sqlite3
)

add_subdirectory(tests)
