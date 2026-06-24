#pragma once

#include "ecs.hpp"
#include "items/gu_worm.hpp"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

using ecse::Entity;
using MapId = std::uint32_t;

namespace Cell {
constexpr int Empty = 0;
constexpr int Wall = 1;
constexpr int Door = 2;
} // namespace Cell

struct Door {
    MapId target_map;
    int target_x, target_y; // landing position in the target map
};

class Map {
public:
    MapId id;
    std::string name;
    std::string description;
    bool is_exit = false;

    std::array<std::array<int, 10>, 10> grid{};    // grid[y][x]; 0=Empty 1=Wall 2=Door
    std::unordered_map<std::uint32_t, Door> doors; // cell_key(x,y) → Door
    std::vector<Entity> entities;
    std::vector<GuWorm> dropped_worms;

    static std::uint32_t cell_key(int x, int y) { return static_cast<std::uint32_t>(y * 10 + x); }
};
