#include "world/world.hpp"

#include <algorithm>

using namespace ecse;
#include <random>
#include <stdexcept>

// Dungeon layout (map IDs match construction order):
//
//  [0] Collapsed Archway (entrance)
//       | south (5,9)↔(5,0)
//  [1] Spirit Stone Passage
//       | south (5,9)↔(5,0)     east (9,5)↔(0,5)
//  [2] Worm Hive Chamber         [5] Forgotten Cache
//       | south (5,9)↔(5,0)
//  [3] Bloodstained Hall
//       | south (5,9)↔(5,0)
//  [4] Ancient Refinement        west (0,5)↔(9,5)
//       | south (5,9)↔(5,0)     [6] Sealed Side Chamber
//  [7] Guardian's Sanctum (exit)

GameWorld::GameWorld() {
    auto entrance = make_map(
        "Collapsed Archway",
        "You awaken amid shattered jade pillars. The air reeks of decayed Gu worms."
    );
    auto passage = make_map(
        "Spirit Stone Passage",
        "Faintly glowing spirit stones line the walls. Several have already gone dark."
    );
    auto hive = make_map(
        "Worm Hive Chamber", "The floor writhes. Dozens of rank-1 Gu worms nest in the cracks."
    );
    auto hall = make_map(
        "Bloodstained Hall",
        "Dried blood traces old battles between Gu Masters who came before you."
    );
    auto refinery = make_map(
        "Ancient Refinement Chamber",
        "Rusted refinement cauldrons line the walls. Some worms still circle the vats."
    );
    auto cache = make_map(
        "Forgotten Cache",
        "A cramped alcove sealed by a collapsed pillar. Worm casings litter the floor."
    );
    auto sealed = make_map(
        "Sealed Side Chamber",
        "A reinforced door left ajar. The previous occupant did not leave voluntarily."
    );
    auto sanctum = make_map(
        "Guardian's Sanctum",
        "The Immortal's last construct stands here, waiting. The exit seal glows ahead.",
        true
    );

    // South/north connections: door at col 5, bottom row ↔ col 5, top row
    connect(entrance, 5, 9, passage, 5, 0);
    connect(passage, 5, 9, hive, 5, 0);
    connect(hive, 5, 9, hall, 5, 0);
    connect(hall, 5, 9, refinery, 5, 0);
    connect(refinery, 5, 9, sanctum, 5, 0);

    // East/west connections: door at right col, mid row ↔ left col, mid row
    connect(hive, 9, 5, cache, 0, 5);
    connect(refinery, 0, 5, sealed, 9, 5);
}

MapId GameWorld::make_map(std::string name, std::string description, bool is_exit) {
    MapId id = next_id_++;
    auto map = std::make_unique<Map>();
    map->id = id;
    map->name = std::move(name);
    map->description = std::move(description);
    map->is_exit = is_exit;

    // Border = Wall, interior = Empty
    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 10; ++x)
            map->grid[y][x] = (y == 0 || y == 9 || x == 0 || x == 9) ? Cell::Wall : Cell::Empty;

    maps_[id] = std::move(map);
    return id;
}

void GameWorld::connect(MapId a, int ax, int ay, MapId b, int bx, int by) {
    auto& ma = *maps_.at(a);
    auto& mb = *maps_.at(b);

    ma.grid[ay][ax] = Cell::Door;
    ma.doors[Map::cell_key(ax, ay)] = {b, bx, by};

    mb.grid[by][bx] = Cell::Door;
    mb.doors[Map::cell_key(bx, by)] = {a, ax, ay};
}

Map* GameWorld::get_map(MapId id) const {
    auto it = maps_.find(id);
    return it != maps_.end() ? it->second.get() : nullptr;
}

void GameWorld::add_entity(MapId map_id, Entity entity) {
    maps_.at(map_id)->entities.push_back(entity);
}

void GameWorld::remove_entity(MapId map_id, Entity entity) {
    auto& entities = maps_.at(map_id)->entities;
    entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
}

void GameWorld::move_entity(Entity entity, MapId from, MapId to) {
    remove_entity(from, entity);
    add_entity(to, entity);
}

std::pair<int, int> GameWorld::random_empty_cell(MapId id) {
    static std::mt19937 gen(std::random_device{}());
    Map* map = maps_.at(id).get();

    std::vector<std::pair<int, int>> candidates;
    for (int y = 1; y <= 8; ++y)
        for (int x = 1; x <= 8; ++x)
            if (map->grid[y][x] == Cell::Empty)
                candidates.emplace_back(x, y);

    std::uniform_int_distribution<int> dist(0, static_cast<int>(candidates.size()) - 1);
    return candidates[dist(gen)];
}
