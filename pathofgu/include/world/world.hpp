#pragma once

#include "world/map.hpp"

#include <memory>
#include <unordered_map>

class World {
    std::unordered_map<MapId, std::unique_ptr<Map>> maps_;
    MapId next_id_ = 0;

public:
    World(); // generates the Grotto-Heaven dungeon layout

    Map* get_map(MapId id);
    MapId entrance_id() const { return 0; }

    void add_entity(MapId map_id, Entity entity);
    void remove_entity(MapId map_id, Entity entity);
    void move_entity(Entity entity, MapId from, MapId to);

private:
    MapId make_map(std::string name, std::string description, bool is_exit = false);
    void connect(MapId a, const std::string& dir, MapId b); // connects both ways
};
