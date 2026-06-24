#pragma once

enum class ResourceType { Gold, Elixir };

// Gold Mines and Elixir Collectors.
// ResourceProductionSystem increments `stored` each tick.
// Player collects by transferring stored → matching ResourceStorage entities.
struct ResourceProducer {
    ResourceType resource;
    float rate_per_hour; // units produced per real hour of game time
    float stored;        // accumulated since last collection; capped at capacity
    float capacity;      // max storable before the mine is full
};
