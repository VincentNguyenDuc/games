#pragma once

#include "world/map.hpp"

#include <memory>
#include <unordered_map>
#include <utility>

using ecse::Entity;

class GameWorld {
    std::unordered_map<MapId, std::unique_ptr<Map>> maps_;
    MapId next_id_ = 0;

public:
    GameWorld(); // generates the Grotto-Heaven dungeon layout

    Map* get_map(MapId id) const;
    MapId entrance_id() const { return 0; }

    void add_entity(MapId map_id, Entity entity);
    void remove_entity(MapId map_id, Entity entity);
    void move_entity(Entity entity, MapId from, MapId to);

    // Returns a random interior (non-wall, non-door) cell in the map.
    std::pair<int, int> random_empty_cell(MapId id);

private:
    MapId make_map(std::string name, std::string description, bool is_exit = false);
    // Place bidirectional Door cells: (ax,ay) on map a ↔ (bx,by) on map b.
    void connect(MapId a, int ax, int ay, MapId b, int bx, int by);
};
