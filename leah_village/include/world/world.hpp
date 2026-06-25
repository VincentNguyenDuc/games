#pragma once

#include "world/map.hpp"

#include <vector>

// Owns all maps. Does NOT own ECS state — entities live in the Engine's registry.
class World {
public:
    World();

    Map& current_map() { return maps_[current_map_id_]; }
    const Map& current_map() const { return maps_[current_map_id_]; }

    Map& get_map(MapId id) { return maps_.at(id); }
    const Map& get_map(MapId id) const { return maps_.at(id); }

    void travel_to(MapId id) { current_map_id_ = id; }
    MapId current_map_id() const { return current_map_id_; }

    std::size_t num_maps() const { return maps_.size(); }
    bool map_exists(MapId id) const { return id < maps_.size(); }

private:
    void build_home_village();
    void build_ancient_forest();
    void build_crystal_caves();

    std::vector<Map> maps_;
    MapId current_map_id_{0};
};
