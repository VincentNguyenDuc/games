# "Path of Gu" — Dungeon Crawler RPG Architecture Plan

## Context
Building a CLI turn-based RPG in `rpg/` alongside `tictactoe/`, themed after the xianxia novel *Reverend Insanity*. Primary goal: playable game. Advanced C++ (ECS, multithreading, smart pointers, C++20 coroutines) serve the design naturally.

---

## Story & Setting

**Title**: *Path of Gu*

You are a newly awakened Gu Master trapped inside the Grotto-Heaven of a deceased Rank 6 Immortal. The Grotto-Heaven is collapsing — you have a limited number of turns to escape through its deepest chamber. Along the way you face wild Gu worms, demonic Gu Masters who arrived before you, and the Immortal's own guardian constructs.

**Win condition**: Reach the exit chamber as a Rank 5 Gu Master.  
**Lose condition**: HP drops to 0, or primeval essence fully depleted for 3 consecutive turns (aperture collapses).

---

## Power System (replaces generic RPG stats)

```
Gu Worm (unit of power):
  name:     "Strength Gu", "Iron Skin Gu", "Jade Skin Gu", ...
  rank:     1–5 (mortal grade)
  type:     Offensive | Defensive | Recovery | Movement | Support
  cost:     primeval_essence (consumed each activation)
  effect:   damage bonus, armor, heal, flee chance, etc.

Aperture (replaces Inventory):
  capacity: grows with player rank (rank * 3 slots)
  stored:   vector<GuWorm> — the Gu worms you own

PrimevalEssence:
  current / max  — max grows with rank
  regenerates 20% per turn when not in combat

PlayerRank: 1–5 (Rank 5 = game win)
  advances when accumulated Refinement Points threshold reached
  unlocks higher-rank Gu worms
```

---

## Architecture: ECS

Entities are `uint16_t`. Components are plain data. Systems hold all logic.

```cpp
// Core components
struct Health          { int hp, max_hp; };
struct PrimevalEssence { int current, max; };
struct Aperture        { std::vector<GuWorm> worms; int capacity; };
struct CultivationRank { int rank; int refinement_points; };
struct Position        { uint16_t room_id; };
struct Name            { std::string value; };
struct AIBehavior      { BehaviorType type; };   // Wild, Schemer, Guardian
struct Loot            { std::vector<GuWormDrop> drops; };
```

**Registry** (`include/ecs/registry.hpp`):
```cpp
template<typename T> void         add(Entity, T);
template<typename T> std::optional<std::reference_wrapper<T>> get(Entity);
template<typename T> void         remove(Entity);
template<typename T> std::vector<Entity> view();   // all entities with T
```
Internally: `std::unordered_map<std::type_index, std::any>` of per-type `unordered_map<Entity, T>`.

---

## Threading Model

```
std::jthread input_thread
  └─ reads std::cin, wraps into InputEvent{raw_string}
  └─ pushes to EventQueue<InputEvent>  (mutex + condition_variable)

Main thread (game loop)
  └─ blocks on queue.pop_blocking() each turn
  └─ parses command, dispatches to systems
  └─ after player acts, runs AISystem concurrently:
       for each enemy entity:
           futures.push_back(std::async(std::launch::async, decide_action, entity, registry_snapshot))
       collect futures → apply decisions sequentially (no registry races)
  └─ renders result
```

`EventQueue<T>` (`include/events/event_queue.hpp`): `std::mutex` + `std::condition_variable`.  
`std::jthread` throughout for RAII automatic join.

---

## Smart Pointer Ownership

