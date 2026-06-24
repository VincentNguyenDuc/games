#pragma once

#include "ecs.hpp"

#include <string>

struct ActivationResult {
    bool success;
    std::string message;
};

ActivationResult activate_worm(
    EntityComponentRegistry& reg, Entity source, Entity target, int worm_slot
);
