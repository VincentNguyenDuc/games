#pragma once

#include "ecs.hpp"
#include "world/world.hpp"

#include <string>

using ecse::CommandBuffer;
using ecse::Entity;
using ecse::EntityComponentRegistry;
using ecse::ISystem;

struct AiTickSystem : ISystem {
    World& game_world;
    Entity player;
    std::string output;

    AiTickSystem(World& gw, Entity p);
    void update(EntityComponentRegistry& reg, CommandBuffer&) override;
};