| Owner | Resource | Pointer type |
|-------|----------|--------------|
| `Game` | `Registry` | `shared_ptr<Registry>` |
| `Game` | `World` | `unique_ptr<World>` |
| `World` | rooms | `unordered_map<RoomId, unique_ptr<Room>>` |
| `ItemDatabase` | `GuWormDefinition` | `shared_ptr<GuWormDefinition>` (flyweight — name/effect shared) |
| `GuWorm` instance | its definition | `shared_ptr<GuWormDefinition>` |
| Systems | registry | `weak_ptr<Registry>` (don't own it) |

---

## Coroutines (C++20) — Rival Gu Master Dialogue

A minimal `Generator<T>` (`include/coroutines/generator.hpp`) using C++20 `<coroutine>`:

```cpp
Generator<std::string> rival_gu_master_speech() {
    co_yield "You dare enter Senior's Grotto-Heaven?";
    co_yield "Every Gu worm here will be mine.";
    co_yield "Survival of the fittest. Don't blame me.";
    co_yield "*activates Blood Sacrifice Gu*";
}
```

`RenderSystem` steps the generator one line per turn during pre-combat dialogue phase. Boss (the Immortal's Guardian) gets a longer coroutine.

---

## File Structure

```
rpg/
├── CMakeLists.txt
├── include/
│   ├── ecs/
│   │   ├── entity.hpp          # Entity = uint32_t, EntityManager (alloc + recycle)
│   │   └── registry.hpp        # Template ComponentRegistry
│   ├── components/
│   │   ├── health.hpp
│   │   ├── primeval_essence.hpp
│   │   ├── aperture.hpp        # Gu worm container
│   │   ├── cultivation_rank.hpp
│   │   ├── position.hpp
│   │   ├── name.hpp
│   │   ├── ai_behavior.hpp     # Wild | Schemer | Guardian
│   │   └── loot.hpp
│   ├── systems/
│   │   ├── combat_system.hpp   # Activates Gu worms, applies effects, checks death
│   │   ├── ai_system.hpp       # std::async per enemy for concurrent decisions
│   │   ├── movement_system.hpp # Room transitions, exit check
│   │   ├── render_system.hpp   # Prints turn state, steps dialogue generators
│   │   ├── loot_system.hpp     # Worm drops on enemy death, pickup
│   │   └── refinement_system.hpp # Refinement points → rank up
│   ├── world/
│   │   ├── room.hpp            # id, name, description, exits map, entity list
│   │   └── world.hpp           # unique_ptr<Room> map, procedural Grotto-Heaven gen
│   ├── items/
│   │   ├── gu_worm.hpp         # GuWormDefinition (shared), GuWorm instance, GuWormDrop
│   │   └── gu_worm_db.hpp      # Catalog of all worm types by rank/type
│   ├── events/
│   │   ├── events.hpp          # std::variant<InputEvent, CombatEvent, DeathEvent, ...>
│   │   └── event_queue.hpp     # Thread-safe EventQueue<T>
│   ├── coroutines/
│   │   ├── generator.hpp       # Generic Generator<T> with promise_type + iterator
│   │   └── dialogue.hpp        # Per-enemy/boss dialogue generators
│   └── game.hpp                # Owns registry, world, jthread, event queue; runs loop
├── src/
│   ├── ecs/entity.cpp
│   ├── systems/{combat,ai,movement,render,loot,refinement}_system.cpp
│   ├── world/{room,world}.cpp
│   ├── items/{gu_worm,gu_worm_db}.cpp
│   ├── coroutines/dialogue.cpp
│   ├── game.cpp
│   └── main.cpp
└── tests/
    ├── CMakeLists.txt
    ├── test_registry.cpp        # add/get/remove/view components
    ├── test_combat.cpp          # damage calc, essence cost, death
    ├── test_event_queue.cpp     # concurrent push/pop thread safety
    └── test_dialogue.cpp        # generator co_yield step-through
```

---

## Implementation Order

1. `ecs/entity.hpp + registry.hpp` — everything depends on this
2. Component headers (data only, trivial)
3. `events/event_queue.hpp`
4. `items/gu_worm.hpp + gu_worm_db.hpp`
5. `world/room.hpp + world.hpp`
6. `coroutines/generator.hpp + dialogue.hpp`
7. Systems: combat → movement → loot → refinement → AI → render
8. `game.hpp + game.cpp` — wires jthread + event loop
9. `main.cpp`
10. Tests throughout

---

## Verification
- `make build GAME=rpg` — clean compile, no warnings
- `make test GAME=rpg` — all Catch2 tests pass
- `make run GAME=rpg` — explore Grotto-Heaven rooms, fight enemies with Gu worms, collect drops, rank up, reach exit
