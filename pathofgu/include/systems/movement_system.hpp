#pragma once

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"
#include "world/world.hpp"

#include <string>

bool move_player(
    EntityComponentRegistry& reg, World& world, Entity player, const std::string& direction
);
