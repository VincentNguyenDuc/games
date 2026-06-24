#pragma once

#include "ecs.hpp"

#include <string>

struct SelfEffectTickSystem : ISystem {
    std::string output;

    SelfEffectTickSystem();
    void update(EntityComponentRegistry& reg, CommandBuffer&) override;

    // Callable standalone (e.g. in tests or single-step resolution).
    static std::string resolve(EntityComponentRegistry& reg);
};

struct AttackEffectTickSystem : ISystem {
    std::string output;

    AttackEffectTickSystem();
    void update(EntityComponentRegistry& reg, CommandBuffer&) override;

    static std::string resolve(EntityComponentRegistry& reg);
};
