#pragma once

#include "ecs.hpp"
#include "world/world.hpp"

#include <string>

using ecse::CommandBuffer;
using ecse::EntityComponentRegistry;
using ecse::ISystem;

struct MoveTickSystem : ISystem {
    GameWorld& game_world;
    std::string output;

    explicit MoveTickSystem(GameWorld& gw);
    void update(EntityComponentRegistry& reg, CommandBuffer&) override;

    // Callable standalone for immediate player movement outside the tick loop.
    static std::string resolve(EntityComponentRegistry& reg, GameWorld& world);
};
