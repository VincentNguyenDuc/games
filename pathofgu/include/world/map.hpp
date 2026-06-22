#pragma once

#include "ecs/entity.hpp"
#include "items/gu_worm.hpp"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

using MapId = std::uint32_t;

class Map {
public:
    MapId id;
    std::string name;
    std::string description;
    std::unordered_map<std::string, MapId> exits; // "north" → connected MapId
    std::vector<Entity> entities;
    std::vector<GuWorm> dropped_worms;
    std::array<std::array<int, 10>, 10> grid;
    bool is_exit = false; // reaching this map wins the game
};
