#pragma once

#include "ecs.hpp"

#include <functional>

// Drains PendingXP components into the player's Experience and fires on_level_up
// whenever a threshold is crossed.
struct LevelUpSystem : ecse::ISystem {
    ecse::Entity player_entity{0};
    std::function<void(int /*new_level*/)> on_level_up;

    LevelUpSystem();
    void update(ecse::EntityComponentRegistry&, ecse::CommandBuffer&) override;
};
