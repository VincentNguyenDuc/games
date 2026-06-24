# ECS Engine

The `ecs_engine` is a lightweight Entity Component System shared across all games. It provides entity lifecycle management, a cache-friendly component registry, a DAG-based parallel scheduler, deferred command buffering, and a thread-safe event queue.

## Including It

```cpp
#include "ecs.hpp"  // entity, registry, system, command_buffer, event_queue, engine
```

All headers are flat under `ecs_engine/` — there is no `detail/` subdirectory.

## Linking (CMake)

```cmake
target_link_libraries(my_game PRIVATE ecs_engine)
```

The `ecs_engine` target is a static library that propagates its include directory, so no extra `target_include_directories` is needed.

---

## Entity

An entity is just a `uint16_t` ID — a lightweight handle with no data of its own. Components are associated with it externally via the registry.

```cpp
using Entity = uint16_t;
const Entity ENTITY_LIMIT = 65535;
```

### EntityManager

Manages creation and recycling of entity IDs.

```cpp
EntityManager em;

Entity e = em.createEntity();   // returns next ID, or recycles a destroyed one
em.destroyEntity(e);            // queues e for reuse on the next createEntity()
```

- Throws `std::runtime_error` if the entity limit is exceeded.
- Recycling is lazy — IDs are reused in FIFO order via an internal queue.

---

## EntityComponentRegistry

The registry stores components in typed sparse sets. Each component type gets its own `ComponentStore<T>` with a packed dense array.

### API

```cpp
EntityComponentRegistry reg;

// Add or overwrite a component
reg.addComponent(entity, Health{100, 100});

// Get a pointer to the component (nullptr if absent)
Health* hp = reg.getComponent<Health>(entity);

// Remove (no-op if absent)
reg.removeComponent<Health>(entity);

// Get a snapshot of all entities that have component T
std::vector<Entity> alive = reg.view<Health>();
```

### Storage Layout

```
ComponentStore<T>
  sparse[entity_id]  →  index into dense (or INVALID)
  dense[]            →  packed T values  ← cache-friendly iteration
  entities[]         →  parallel entity IDs
```

- **O(1)** add, get, and remove (array index, no hashing on the hot path).
- **view()** returns a copy of the entity list — safe to call `removeComponent` while iterating.
- **Removal** swaps the target with the last element so `dense` stays packed with no holes.
- **Thread safety** — `getComponent` and `view` use a read-only map lookup (`find`) and are safe to call from concurrent systems. `addComponent` and `removeComponent` are not thread-safe; use a `CommandBuffer` to defer mutations from within a system.

---

## Systems

### ISystem

All ECS systems inherit from `ISystem` and declare which component types they read and write. The scheduler uses these declarations to determine which systems can safely run in parallel.

```cpp
struct ISystem {
    std::vector<ComponentType> reads;   // components read but not written
    std::vector<ComponentType> writes;  // components written (implies read)
    virtual ~ISystem() = default;
    virtual void update(EntityComponentRegistry& reg, CommandBuffer& cmd) = 0;
};
```

`ComponentType` is `std::type_index`. Declare types in the constructor:

```cpp
struct VelocitySystem : ISystem {
    VelocitySystem() {
        reads  = { ComponentType(typeid(Velocity)) };
        writes = { ComponentType(typeid(Position)) };
    }
    void update(EntityComponentRegistry& reg, CommandBuffer&) override {
        for (Entity e : reg.view<Velocity>()) {
            auto* v = reg.getComponent<Velocity>(e);
            auto* p = reg.getComponent<Position>(e);
            if (v && p) { p->x += v->dx; p->y += v->dy; }
        }
    }
};
```

### SystemsScheduler

Builds a dependency DAG from system read/write declarations and sorts it into parallel **waves** using Kahn's algorithm. Systems in the same wave have no shared writer and can run concurrently.

**Conflict rules:**
- write → read on the same type: the writer must run first.
- write → write on the same type: the earlier-registered system runs first.
- read → read: never a conflict; both can run in the same wave.

```cpp
SystemsScheduler sched;
sched.add_system(sysA);  // registration order breaks ties
sched.add_system(sysB);
sched.build();            // must call before get_waves()

for (const auto& wave : sched.get_waves()) {
    // each wave is a std::vector<int> of system indices
}
```

---

## CommandBuffer

Systems must not structurally modify the registry (add/remove components, destroy entities) while other systems may be reading it. Instead they queue mutations into a `CommandBuffer` which is flushed by `World` between waves.

```cpp
void MySystem::update(EntityComponentRegistry& reg, CommandBuffer& cmd) {
    for (Entity e : reg.view<Dying>()) {
        cmd.destroy_entity(e);                      // deferred
        cmd.remove_component<Dying>(e);             // deferred
        cmd.add_component(e, Loot{...});            // deferred
    }
}
```

Available operations:

| Method | Effect (applied at flush) |
|---|---|
| `cmd.add_component(e, T{...})` | Adds or overwrites component T on entity e |
| `cmd.remove_component<T>(e)` | Removes component T from entity e |
| `cmd.destroy_entity(e)` | Destroys entity e via EntityManager |

Flush happens automatically — do not call `flush` yourself when using `World`.

---

## Engine

`Engine` is the top-level orchestrator. It owns the `EntityManager`, `EntityComponentRegistry`, `SystemsScheduler`, `ThreadPool`, and all system instances.

### Setup

```cpp
Engine engine;

// Register systems — Engine takes ownership
auto& vel_sys = engine.add_system<VelocitySystem>();
auto& render  = engine.add_system<RenderSystem>(window);

engine.build();  // builds the scheduler; call once after all add_system() calls
```

`add_system<T>(args...)` constructs T in-place and returns a non-owning reference, useful for holding a pointer to read per-system output after each tick.

### Accessing the Registry

```cpp
Entity e = engine.entities().createEntity();
engine.registry().addComponent(e, Position{0, 0});
```

Both `entities()` and `registry()` have const overloads, so `Engine` can be held by a `const` reference where mutation is not needed.

### Ticking

```cpp
engine.tick();
```

Each `tick()` call:
1. For each scheduler wave:
   a. Submits every system in the wave to the thread pool.
   b. Waits for all systems in the wave to finish.
   c. Flushes each system's `CommandBuffer` sequentially.
2. Returns when all waves and flushes are complete.

Systems in the same wave run concurrently — correctness is guaranteed by the scheduler's conflict analysis. Structural mutations (add/remove component, destroy entity) must always go through `CommandBuffer`.

---

## EventQueue

A thread-safe FIFO queue, typically used to pass commands between an input thread and the game loop.

```cpp
EventQueue<PlayerCommand> queue;

// Producer thread
queue.push(MoveCommand{"north"});

// Consumer thread — blocks until an event is available
PlayerCommand cmd = queue.pop_blocking();

// Consumer thread — returns immediately
std::optional<PlayerCommand> cmd = queue.try_pop();

bool empty = queue.empty();
```

Backed by `std::mutex` + `std::condition_variable`. `pop_blocking()` sleeps with zero CPU usage until `push()` wakes it.
