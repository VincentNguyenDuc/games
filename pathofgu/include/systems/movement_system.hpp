#pragma once

#include "ecs/registry.hpp"
#include "world/world.hpp"

#include <string>

// Resolves all pending MoveIntentComponents: applies movement, handles door
// transitions, enforces wall and entity collision. Works the same for any entity.
// Returns messages (door transitions, etc.).
std::string resolve_moves(EntityComponentRegistry& reg, World& world);
