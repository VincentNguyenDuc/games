#pragma once

#include "ecs/entity.hpp"
#include "ecs/registry.hpp"

#include <string>

struct CombatResult {
    bool success;
    std::string message;
    bool target_died = false;
    int damage_dealt = 0; // set for Offensive/Defensive worms; 0 otherwise
};

CombatResult player_attack(
    EntityComponentRegistry& reg, Entity player, Entity target, int worm_slot
);

// Activates a self-targeting worm (Recovery/Support/Movement) without a target.
CombatResult player_use(EntityComponentRegistry& reg, Entity player, int worm_slot);
