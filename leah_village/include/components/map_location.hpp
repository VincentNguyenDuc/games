#pragma once

#include <cstdint>

// Which map this entity permanently lives on.
// Buildings and obstacles have this; the player does not (they move between maps).
struct MapLocation {
    std::uint32_t map_id;
};
