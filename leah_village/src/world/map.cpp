#include "world/map.hpp"

#include <algorithm>

Map::Map(MapId id, std::string name, std::string desc, int width, int height)
    : id(id)
    , name(std::move(name))
    , description(std::move(desc))
    , width_(width)
    , height_(height) {
    grid.assign(height_, std::vector<int>(width_, Cell::Empty));
}

bool Map::walkable(int x, int y) const {
    if (!in_bounds(x, y))
        return false;
    if (grid[y][x] == Cell::Wall)
        return false;
    return entity_at.find(cell_key(x, y)) == entity_at.end();
}

ecse::Entity Map::get_entity(int x, int y) const {
    auto it = entity_at.find(cell_key(x, y));
    return it != entity_at.end() ? it->second : ecse::Entity{0};
}

bool Map::has_entity(int x, int y) const {
    return entity_at.find(cell_key(x, y)) != entity_at.end();
}

void Map::place_entity(int x, int y, ecse::Entity e) {
    entity_at[cell_key(x, y)] = e;
    entities.push_back(e);
}

void Map::remove_entity_at(int x, int y) {
    auto it = entity_at.find(cell_key(x, y));
    if (it != entity_at.end()) {
        remove_entity(it->second);
        entity_at.erase(it);
    }
}

void Map::remove_entity(ecse::Entity e) {
    auto it = std::find(entities.begin(), entities.end(), e);
    if (it != entities.end())
        entities.erase(it);
    // Also erase from entity_at if still present
    for (auto jt = entity_at.begin(); jt != entity_at.end(); ++jt) {
        if (jt->second == e) {
            entity_at.erase(jt);
            break;
        }
    }
}

void Map::add_gate(int x, int y, Gate g) {
    grid[y][x] = Cell::Gate;
    gates[cell_key(x, y)] = g;
}
