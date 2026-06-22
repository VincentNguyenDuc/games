#include "world/world.hpp"

#include <algorithm>
#include <stdexcept>

// Dungeon layout:
//
//  [0] Collapsed Archway (entrance)
//       | south
//  [1] Spirit Stone Passage
//       | south              | east
//  [2] Worm Hive Chamber    [5] Forgotten Cache (dead end, loot)
//       | south
//  [3] Bloodstained Hall
//       | south              | west
//  [4] Ancient Refinement   [6] Sealed Side Chamber (dead end, loot)
//       | south
//  [7] Guardian's Sanctum (boss / exit)

World::World() {
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

    connect(entrance, "south", passage);
    connect(passage, "south", hive);
    connect(hive, "south", hall);
    connect(hive, "east", cache);
    connect(hall, "south", refinery);
    connect(refinery, "south", sanctum);
    connect(refinery, "west", sealed);
}

MapId World::make_map(std::string name, std::string description, bool is_exit) {
    MapId id = next_id_++;
    auto map = std::make_unique<Map>();
    map->id = id;
    map->name = std::move(name);
    map->description = std::move(description);
    map->is_exit = is_exit;
    maps_[id] = std::move(map);
    return id;
}

void World::connect(MapId a, const std::string& dir, MapId b) {
    static const std::unordered_map<std::string, std::string> opposites = {
        {"north", "south"},
        {"south", "north"},
        {"east", "west"},
        {"west", "east"},
    };
    maps_.at(a)->exits[dir] = b;
    maps_.at(b)->exits[opposites.at(dir)] = a;
}

Map* World::get_map(MapId id) {
    auto it = maps_.find(id);
    return it != maps_.end() ? it->second.get() : nullptr;
}

void World::add_entity(MapId map_id, Entity entity) {
    maps_.at(map_id)->entities.push_back(entity);
}

void World::remove_entity(MapId map_id, Entity entity) {
    auto& entities = maps_.at(map_id)->entities;
    entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
}

void World::move_entity(Entity entity, MapId from, MapId to) {
    remove_entity(from, entity);
    add_entity(to, entity);
}
