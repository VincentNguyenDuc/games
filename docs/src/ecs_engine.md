# ECS Engine

The `ecs_engine` is a lightweight, header-based Entity Component System shared across all games. It provides entity lifecycle management, a cache-friendly component registry, and a thread-safe event queue.

## Including It

```cpp
#include "ecs.hpp"  // pulls in entity, registry, and event_queue
```

Internal headers live under `detail/` and are not part of the public API.

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
