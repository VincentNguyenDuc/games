#pragma once

#include "ecs.hpp"
#include "world/world.hpp"

using ecse::Entity;
using ecse::EntityComponentRegistry;

void process_death(EntityComponentRegistry& reg, GameWorld& world, Entity entity);
bool pickup_worm(EntityComponentRegistry& reg, GameWorld& world, Entity player, int pickup_index);
bool drop_worm(EntityComponentRegistry& reg, GameWorld& world, Entity player, int drop_index);
