#pragma once

#include "resource_producer.hpp"

// Gold Storage and Elixir Storage buildings.
// All storages of the same ResourceType share the global pool:
// total available gold = sum of current across all Gold storages.
struct ResourceStorage {
    ResourceType resource;
    int current;
    int capacity;
};
