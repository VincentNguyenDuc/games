#pragma once

#include "ecs.hpp"
#include "items/gu_worm.hpp"

#include <string>

using ecse::Entity;

// Stamped on an entity when it activates an offensive worm or makes a raw stat attack.
// Processed and removed by resolve_attack_effects().
struct AttackEffectComponent {
    Entity target;
    int effect_value;      // pre-mitigation for worm attacks; final for raw stat attacks
    std::string worm_name; // empty = raw stat attack
    GuWormType worm_type = GuWormType::Offensive;
    // Offensive/Defensive → damage HP of target
    // Support             → drain essence of target (e.g. Schemer aperture disruption)
};
