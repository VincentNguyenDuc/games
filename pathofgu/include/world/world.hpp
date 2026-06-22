#pragma once

#include "world/room.hpp"

#include <memory>
#include <unordered_map>

class World {
    std::unordered_map<RoomId, std::unique_ptr<Room>> rooms_;
    RoomId next_id_ = 0;

public:
    World(); // generates the Grotto-Heaven dungeon layout

    Room* get_room(RoomId id);
    RoomId entrance_id() const { return 0; }

    void add_entity(RoomId room_id, Entity entity);
    void remove_entity(RoomId room_id, Entity entity);
    void move_entity(Entity entity, RoomId from, RoomId to);

private:
    RoomId make_room(std::string name, std::string description, bool is_exit = false);
    void connect(RoomId a, const std::string& dir, RoomId b); // connects both ways
};
