#pragma once

#include "ecs.hpp"

// Ticks obstacle-clearing timers. When a clear completes, fires on_cleared
// so Game can award resources and destroy the entity.
struct ExtractSystem : ecse::ISystem {
    float dt{0.f};
    std::function<void(ecse::Entity)> on_cleared;

    ExtractSystem();
    void update(ecse::EntityComponentRegistry&, ecse::CommandBuffer&) override;
};
