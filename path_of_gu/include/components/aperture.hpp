#pragma once

#include "items/gu_worm.hpp"

#include <vector>

struct Aperture {
    std::vector<GuWorm> worms;
    int capacity; // max worms; grows with cultivation rank (rank * 3)
};
