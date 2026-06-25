#pragma once

// Building slot that exists on the grid but cannot be built until the village
// Town Hall reaches `required_town_hall_level`.
// Removed (and building made available) by an unlock check system.
struct Locked {
    int required_town_hall_level;
};
