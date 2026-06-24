#pragma once

#include "items/gu_worm.hpp"

#include <string>

// Stamped on an entity when it activates a self-targeting worm (range == 0).
// Processed and removed by resolve_self_effects().
struct SelfEffectComponent {
    GuWormType worm_type;
    int effect_value;
    std::string worm_name;
};
