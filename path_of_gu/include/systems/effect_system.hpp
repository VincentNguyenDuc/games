#pragma once

#include "ecs.hpp"

#include <string>

// Applies all pending SelfEffectComponents (heals, buffs) and removes them.
std::string resolve_self_effects(EntityComponentRegistry& reg);

// Applies all pending AttackEffectComponents (damage, essence drain) and removes them.
std::string resolve_attack_effects(EntityComponentRegistry& reg);
