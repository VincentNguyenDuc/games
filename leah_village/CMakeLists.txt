find_package(ftxui CONFIG REQUIRED)

add_executable(leah_village
    src/main.cpp
    src/game.cpp
)
target_include_directories(leah_village PRIVATE include)
target_link_libraries(leah_village PRIVATE
    ecs_engine
    fmt::fmt
    spdlog::spdlog
    ftxui::screen
    ftxui::dom
    ftxui::component
)

add_subdirectory(tests)
