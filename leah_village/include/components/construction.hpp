#pragma once

#include "entity.hpp"

using ecse::Entity;

enum class ConstructionKind { Build, Upgrade };

// Attached to a building entity while it is being built or upgraded.
struct Construction {
    ConstructionKind kind;
    Entity builder;         // Builder entity assigned to this job
    float time_remaining_s; // counts down to 0 then completes
};
