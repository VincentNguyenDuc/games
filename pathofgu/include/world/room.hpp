#pragma once

#include "ecs/entity.hpp"
#include "items/gu_worm.hpp"

#include <string>
#include <unordered_map>
#include <vector>

using RoomId = std::uint32_t;

struct Room {
    RoomId id;
    std::string name;
    std::string description;
    std::unordered_map<std::string, RoomId> exits; // "north" → connected RoomId
    std::vector<Entity> entities;
    std::vector<GuWorm> dropped_worms;
    bool is_exit = false; // reaching this room wins the game
};
