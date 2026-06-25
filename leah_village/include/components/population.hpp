#pragma once

// Farms grant housing capacity.
// Total village population capacity = sum of `capacity` across all PopulationProvider entities.
struct PopulationProvider {
    int capacity;
};
