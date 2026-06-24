#pragma once

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"
#include "world/world.hpp"

void process_death(EntityComponentRegistry& reg, World& world, Entity entity);
bool pickup_worm(EntityComponentRegistry& reg, World& world, Entity player, int pickup_index);
bool drop_worm(EntityComponentRegistry& reg, World& world, Entity player, int drop_index);
