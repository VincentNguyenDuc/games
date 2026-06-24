#pragma once

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"
#include "world/world.hpp"

#include <vector>

struct AIDecision {
    Entity entity;
    int hp_damage = 0;
    int essence_damage = 0;
    int worm_slot = -1;     // -1 = raw stat attack; >= 0 = use aperture slot
    bool heal_self = false; // worm targets the enemy itself (Recovery)
};

std::string ai_tick(EntityComponentRegistry& reg, World& world, Entity player);
