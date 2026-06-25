#pragma once

#include "ecs.hpp"
#include "world/world.hpp"

using ecse::Entity;
using ecse::EntityComponentRegistry;

void process_death(EntityComponentRegistry& reg, World& world, Entity entity);
bool pickup_worm(EntityComponentRegistry& reg, World& world, Entity player, int pickup_index);
bool drop_worm(EntityComponentRegistry& reg, World& world, Entity player, int drop_index);
