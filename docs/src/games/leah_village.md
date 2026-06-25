# Leah's Village

> **Status: In development**

A real-time village builder. Place buildings, manage resources, clear obstacles, and expand your settlement.

## Architecture

The game is built on `ecs_engine` for simulation logic and **raylib** for graphical rendering (1280×720 window).

### Layout

```
┌─────────────────────────────────────────────────────┐
│  HUD bar — gold / elixir / level / XP        [60px] │
├──────────────────────────────┬──────────────────────┤
│                              │                      │
│  Map viewport  800×408       │  Detail panel  480px │
│  (20×12 tiles, 40×34px ea.)  │                      │
│                              │                      │
├──────────────────────────────┴──────────────────────┤
│  Status / message log                       [252px] │
└─────────────────────────────────────────────────────┘
```

### Game Loop

```
InitWindow → while (!WindowShouldClose())
    handle_input()   ← raylib key polling
    tick(dt)         ← ECS systems update
    BeginDrawing / render() / EndDrawing
CloseWindow
```

### ECS Systems

| System | Responsibility |
|---|---|
| `ProduceSystem` | Ticks resource producers, fills storage |
| `ConstructionSystem` | Advances build/upgrade timers, fires completion |
| `BoostSystem` | Applies time-limited production multipliers |
| `LevelUpSystem` | Awards level-ups when pending XP thresholds are met |
| `ExtractSystem` | Processes obstacle-clearing timers |

### Key Components

| Component | Data |
|---|---|
| `Building` | type enum, level |
| `Construction` | kind (build/upgrade), remaining time |
| `ResourceProducer` | rate, output resource type |
| `ResourceStorage` | gold/elixir amounts and caps |
| `Obstacle` | gold/elixir reward, clear time |
| `MapLocation` | map ID, tile coordinates |
| `Position` | viewport position (follows cursor) |
| `Selected` | marker — which entity the cursor is on |

### Persistence

Game state is saved to and loaded from SQLite (`leah_village/game.db`) via `unofficial-sqlite3`.

## Running in a Devcontainer

The game requires an X11 display. The devcontainer ships a virtual display stack:

```
Xvfb :99  →  openbox  →  x11vnc (5900)  →  websockify (6080)  →  browser noVNC
```

Access the game at **http://localhost:6080/vnc.html** after the container starts. The `DISPLAY=:99` and `LIBGL_ALWAYS_SOFTWARE=1` env vars are set automatically.
