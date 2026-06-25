#pragma once

#include "ecs.hpp"

#include <functional>

// Ticks Construction timers. When time_remaining_s reaches 0, fires on_complete
// with the finished entity so Game can apply level-up and free the builder.
struct ConstructionSystem : ecse::ISystem {
    float dt{0.f};
    std::function<void(ecse::Entity, bool /*is_upgrade*/)> on_complete;

    ConstructionSystem();
    void update(ecse::EntityComponentRegistry&, ecse::CommandBuffer&) override;
};
