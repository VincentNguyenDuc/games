#pragma once

#include "entity.hpp"

using ecse::Entity;

enum class ConstructionKind { Build, Upgrade };

// Attached to a building entity while it is being built or upgraded.
// Removed by ConstructionSystem when time_remaining_s reaches zero.
struct Construction {
    ConstructionKind kind;
    int target_level; // level the building will be when done
    float time_remaining_s;
    Entity builder; // Builder entity assigned to this job
};
