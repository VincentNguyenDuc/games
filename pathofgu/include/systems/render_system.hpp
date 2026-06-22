#pragma once

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"
#include "world/world.hpp"

void render(EntityComponentRegistry& reg, World& world, Entity player);
