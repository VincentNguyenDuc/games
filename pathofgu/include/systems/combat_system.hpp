#pragma once

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"

#include <string>

struct CombatResult {
    bool success;
    std::string message;
    bool target_died = false;
};

CombatResult player_attack(
    EntityComponentRegistry& reg, Entity player, Entity target, int worm_slot
);
