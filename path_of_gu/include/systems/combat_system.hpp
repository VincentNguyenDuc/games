#pragma once

#include "ecs.hpp"

#include <string>

struct ActivationResult {
    bool success;
    std::string message; // empty on success; error description on failure
};

// Validates the worm activation (essence, range, slot), deducts essence, and stamps
// either a SelfEffectComponent (range == 0) or AttackEffectComponent (range > 0) on
// the source entity. Applies equally to player and enemy — no special-casing.
ActivationResult activate_worm(
    EntityComponentRegistry& reg, Entity source, Entity target, int worm_slot
);
