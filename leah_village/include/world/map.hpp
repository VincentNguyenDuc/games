#pragma once

#include "ecs.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

using MapId = std::uint32_t;

namespace Cell {
constexpr int Empty = 0;
constexpr int Wall = 1;
constexpr int Gate = 2;
} // namespace Cell

struct Gate {
    MapId target_map;
    int target_x, target_y; // landing tile in the target map
};

class Map {
public:
    Map(MapId id, std::string name, std::string description, int width, int height);

    MapId id;
    std::string name;
    std::string description;
    int required_player_level{1};

    int width() const { return width_; }
    int height() const { return height_; }

    std::vector<std::vector<int>> grid;                        // grid[y][x]
    std::unordered_map<std::uint32_t, Gate> gates;             // cell_key → Gate
    std::unordered_map<std::uint32_t, ecse::Entity> entity_at; // cell_key → entity
    std::vector<ecse::Entity> entities;                        // all entities on this map

    std::uint32_t cell_key(int x, int y) const {
        return static_cast<std::uint32_t>(y * width_ + x);
    }

    bool in_bounds(int x, int y) const { return x >= 0 && x < width_ && y >= 0 && y < height_; }

    // True if the player can walk onto (x, y): in bounds, not a wall, no building there.
    bool walkable(int x, int y) const;

    ecse::Entity get_entity(int x, int y) const;
    bool has_entity(int x, int y) const;
    void place_entity(int x, int y, ecse::Entity e);
    void remove_entity_at(int x, int y);
    void remove_entity(ecse::Entity e); // removes from entities list (not entity_at)

    void add_gate(int x, int y, Gate g);

private:
    int width_;
    int height_;
};
