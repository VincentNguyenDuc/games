#pragma once

#include "ecs.hpp"

// Ticks every ResourceProducer each frame.
// If the entity also has a Boost, the production rate is multiplied.
struct ProduceSystem : ecse::ISystem {
    float dt{0.f}; // seconds since last tick — set by Game before each engine.tick()

    ProduceSystem();
    void update(ecse::EntityComponentRegistry&, ecse::CommandBuffer&) override;
};
