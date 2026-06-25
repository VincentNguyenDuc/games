#pragma once

#include "ecs.hpp"

// Ticks Boost timers and removes expired boosts.
struct BoostSystem : ecse::ISystem {
    float dt{0.f};

    BoostSystem();
    void update(ecse::EntityComponentRegistry&, ecse::CommandBuffer&) override;
};
