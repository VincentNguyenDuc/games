#pragma once

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"
#include "world/world.hpp"

#include <vector>

struct AIDecision {
    Entity entity;
    int hp_damage;
    int essence_damage;
};

void ai_tick(EntityComponentRegistry& reg, World& world, Entity player);
