# Introduction

This is a C++20 monorepo containing games built on a shared custom ECS (Entity Component System) engine and built with CMake + vcpkg. Games may be terminal-based (FTXUI) or graphical (raylib).

## Projects

| Project | Description | Status |
|---|---|---|
| [ecs_engine](./ecs_engine.md) | Shared ECS core — entity management, sparse-set registry, thread-safe event queue | Stable |
| [Path of Gu](./games/path_of_gu.md) | Turn-based roguelike dungeon crawler with Gu worm cultivation | Complete |
| [Tic Tac Toe](./games/tic_tac_toe.md) | Classic two-player terminal game | Complete |
| [Leah's Village](./games/leah_village.md) | Graphical real-time village builder (raylib) | In development |

## Monorepo Structure

Each game is an independent CMake target. `ecs_engine` is a static library (`libecs_engine.a`) that any game can link against. The dependency graph is flat — games depend on `ecs_engine`, but not on each other.

```
ecs_engine  ←  path_of_gu
            ←  leah_village
            ←  (future games)

tic_tac_toe        (standalone, no ECS)
```

## Dependencies

| Library | Purpose |
|---|---|
| [fmt](https://github.com/fmtlib/fmt) | String formatting |
| [spdlog](https://github.com/gabime/spdlog) | Logging |
| [FTXUI](https://github.com/ArthurSonzogni/FTXUI) | TUI rendering (path_of_gu) |
| [raylib](https://www.raylib.com/) | Graphical rendering (leah_village) |
| [Catch2](https://github.com/catchorg/Catch2) | Unit testing |
