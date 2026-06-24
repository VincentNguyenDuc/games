#pragma once

// Public API — everything a game needs.
#include "detail/command_buffer.hpp"
#include "detail/entity.hpp"
#include "detail/event_queue.hpp"
#include "detail/registry.hpp"
#include "detail/system.hpp"
#include "world.hpp"

// Intentionally not exposed: detail/thread_pool.hpp, detail/system.hpp internals
// (SystemsScheduler, SystemNode, NodeWaves) — those are owned by World.
